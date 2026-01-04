/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/usb.h>
#include <holy/usbtrans.h>
#include <holy/pci.h>
#include <holy/cpu/io.h>
#include <holy/time.h>
#include <holy/cpu/pci.h>
#include <holy/disk.h>

holy_MOD_LICENSE ("GPLv2+");

#define holy_UHCI_IOMASK	(0x7FF << 5)

#define N_QH  256
#define N_TD  640

typedef enum
  {
    holy_UHCI_REG_USBCMD = 0x00,
    holy_UHCI_REG_USBINTR = 0x04,
    holy_UHCI_REG_FLBASEADD = 0x08,
    holy_UHCI_REG_PORTSC1 = 0x10,
    holy_UHCI_REG_PORTSC2 = 0x12,
    holy_UHCI_REG_USBLEGSUP = 0xc0
  } holy_uhci_reg_t;

enum
  {
    holy_UHCI_DETECT_CHANGED = (1 << 1),
    holy_UHCI_DETECT_HAVE_DEVICE = 1,
    holy_UHCI_DETECT_LOW_SPEED = (1 << 8)
  };

/* R/WC legacy support bits */
enum
  {
    holy_UHCI_LEGSUP_END_A20GATE = (1 << 15),
    holy_UHCI_TRAP_BY_64H_WSTAT = (1 << 11),
    holy_UHCI_TRAP_BY_64H_RSTAT = (1 << 10),
    holy_UHCI_TRAP_BY_60H_WSTAT = (1 <<  9),
    holy_UHCI_TRAP_BY_60H_RSTAT = (1 <<  8)
  };

/* Reset all legacy support - clear all R/WC bits and all R/W bits */
#define holy_UHCI_RESET_LEGSUP_SMI ( holy_UHCI_LEGSUP_END_A20GATE \
                                     | holy_UHCI_TRAP_BY_64H_WSTAT \
                                     | holy_UHCI_TRAP_BY_64H_RSTAT \
                                     | holy_UHCI_TRAP_BY_60H_WSTAT \
                                     | holy_UHCI_TRAP_BY_60H_RSTAT )

/* Some UHCI commands */
#define holy_UHCI_CMD_RUN_STOP (1 << 0)
#define holy_UHCI_CMD_HCRESET  (1 << 1)
#define holy_UHCI_CMD_MAXP     (1 << 7)

/* Important bits in structures */
#define holy_UHCI_LINK_TERMINATE	1
#define holy_UHCI_LINK_QUEUE_HEAD	2

enum
  {
    holy_UHCI_REG_PORTSC_CONNECT_CHANGED = 0x0002,
    holy_UHCI_REG_PORTSC_PORT_ENABLED    = 0x0004,
    holy_UHCI_REG_PORTSC_RESUME          = 0x0040,
    holy_UHCI_REG_PORTSC_RESET           = 0x0200,
    holy_UHCI_REG_PORTSC_SUSPEND         = 0x1000,
    holy_UHCI_REG_PORTSC_RW = holy_UHCI_REG_PORTSC_PORT_ENABLED
    | holy_UHCI_REG_PORTSC_RESUME | holy_UHCI_REG_PORTSC_RESET
    | holy_UHCI_REG_PORTSC_SUSPEND,
    /* These bits should not be written as 1 unless we really need it */
    holy_UHCI_PORTSC_RWC = ((1 << 1) | (1 << 3) | (1 << 11) | (3 << 13))
  };

/* UHCI Queue Head.  */
struct holy_uhci_qh
{
  /* Queue head link pointer which points to the next queue head.  */
  holy_uint32_t linkptr;

  /* Queue element link pointer which points to the first data object
     within the queue.  */
  holy_uint32_t elinkptr;

  /* Queue heads are aligned on 16 bytes, pad so a queue head is 16
     bytes so we can store many in a 4K page.  */
  holy_uint8_t pad[8];
} holy_PACKED;

/* UHCI Transfer Descriptor.  */
struct holy_uhci_td
{
  /* Pointer to the next TD in the list.  */
  holy_uint32_t linkptr;

  /* Control and status bits.  */
  holy_uint32_t ctrl_status;

  /* All information required to transfer the Token packet.  */
  holy_uint32_t token;

  /* A pointer to the data buffer, UHCI requires this pointer to be 32
     bits.  */
  holy_uint32_t buffer;

  /* Another linkptr that is not overwritten by the Host Controller.
     This is holy specific.  */
  holy_uint32_t linkptr2;

  /* 3 additional 32 bits words reserved for the Host Controller Driver.  */
  holy_uint32_t data[3];
} holy_PACKED;

typedef volatile struct holy_uhci_td *holy_uhci_td_t;
typedef volatile struct holy_uhci_qh *holy_uhci_qh_t;

struct holy_uhci
{
  holy_port_t iobase;
  volatile holy_uint32_t *framelist_virt;
  holy_uint32_t framelist_phys;
  struct holy_pci_dma_chunk *framelist_chunk;

  /* N_QH Queue Heads.  */
  struct holy_pci_dma_chunk *qh_chunk;
  volatile holy_uhci_qh_t qh_virt;
  holy_uint32_t qh_phys;

  /* N_TD Transfer Descriptors.  */
  struct holy_pci_dma_chunk *td_chunk;
  volatile holy_uhci_td_t td_virt;
  holy_uint32_t td_phys;

  /* Free Transfer Descriptors.  */
  holy_uhci_td_t tdfree;

  int qh_busy[N_QH];

  struct holy_uhci *next;
};

static struct holy_uhci *uhci;

static holy_uint16_t
holy_uhci_readreg16 (struct holy_uhci *u, holy_uhci_reg_t reg)
{
  return holy_inw (u->iobase + reg);
}

#if 0
static holy_uint32_t
holy_uhci_readreg32 (struct holy_uhci *u, holy_uhci_reg_t reg)
{
  return holy_inl (u->iobase + reg);
}
#endif

static void
holy_uhci_writereg16 (struct holy_uhci *u,
		      holy_uhci_reg_t reg, holy_uint16_t val)
{
  holy_outw (val, u->iobase + reg);
}

static void
holy_uhci_writereg32 (struct holy_uhci *u,
		    holy_uhci_reg_t reg, holy_uint32_t val)
{
  holy_outl (val, u->iobase + reg);
}

/* Iterate over all PCI devices.  Determine if a device is an UHCI
   controller.  If this is the case, initialize it.  */
static int
holy_uhci_pci_iter (holy_pci_device_t dev,
		    holy_pci_id_t pciid __attribute__((unused)),
		    void *data __attribute__ ((unused)))
{
  holy_uint32_t class_code;
  holy_uint32_t class;
  holy_uint32_t subclass;
  holy_uint32_t interf;
  holy_uint32_t base;
  holy_uint32_t fp;
  holy_pci_address_t addr;
  struct holy_uhci *u;
  int i;

  addr = holy_pci_make_address (dev, holy_PCI_REG_CLASS);
  class_code = holy_pci_read (addr) >> 8;

  interf = class_code & 0xFF;
  subclass = (class_code >> 8) & 0xFF;
  class = class_code >> 16;

  /* If this is not an UHCI controller, just return.  */
  if (class != 0x0c || subclass != 0x03 || interf != 0x00)
    return 0;

  /* Determine IO base address.  */
  addr = holy_pci_make_address (dev, holy_PCI_REG_ADDRESS_REG4);
  base = holy_pci_read (addr);
  /* Stop if there is no IO space base address defined.  */
  if ((base & holy_PCI_ADDR_SPACE_MASK) != holy_PCI_ADDR_SPACE_IO)
    return 0;

  if ((base & holy_UHCI_IOMASK) == 0)
    return 0;

  /* Set bus master - needed for coreboot or broken BIOSes */
  addr = holy_pci_make_address (dev, holy_PCI_REG_COMMAND);
  holy_pci_write_word(addr, holy_PCI_COMMAND_IO_ENABLED
		      | holy_PCI_COMMAND_BUS_MASTER
		      | holy_PCI_COMMAND_MEM_ENABLED
		      | holy_pci_read_word (addr));

  holy_dprintf ("uhci", "base = %x\n", base);

  /* Allocate memory for the controller and register it.  */
  u = holy_zalloc (sizeof (*u));
  if (! u)
    return 1;

  u->iobase = (base & holy_UHCI_IOMASK) + holy_MACHINE_PCI_IO_BASE;

  /* Reset PIRQ and SMI */
  addr = holy_pci_make_address (dev, holy_UHCI_REG_USBLEGSUP);
  holy_pci_write_word(addr, holy_UHCI_RESET_LEGSUP_SMI);
  /* Reset the HC */
  holy_uhci_writereg16(u, holy_UHCI_REG_USBCMD, holy_UHCI_CMD_HCRESET);
  holy_millisleep(5);
  /* Disable interrupts and commands (just to be safe) */
  holy_uhci_writereg16(u, holy_UHCI_REG_USBINTR, 0);
  /* Finish HC reset, HC remains disabled */
  holy_uhci_writereg16(u, holy_UHCI_REG_USBCMD, 0);
  /* Read back to be sure PCI write is done */
  holy_uhci_readreg16(u, holy_UHCI_REG_USBCMD);

  /* Reserve a page for the frame list.  */
  u->framelist_chunk = holy_memalign_dma32 (4096, 4096);
  if (! u->framelist_chunk)
    goto fail;
  u->framelist_virt = holy_dma_get_virt (u->framelist_chunk);
  u->framelist_phys = holy_dma_get_phys (u->framelist_chunk);

  holy_dprintf ("uhci",
		"class=0x%02x 0x%02x interface 0x%02x base=0x%x framelist=%p\n",
		class, subclass, interf, u->iobase, u->framelist_virt);

  /* The QH pointer of UHCI is only 32 bits, make sure this
     code works on on 64 bits architectures.  */
  u->qh_chunk = holy_memalign_dma32 (4096, sizeof(struct holy_uhci_qh) * N_QH);
  if (! u->qh_chunk)
    goto fail;
  u->qh_virt = holy_dma_get_virt (u->qh_chunk);
  u->qh_phys = holy_dma_get_phys (u->qh_chunk);

  /* The TD pointer of UHCI is only 32 bits, make sure this
     code works on on 64 bits architectures.  */
  u->td_chunk = holy_memalign_dma32 (4096, sizeof(struct holy_uhci_td) * N_TD);
  if (! u->td_chunk)
    goto fail;
  u->td_virt = holy_dma_get_virt (u->td_chunk);
  u->td_phys = holy_dma_get_phys (u->td_chunk);

  holy_dprintf ("uhci", "QH=%p, TD=%p\n",
		u->qh_virt, u->td_virt);

  /* Link all Transfer Descriptors in a list of available Transfer
     Descriptors.  */
  for (i = 0; i < N_TD; i++)
    u->td_virt[i].linkptr = u->td_phys + (i + 1) * sizeof(struct holy_uhci_td);
  u->td_virt[N_TD - 2].linkptr = 0;
  u->tdfree = u->td_virt;

  /* Setup the frame list pointers.  Since no isochronous transfers
     are and will be supported, they all point to the (same!) queue
     head.  */
  fp = u->qh_phys & (~15);
  /* Mark this as a queue head.  */
  fp |= 2;
  for (i = 0; i < 1024; i++)
    u->framelist_virt[i] = fp;
  /* Program the framelist address into the UHCI controller.  */
  holy_uhci_writereg32 (u, holy_UHCI_REG_FLBASEADD, u->framelist_phys);

  /* Make the Queue Heads point to each other.  */
  for (i = 0; i < N_QH; i++)
    {
      /* Point to the next QH.  */
      u->qh_virt[i].linkptr = ((u->qh_phys
				+ (i + 1) * sizeof(struct holy_uhci_qh))
			  & (~15));

      /* This is a QH.  */
      u->qh_virt[i].linkptr |= holy_UHCI_LINK_QUEUE_HEAD;

      /* For the moment, do not point to a Transfer Descriptor.  These
	 are set at transfer time, so just terminate it.  */
      u->qh_virt[i].elinkptr = 1;
    }

  /* The last Queue Head should terminate.  */
  u->qh_virt[N_QH - 1].linkptr = 1;

  /* Enable UHCI again.  */
  holy_uhci_writereg16 (u, holy_UHCI_REG_USBCMD,
                        holy_UHCI_CMD_RUN_STOP | holy_UHCI_CMD_MAXP);

  /* UHCI is initialized and ready for transfers.  */
  holy_dprintf ("uhci", "UHCI initialized\n");


#if 0
  {
    int i;
    for (i = 0; i < 10; i++)
      {
	holy_uint16_t frnum;

	frnum = holy_uhci_readreg16 (u, 6);
	holy_dprintf ("uhci", "Framenum=%d\n", frnum);
	holy_millisleep (100);
      }
  }
#endif

  /* Link to uhci now that initialisation is successful.  */
  u->next = uhci;
  uhci = u;

  return 0;

 fail:
  if (u)
    {
      holy_dma_free (u->qh_chunk);
      holy_dma_free (u->framelist_chunk);
    }
  holy_free (u);

  return 1;
}

static void
holy_uhci_inithw (void)
{
  holy_pci_iterate (holy_uhci_pci_iter, NULL);
}

static holy_uhci_td_t
holy_alloc_td (struct holy_uhci *u)
{
  holy_uhci_td_t ret;

  /* Check if there is a Transfer Descriptor available.  */
  if (! u->tdfree)
    return NULL;

  ret = u->tdfree;
  u->tdfree = holy_dma_phys2virt (u->tdfree->linkptr, u->td_chunk);

  return ret;
}

static void
holy_free_td (struct holy_uhci *u, holy_uhci_td_t td)
{
  td->linkptr = holy_dma_virt2phys (u->tdfree, u->td_chunk);
  u->tdfree = td;
}

static void
holy_free_queue (struct holy_uhci *u, holy_uhci_qh_t qh, holy_uhci_td_t td,
                 holy_usb_transfer_t transfer, holy_size_t *actual)
{
  int i; /* Index of TD in transfer */

  u->qh_busy[qh - u->qh_virt] = 0;

  *actual = 0;
  
  /* Free the TDs in this queue and set last_trans.  */
  for (i=0; td; i++)
    {
      holy_uhci_td_t tdprev;

      holy_dprintf ("uhci", "Freeing %p\n", td);
      /* Check state of TD and possibly set last_trans */
      if (transfer && (td->linkptr & 1))
        transfer->last_trans = i;

      *actual += (td->ctrl_status + 1) & 0x7ff;
      
      /* Unlink the queue.  */
      tdprev = td;
      if (!td->linkptr2)
	td = 0;
      else
	td = holy_dma_phys2virt (td->linkptr2, u->td_chunk);

      /* Free the TD.  */
      holy_free_td (u, tdprev);
    }
}

static holy_uhci_qh_t
holy_alloc_qh (struct holy_uhci *u,
	       holy_transaction_type_t tr __attribute__((unused)))
{
  int i;
  holy_uhci_qh_t qh;

  /* Look for a Queue Head for this transfer.  Skip the first QH if
     this is a Interrupt Transfer.  */
#if 0
  if (tr == holy_USB_TRANSACTION_TYPE_INTERRUPT)
    i = 0;
  else
#endif
    i = 1;

  for (; i < N_QH; i++)
    {
      if (!u->qh_busy[i])
	break;
    }
  qh = &u->qh_virt[i];
  if (i == N_QH)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY,
		  "no free queue heads available");
      return NULL;
    }

  u->qh_busy[qh - u->qh_virt] = 1;

  return qh;
}

static holy_uhci_td_t
holy_uhci_transaction (struct holy_uhci *u, unsigned int endp,
		       holy_transfer_type_t type, unsigned int addr,
		       unsigned int toggle, holy_size_t size,
		       holy_uint32_t data, holy_usb_speed_t speed)
{
  holy_uhci_td_t td;
  static const unsigned int tf[] = { 0x69, 0xE1, 0x2D };

  /* XXX: Check if data is <4GB.  If it isn't, just copy stuff around.
     This is only relevant for 64 bits architectures.  */

  /* Grab a free Transfer Descriptor and initialize it.  */
  td = holy_alloc_td (u);
  if (! td)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY,
		  "no transfer descriptors available for UHCI transfer");
      return 0;
    }

  holy_dprintf ("uhci",
		"transaction: endp=%d, type=%d, addr=%d, toggle=%d, size=%lu data=0x%x td=%p\n",
		endp, type, addr, toggle, (unsigned long) size, data, td);

  /* Don't point to any TD, just terminate.  */
  td->linkptr = 1;

  /* Active!  Only retry a transfer 3 times.  */
  td->ctrl_status = (1 << 23) | (3 << 27) |
                    ((speed == holy_USB_SPEED_LOW) ? (1 << 26) : 0);

  /* If zero bytes are transmitted, size is 0x7FF.  Otherwise size is
     size-1.  */
  if (size == 0)
    size = 0x7FF;
  else
    size = size - 1;

  /* Setup whatever is required for the token packet.  */
  td->token = ((size << 21) | (toggle << 19) | (endp << 15)
	       | (addr << 8) | tf[type]);

  td->buffer = data;

  return td;
}

struct holy_uhci_transfer_controller_data
{
  holy_uhci_qh_t qh;
  holy_uhci_td_t td_first;
};

static holy_usb_err_t
holy_uhci_setup_transfer (holy_usb_controller_t dev,
			  holy_usb_transfer_t transfer)
{
  struct holy_uhci *u = (struct holy_uhci *) dev->data;
  holy_uhci_td_t td;
  holy_uhci_td_t td_prev = NULL;
  int i;
  struct holy_uhci_transfer_controller_data *cdata;

  cdata = holy_malloc (sizeof (*cdata));
  if (!cdata)
    return holy_USB_ERR_INTERNAL;

  cdata->td_first = NULL;

  /* Allocate a queue head for the transfer queue.  */
  cdata->qh = holy_alloc_qh (u, holy_USB_TRANSACTION_TYPE_CONTROL);
  if (! cdata->qh)
    {
      holy_free (cdata);
      return holy_USB_ERR_INTERNAL;
    }

  holy_dprintf ("uhci", "transfer, iobase:%08x\n", u->iobase);
  
  for (i = 0; i < transfer->transcnt; i++)
    {
      holy_usb_transaction_t tr = &transfer->transactions[i];

      td = holy_uhci_transaction (u, transfer->endpoint & 15, tr->pid,
				  transfer->devaddr, tr->toggle,
				  tr->size, tr->data,
				  transfer->dev->speed);
      if (! td)
	{
	  holy_size_t actual = 0;
	  /* Terminate and free.  */
	  if (td_prev)
	    {
	      td_prev->linkptr2 = 0;
	      td_prev->linkptr = 1;
	    }

	  if (cdata->td_first)
	    holy_free_queue (u, cdata->qh, cdata->td_first, NULL, &actual);

	  holy_free (cdata);
	  return holy_USB_ERR_INTERNAL;
	}

      if (! cdata->td_first)
	cdata->td_first = td;
      else
	{
	  td_prev->linkptr2 = holy_dma_virt2phys (td, u->td_chunk);
	  td_prev->linkptr = holy_dma_virt2phys (td, u->td_chunk);
	  td_prev->linkptr |= 4;
	}
      td_prev = td;
    }
  td_prev->linkptr2 = 0;
  td_prev->linkptr = 1;

  holy_dprintf ("uhci", "setup transaction %d\n", transfer->type);

  /* Link it into the queue and terminate.  Now the transaction can
     take place.  */
  cdata->qh->elinkptr = holy_dma_virt2phys (cdata->td_first, u->td_chunk);

  holy_dprintf ("uhci", "initiate transaction\n");

  transfer->controller_data = cdata;

  return holy_USB_ERR_NONE;
}

static holy_usb_err_t
holy_uhci_check_transfer (holy_usb_controller_t dev,
			  holy_usb_transfer_t transfer,
			  holy_size_t *actual)
{
  struct holy_uhci *u = (struct holy_uhci *) dev->data;
  holy_uhci_td_t errtd;
  struct holy_uhci_transfer_controller_data *cdata = transfer->controller_data;

  *actual = 0;

  if (cdata->qh->elinkptr & ~0x0f)
    errtd = holy_dma_phys2virt (cdata->qh->elinkptr & ~0x0f, u->qh_chunk);
  else
    errtd = 0;
  
  if (errtd)
    {
      holy_dprintf ("uhci", ">t status=0x%02x data=0x%02x td=%p, %x\n",
		    errtd->ctrl_status, errtd->buffer & (~15), errtd,
		    cdata->qh->elinkptr);
    }

  /* Check if the transaction completed.  */
  if (cdata->qh->elinkptr & 1)
    {
      holy_dprintf ("uhci", "transaction complete\n");

      /* Place the QH back in the free list and deallocate the associated
	 TDs.  */
      cdata->qh->elinkptr = 1;
      holy_free_queue (u, cdata->qh, cdata->td_first, transfer, actual);
      holy_free (cdata);
      return holy_USB_ERR_NONE;
    }

  if (errtd && !(errtd->ctrl_status & (1 << 23)))
    {
      holy_usb_err_t err = holy_USB_ERR_NONE;

      /* Check if the endpoint is stalled.  */
      if (errtd->ctrl_status & (1 << 22))
	err = holy_USB_ERR_STALL;
      
      /* Check if an error related to the data buffer occurred.  */
      else if (errtd->ctrl_status & (1 << 21))
	err = holy_USB_ERR_DATA;
      
      /* Check if a babble error occurred.  */
      else if (errtd->ctrl_status & (1 << 20))
	err = holy_USB_ERR_BABBLE;
      
      /* Check if a NAK occurred.  */
      else if (errtd->ctrl_status & (1 << 19))
	err = holy_USB_ERR_NAK;
      
      /* Check if a timeout occurred.  */
      else if (errtd->ctrl_status & (1 << 18))
	err = holy_USB_ERR_TIMEOUT;
      
      /* Check if a bitstuff error occurred.  */
      else if (errtd->ctrl_status & (1 << 17))
	err = holy_USB_ERR_BITSTUFF;
      
      if (err)
	{
	  holy_dprintf ("uhci", "transaction failed\n");

	  /* Place the QH back in the free list and deallocate the associated
	     TDs.  */
	  cdata->qh->elinkptr = 1;
	  holy_free_queue (u, cdata->qh, cdata->td_first, transfer, actual);
	  holy_free (cdata);

	  return err;
	}
    }

  /* Fall through, no errors occurred, so the QH might be
     updated.  */
  holy_dprintf ("uhci", "transaction fallthrough\n");

  return holy_USB_ERR_WAIT;
}

static holy_usb_err_t
holy_uhci_cancel_transfer (holy_usb_controller_t dev,
			   holy_usb_transfer_t transfer)
{
  struct holy_uhci *u = (struct holy_uhci *) dev->data;
  holy_size_t actual;
  struct holy_uhci_transfer_controller_data *cdata = transfer->controller_data;

  holy_dprintf ("uhci", "transaction cancel\n");

  /* Place the QH back in the free list and deallocate the associated
     TDs.  */
  cdata->qh->elinkptr = 1;
  holy_free_queue (u, cdata->qh, cdata->td_first, transfer, &actual);
  holy_free (cdata);

  return holy_USB_ERR_NONE;
}

static int
holy_uhci_iterate (holy_usb_controller_iterate_hook_t hook, void *hook_data)
{
  struct holy_uhci *u;
  struct holy_usb_controller dev;

  for (u = uhci; u; u = u->next)
    {
      dev.data = u;
      if (hook (&dev, hook_data))
	return 1;
    }

  return 0;
}

static holy_usb_err_t
holy_uhci_portstatus (holy_usb_controller_t dev,
		      unsigned int port, unsigned int enable)
{
  struct holy_uhci *u = (struct holy_uhci *) dev->data;
  int reg;
  unsigned int status;
  holy_uint64_t endtime;

  holy_dprintf ("uhci", "portstatus, iobase:%08x\n", u->iobase);
  
  holy_dprintf ("uhci", "enable=%d port=%d\n", enable, port);

  if (port == 0)
    reg = holy_UHCI_REG_PORTSC1;
  else if (port == 1)
    reg = holy_UHCI_REG_PORTSC2;
  else
    return holy_USB_ERR_INTERNAL;

  status = holy_uhci_readreg16 (u, reg);
  holy_dprintf ("uhci", "detect=0x%02x\n", status);

  if (!enable) /* We don't need reset port */
    {
      /* Disable the port.  */
      holy_uhci_writereg16 (u, reg, 0 << 2);
      holy_dprintf ("uhci", "waiting for the port to be disabled\n");
      endtime = holy_get_time_ms () + 1000;
      while ((holy_uhci_readreg16 (u, reg) & (1 << 2)))
        if (holy_get_time_ms () > endtime)
          return holy_USB_ERR_TIMEOUT;

      status = holy_uhci_readreg16 (u, reg);
      holy_dprintf ("uhci", ">3detect=0x%02x\n", status);
      return holy_USB_ERR_NONE;
    }
    
  /* Reset the port.  */
  status = holy_uhci_readreg16 (u, reg) & ~holy_UHCI_PORTSC_RWC;
  holy_uhci_writereg16 (u, reg, status | (1 << 9));
  holy_uhci_readreg16 (u, reg); /* Ensure it is writen... */

  /* Wait for the reset to complete.  XXX: How long exactly?  */
  holy_millisleep (50); /* For root hub should be nominaly 50ms */
  status = holy_uhci_readreg16 (u, reg) & ~holy_UHCI_PORTSC_RWC;
  holy_uhci_writereg16 (u, reg, status & ~(1 << 9));
  holy_uhci_readreg16 (u, reg); /* Ensure it is writen... */

  /* Note: some debug prints were removed because they affected reset/enable timing. */

  holy_millisleep (1); /* Probably not needed at all or only few microsecs. */

  /* Reset bits Connect & Enable Status Change */
  status = holy_uhci_readreg16 (u, reg) & ~holy_UHCI_PORTSC_RWC;
  holy_uhci_writereg16 (u, reg, status | (1 << 3) | holy_UHCI_REG_PORTSC_CONNECT_CHANGED);
  holy_uhci_readreg16 (u, reg); /* Ensure it is writen... */

  /* Enable the port.  */
  status = holy_uhci_readreg16 (u, reg) & ~holy_UHCI_PORTSC_RWC;
  holy_uhci_writereg16 (u, reg, status | (1 << 2));
  holy_uhci_readreg16 (u, reg); /* Ensure it is writen... */

  endtime = holy_get_time_ms () + 1000;
  while (! ((status = holy_uhci_readreg16 (u, reg)) & (1 << 2)))
    if (holy_get_time_ms () > endtime)
      return holy_USB_ERR_TIMEOUT;

  /* Reset recovery time */
  holy_millisleep (10);

  /* Read final port status */
  status = holy_uhci_readreg16 (u, reg);
  holy_dprintf ("uhci", ">3detect=0x%02x\n", status);


  return holy_USB_ERR_NONE;
}

static holy_usb_speed_t
holy_uhci_detect_dev (holy_usb_controller_t dev, int port, int *changed)
{
  struct holy_uhci *u = (struct holy_uhci *) dev->data;
  int reg;
  unsigned int status;

  holy_dprintf ("uhci", "detect_dev, iobase:%08x\n", u->iobase);
  
  if (port == 0)
    reg = holy_UHCI_REG_PORTSC1;
  else if (port == 1)
    reg = holy_UHCI_REG_PORTSC2;
  else
    return holy_USB_SPEED_NONE;

  status = holy_uhci_readreg16 (u, reg);

  holy_dprintf ("uhci", "detect=0x%02x port=%d\n", status, port);

  /* Connect Status Change bit - it detects change of connection */
  if (status & holy_UHCI_DETECT_CHANGED)
    {
      *changed = 1;
      /* Reset bit Connect Status Change */
      holy_uhci_writereg16 (u, reg, (status & holy_UHCI_REG_PORTSC_RW)
			    | holy_UHCI_REG_PORTSC_CONNECT_CHANGED);
    }
  else
    *changed = 0;
    
  if (! (status & holy_UHCI_DETECT_HAVE_DEVICE))
    return holy_USB_SPEED_NONE;
  else if (status & holy_UHCI_DETECT_LOW_SPEED)
    return holy_USB_SPEED_LOW;
  else
    return holy_USB_SPEED_FULL;
}

static int
holy_uhci_hubports (holy_usb_controller_t dev __attribute__((unused)))
{
  /* The root hub has exactly two ports.  */
  return 2;
}


static struct holy_usb_controller_dev usb_controller =
{
  .name = "uhci",
  .iterate = holy_uhci_iterate,
  .setup_transfer = holy_uhci_setup_transfer,
  .check_transfer = holy_uhci_check_transfer,
  .cancel_transfer = holy_uhci_cancel_transfer,
  .hubports = holy_uhci_hubports,
  .portstatus = holy_uhci_portstatus,
  .detect_dev = holy_uhci_detect_dev,
  /* estimated max. count of TDs for one bulk transfer */
  .max_bulk_tds = N_TD * 3 / 4
};

holy_MOD_INIT(uhci)
{
  holy_stop_disk_firmware ();

  holy_uhci_inithw ();
  holy_usb_controller_dev_register (&usb_controller);
  holy_dprintf ("uhci", "registered\n");
}

holy_MOD_FINI(uhci)
{
  struct holy_uhci *u;

  /* Disable all UHCI controllers.  */
  for (u = uhci; u; u = u->next)
    holy_uhci_writereg16 (u, holy_UHCI_REG_USBCMD, 0);

  /* Unregister the controller.  */
  holy_usb_controller_dev_unregister (&usb_controller);
}
