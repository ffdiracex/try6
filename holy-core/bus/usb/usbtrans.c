/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/pci.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/usb.h>
#include <holy/usbtrans.h>
#include <holy/time.h>
#include <holy/cache.h>


static inline unsigned int
holy_usb_bulk_maxpacket (holy_usb_device_t dev,
			 struct holy_usb_desc_endp *endpoint)
{
  /* Use the maximum packet size given in the endpoint descriptor.  */
  if (dev->initialized && endpoint && (unsigned int) endpoint->maxpacket)
    return endpoint->maxpacket;

  return 64;
}


static holy_usb_err_t
holy_usb_execute_and_wait_transfer (holy_usb_device_t dev,
				    holy_usb_transfer_t transfer,
				    int timeout, holy_size_t *actual)
{
  holy_usb_err_t err;
  holy_uint64_t endtime;

  err = dev->controller.dev->setup_transfer (&dev->controller, transfer);
  if (err)
    return err;
  /* endtime moved behind setup transfer to prevent false timeouts
   * while debugging... */
  endtime = holy_get_time_ms () + timeout;
  while (1)
    {
      err = dev->controller.dev->check_transfer (&dev->controller, transfer,
						 actual);
      if (!err)
	return holy_USB_ERR_NONE;
      if (err != holy_USB_ERR_WAIT)
	return err;
      if (holy_get_time_ms () > endtime)
	{
	  err = dev->controller.dev->cancel_transfer (&dev->controller,
						      transfer);
	  if (err)
	    return err;
	  return holy_USB_ERR_TIMEOUT;
	}
      holy_cpu_idle ();
    }
}

holy_usb_err_t
holy_usb_control_msg (holy_usb_device_t dev,
		      holy_uint8_t reqtype,
		      holy_uint8_t request,
		      holy_uint16_t value,
		      holy_uint16_t index,
		      holy_size_t size0, char *data_in)
{
  int i;
  holy_usb_transfer_t transfer;
  int datablocks;
  volatile struct holy_usb_packet_setup *setupdata;
  holy_uint32_t setupdata_addr;
  holy_usb_err_t err;
  unsigned int max;
  struct holy_pci_dma_chunk *data_chunk, *setupdata_chunk;
  volatile char *data;
  holy_uint32_t data_addr;
  holy_size_t size = size0;
  holy_size_t actual;

  /* FIXME: avoid allocation any kind of buffer in a first place.  */
  data_chunk = holy_memalign_dma32 (128, size ? : 16);
  if (!data_chunk)
    return holy_USB_ERR_INTERNAL;
  data = holy_dma_get_virt (data_chunk);
  data_addr = holy_dma_get_phys (data_chunk);
  holy_memcpy ((char *) data, data_in, size);

  holy_arch_sync_dma_caches (data, size);

  holy_dprintf ("usb",
		"control: reqtype=0x%02x req=0x%02x val=0x%02x idx=0x%02x size=%lu\n",
		reqtype, request,  value, index, (unsigned long)size);

  /* Create a transfer.  */
  transfer = holy_malloc (sizeof (*transfer));
  if (! transfer)
    {
      holy_dma_free (data_chunk);
      return holy_USB_ERR_INTERNAL;
    }

  setupdata_chunk = holy_memalign_dma32 (32, sizeof (*setupdata));
  if (! setupdata_chunk)
    {
      holy_free (transfer);
      holy_dma_free (data_chunk);
      return holy_USB_ERR_INTERNAL;
    }

  setupdata = holy_dma_get_virt (setupdata_chunk);
  setupdata_addr = holy_dma_get_phys (setupdata_chunk);

  /* Determine the maximum packet size.  */
  if (dev->descdev.maxsize0)
    max = dev->descdev.maxsize0;
  else
    max = 64;

  holy_dprintf ("usb", "control: transfer = %p, dev = %p\n", transfer, dev);

  datablocks = (size + max - 1) / max;

  /* XXX: Discriminate between different types of control
     messages.  */
  transfer->transcnt = datablocks + 2;
  transfer->size = size; /* XXX ? */
  transfer->endpoint = 0;
  transfer->devaddr = dev->addr;
  transfer->type = holy_USB_TRANSACTION_TYPE_CONTROL;
  transfer->max = max;
  transfer->dev = dev;

  /* Allocate an array of transfer data structures.  */
  transfer->transactions = holy_malloc (transfer->transcnt
					* sizeof (struct holy_usb_transfer));
  if (! transfer->transactions)
    {
      holy_free (transfer);
      holy_dma_free (setupdata_chunk);
      holy_dma_free (data_chunk);
      return holy_USB_ERR_INTERNAL;
    }

  /* Build a Setup packet.  XXX: Endianness.  */
  setupdata->reqtype = reqtype;
  setupdata->request = request;
  setupdata->value = value;
  setupdata->index = index;
  setupdata->length = size;
  holy_arch_sync_dma_caches (setupdata, sizeof (*setupdata));

  transfer->transactions[0].size = sizeof (*setupdata);
  transfer->transactions[0].pid = holy_USB_TRANSFER_TYPE_SETUP;
  transfer->transactions[0].data = setupdata_addr;
  transfer->transactions[0].toggle = 0;

  /* Now the data...  XXX: Is this the right way to transfer control
     transfers?  */
  for (i = 0; i < datablocks; i++)
    {
      holy_usb_transaction_t tr = &transfer->transactions[i + 1];

      tr->size = (size > max) ? max : size;
      /* Use the right most bit as the data toggle.  Simple and
	 effective.  */
      tr->toggle = !(i & 1);
      if (reqtype & 128)
	tr->pid = holy_USB_TRANSFER_TYPE_IN;
      else
	tr->pid = holy_USB_TRANSFER_TYPE_OUT;
      tr->data = data_addr + i * max;
      tr->preceding = i * max;
      size -= max;
    }

  /* End with an empty OUT transaction.  */
  transfer->transactions[datablocks + 1].size = 0;
  transfer->transactions[datablocks + 1].data = 0;
  if ((reqtype & 128) && datablocks)
    transfer->transactions[datablocks + 1].pid = holy_USB_TRANSFER_TYPE_OUT;
  else
    transfer->transactions[datablocks + 1].pid = holy_USB_TRANSFER_TYPE_IN;

  transfer->transactions[datablocks + 1].toggle = 1;

  err = holy_usb_execute_and_wait_transfer (dev, transfer, 1000, &actual);

  holy_dprintf ("usb", "control: err=%d\n", err);

  holy_free (transfer->transactions);
  
  holy_free (transfer);
  holy_dma_free (setupdata_chunk);

  holy_arch_sync_dma_caches (data, size0);
  holy_memcpy (data_in, (char *) data, size0);

  holy_dma_free (data_chunk);

  return err;
}

static holy_usb_transfer_t
holy_usb_bulk_setup_readwrite (holy_usb_device_t dev,
			       struct holy_usb_desc_endp *endpoint,
			       holy_size_t size0, char *data_in,
			       holy_transfer_type_t type)
{
  int i;
  holy_usb_transfer_t transfer;
  int datablocks;
  unsigned int max;
  volatile char *data;
  holy_uint32_t data_addr;
  struct holy_pci_dma_chunk *data_chunk;
  holy_size_t size = size0;
  int toggle = dev->toggle[endpoint->endp_addr];

  holy_dprintf ("usb", "bulk: size=0x%02lx type=%d\n", (unsigned long) size,
		type);

  /* FIXME: avoid allocation any kind of buffer in a first place.  */
  data_chunk = holy_memalign_dma32 (128, size);
  if (!data_chunk)
    return NULL;
  data = holy_dma_get_virt (data_chunk);
  data_addr = holy_dma_get_phys (data_chunk);
  if (type == holy_USB_TRANSFER_TYPE_OUT)
    {
      holy_memcpy ((char *) data, data_in, size);
      holy_arch_sync_dma_caches (data, size);
    }

  /* Create a transfer.  */
  transfer = holy_malloc (sizeof (struct holy_usb_transfer));
  if (! transfer)
    {
      holy_dma_free (data_chunk);
      return NULL;
    }

  max = holy_usb_bulk_maxpacket (dev, endpoint);

  datablocks = ((size + max - 1) / max);
  transfer->transcnt = datablocks;
  transfer->size = size - 1;
  transfer->endpoint = endpoint->endp_addr;
  transfer->devaddr = dev->addr;
  transfer->type = holy_USB_TRANSACTION_TYPE_BULK;
  transfer->dir = type;
  transfer->max = max;
  transfer->dev = dev;
  transfer->last_trans = -1; /* Reset index of last processed transaction (TD) */
  transfer->data_chunk = data_chunk;
  transfer->data = data_in;

  /* Allocate an array of transfer data structures.  */
  transfer->transactions = holy_malloc (transfer->transcnt
					* sizeof (struct holy_usb_transfer));
  if (! transfer->transactions)
    {
      holy_free (transfer);
      holy_dma_free (data_chunk);
      return NULL;
    }

  /* Set up all transfers.  */
  for (i = 0; i < datablocks; i++)
    {
      holy_usb_transaction_t tr = &transfer->transactions[i];

      tr->size = (size > max) ? max : size;
      /* XXX: Use the right most bit as the data toggle.  Simple and
	 effective.  */
      tr->toggle = toggle;
      toggle = toggle ? 0 : 1;
      tr->pid = type;
      tr->data = data_addr + i * max;
      tr->preceding = i * max;
      size -= tr->size;
    }
  return transfer;
}

static void
holy_usb_bulk_finish_readwrite (holy_usb_transfer_t transfer)
{
  holy_usb_device_t dev = transfer->dev;
  int toggle = dev->toggle[transfer->endpoint];

  /* We must remember proper toggle value even if some transactions
   * were not processed - correct value should be inversion of last
   * processed transaction (TD). */
  if (transfer->last_trans >= 0)
    toggle = transfer->transactions[transfer->last_trans].toggle ? 0 : 1;
  else
    toggle = dev->toggle[transfer->endpoint]; /* Nothing done, take original */
  holy_dprintf ("usb", "bulk: toggle=%d\n", toggle);
  dev->toggle[transfer->endpoint] = toggle;

  if (transfer->dir == holy_USB_TRANSFER_TYPE_IN)
    {
      holy_arch_sync_dma_caches (holy_dma_get_virt (transfer->data_chunk),
				 transfer->size + 1);
      holy_memcpy (transfer->data, (void *)
		   holy_dma_get_virt (transfer->data_chunk),
		   transfer->size + 1);
    }

  holy_free (transfer->transactions);
  holy_dma_free (transfer->data_chunk);
  holy_free (transfer);
}

static holy_usb_err_t
holy_usb_bulk_readwrite (holy_usb_device_t dev,
			 struct holy_usb_desc_endp *endpoint,
			 holy_size_t size0, char *data_in,
			 holy_transfer_type_t type, int timeout,
			 holy_size_t *actual)
{
  holy_usb_err_t err;
  holy_usb_transfer_t transfer;

  transfer = holy_usb_bulk_setup_readwrite (dev, endpoint, size0,
					    data_in, type);
  if (!transfer)
    return holy_USB_ERR_INTERNAL;
  err = holy_usb_execute_and_wait_transfer (dev, transfer, timeout, actual);

  holy_usb_bulk_finish_readwrite (transfer);

  return err;
}

static holy_usb_err_t
holy_usb_bulk_readwrite_packetize (holy_usb_device_t dev,
				   struct holy_usb_desc_endp *endpoint,
				   holy_transfer_type_t type,
				   holy_size_t size, char *data)
{
  holy_size_t actual, transferred;
  holy_usb_err_t err = holy_USB_ERR_NONE;
  holy_size_t current_size, position;
  holy_size_t max_bulk_transfer_len = MAX_USB_TRANSFER_LEN;
  holy_size_t max;

  if (dev->controller.dev->max_bulk_tds)
    {
      max = holy_usb_bulk_maxpacket (dev, endpoint);

      /* Calculate max. possible length of bulk transfer */
      max_bulk_transfer_len = dev->controller.dev->max_bulk_tds * max;
    }

  for (position = 0, transferred = 0;
       position < size; position += max_bulk_transfer_len)
    {
      current_size = size - position;
      if (current_size >= max_bulk_transfer_len)
	current_size = max_bulk_transfer_len;
      err = holy_usb_bulk_readwrite (dev, endpoint, current_size,
              &data[position], type, 1000, &actual);
      transferred += actual;
      if (err || (current_size != actual)) break;
    }

  if (!err && transferred != size)
    err = holy_USB_ERR_DATA;
  return err;
}

holy_usb_err_t
holy_usb_bulk_write (holy_usb_device_t dev,
		     struct holy_usb_desc_endp *endpoint,
		     holy_size_t size, char *data)
{
  return holy_usb_bulk_readwrite_packetize (dev, endpoint,
					    holy_USB_TRANSFER_TYPE_OUT,
					    size, data);
}

holy_usb_err_t
holy_usb_bulk_read (holy_usb_device_t dev,
		    struct holy_usb_desc_endp *endpoint,
		    holy_size_t size, char *data)
{
  return holy_usb_bulk_readwrite_packetize (dev, endpoint,
					    holy_USB_TRANSFER_TYPE_IN,
					    size, data);
}

holy_usb_err_t
holy_usb_check_transfer (holy_usb_transfer_t transfer, holy_size_t *actual)
{
  holy_usb_err_t err;
  holy_usb_device_t dev = transfer->dev;

  err = dev->controller.dev->check_transfer (&dev->controller, transfer,
					     actual);
  if (err == holy_USB_ERR_WAIT)
    return err;

  holy_usb_bulk_finish_readwrite (transfer);

  return err;
}

holy_usb_transfer_t
holy_usb_bulk_read_background (holy_usb_device_t dev,
			       struct holy_usb_desc_endp *endpoint,
			       holy_size_t size, void *data)
{
  holy_usb_err_t err;
  holy_usb_transfer_t transfer;

  transfer = holy_usb_bulk_setup_readwrite (dev, endpoint, size,
					    data, holy_USB_TRANSFER_TYPE_IN);
  if (!transfer)
    return NULL;

  err = dev->controller.dev->setup_transfer (&dev->controller, transfer);
  if (err)
    return NULL;

  return transfer;
}

void
holy_usb_cancel_transfer (holy_usb_transfer_t transfer)
{
  holy_usb_device_t dev = transfer->dev;
  dev->controller.dev->cancel_transfer (&dev->controller, transfer);
  holy_errno = holy_ERR_NONE;
}

holy_usb_err_t
holy_usb_bulk_read_extended (holy_usb_device_t dev,
			     struct holy_usb_desc_endp *endpoint,
			     holy_size_t size, char *data,
			     int timeout, holy_size_t *actual)
{
  return holy_usb_bulk_readwrite (dev, endpoint, size, data,
				  holy_USB_TRANSFER_TYPE_IN, timeout, actual);
}
