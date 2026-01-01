/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/usb.h>
#include <holy/scsi.h>
#include <holy/scsicmd.h>
#include <holy/misc.h>

holy_MOD_LICENSE ("GPLv2+");

#define holy_USBMS_DIRECTION_BIT	7

/* Length of CBI command should be always 12 bytes */
#define holy_USBMS_CBI_CMD_SIZE         12
/* CBI class-specific USB request ADSC - it sends CBI (scsi) command to
 * device in DATA stage */
#define holy_USBMS_CBI_ADSC_REQ         0x00

/* The USB Mass Storage Command Block Wrapper.  */
struct holy_usbms_cbw
{
  holy_uint32_t signature;
  holy_uint32_t tag;
  holy_uint32_t transfer_length;
  holy_uint8_t flags;
  holy_uint8_t lun;
  holy_uint8_t length;
  holy_uint8_t cbwcb[16];
} holy_PACKED;

struct holy_usbms_csw
{
  holy_uint32_t signature;
  holy_uint32_t tag;
  holy_uint32_t residue;
  holy_uint8_t status;
} holy_PACKED;

struct holy_usbms_dev
{
  struct holy_usb_device *dev;

  int luns;

  int config;
  int interface;
  struct holy_usb_desc_endp *in;
  struct holy_usb_desc_endp *out;

  int subclass;
  int protocol;
  struct holy_usb_desc_endp *intrpt;
};
typedef struct holy_usbms_dev *holy_usbms_dev_t;

/* FIXME: remove limit.  */
#define MAX_USBMS_DEVICES 128
static holy_usbms_dev_t holy_usbms_devices[MAX_USBMS_DEVICES];
static int first_available_slot = 0;

static holy_usb_err_t
holy_usbms_cbi_cmd (holy_usb_device_t dev, int interface,
                    holy_uint8_t *cbicb)
{
  return holy_usb_control_msg (dev,
                               holy_USB_REQTYPE_CLASS_INTERFACE_OUT,
                               holy_USBMS_CBI_ADSC_REQ, 0, interface,
                               holy_USBMS_CBI_CMD_SIZE, (char*)cbicb);
}

static holy_usb_err_t
holy_usbms_cbi_reset (holy_usb_device_t dev, int interface)
{
  /* Prepare array with Command Block Reset (=CBR) */
  /* CBI specific communication reset command should be send to device
   * via CBI USB class specific request ADCS */
  struct holy_cbi_reset
    {
      holy_uint8_t opcode; /* 0x1d = SEND DIAGNOSTIC */
      holy_uint8_t lun; /* 7-5 LUN, 4-0 flags - for CBR always = 0x04 */
      holy_uint8_t pad[10];
      /* XXX: There is collision between CBI and UFI specifications:
       *      CBI says 0xff, UFI says 0x00 ... probably it does
       *      not matter ... (?) */
    } cbicb = { 0x1d, 0x04,
                { 0xff, 0xff, 0xff, 0xff, 0xff,
                  0xff, 0xff, 0xff, 0xff, 0xff }
              };
  
  return holy_usbms_cbi_cmd (dev, interface, (holy_uint8_t *)&cbicb);
}

static holy_usb_err_t
holy_usbms_bo_reset (holy_usb_device_t dev, int interface)
{
  return holy_usb_control_msg (dev, 0x21, 255, 0, interface, 0, 0);
}

static holy_usb_err_t
holy_usbms_reset (holy_usbms_dev_t dev)
{
  if (dev->protocol == holy_USBMS_PROTOCOL_BULK)
    return holy_usbms_bo_reset (dev->dev, dev->interface);
  else
    return holy_usbms_cbi_reset (dev->dev, dev->interface);
}

static void
holy_usbms_detach (holy_usb_device_t usbdev, int config, int interface)
{
  unsigned i;
  for (i = 0; i < ARRAY_SIZE (holy_usbms_devices); i++)
    if (holy_usbms_devices[i] && holy_usbms_devices[i]->dev == usbdev
	&& holy_usbms_devices[i]->interface == interface
	&& holy_usbms_devices[i]->config == config)
      {
	holy_free (holy_usbms_devices[i]);
	holy_usbms_devices[i] = 0;
      }
}

static int
holy_usbms_attach (holy_usb_device_t usbdev, int configno, int interfno)
{
  struct holy_usb_desc_if *interf
    = usbdev->config[configno].interf[interfno].descif;
  int j;
  holy_uint8_t luns = 0;
  unsigned curnum;
  holy_usb_err_t err = holy_USB_ERR_NONE;

  holy_boot_time ("Attaching USB mass storage");

  if (first_available_slot == ARRAY_SIZE (holy_usbms_devices))
    return 0;

  curnum = first_available_slot;
  first_available_slot++;

  interf = usbdev->config[configno].interf[interfno].descif;

  if ((interf->subclass != holy_USBMS_SUBCLASS_BULK
       /* Experimental support of RBC, MMC-2, UFI, SFF-8070i devices */
       && interf->subclass != holy_USBMS_SUBCLASS_RBC
       && interf->subclass != holy_USBMS_SUBCLASS_MMC2
       && interf->subclass != holy_USBMS_SUBCLASS_UFI
       && interf->subclass != holy_USBMS_SUBCLASS_SFF8070 )
      || (interf->protocol != holy_USBMS_PROTOCOL_BULK
          && interf->protocol != holy_USBMS_PROTOCOL_CBI
          && interf->protocol != holy_USBMS_PROTOCOL_CB))
    return 0;

  holy_usbms_devices[curnum] = holy_zalloc (sizeof (struct holy_usbms_dev));
  if (! holy_usbms_devices[curnum])
    return 0;

  holy_usbms_devices[curnum]->dev = usbdev;
  holy_usbms_devices[curnum]->interface = interfno;
  holy_usbms_devices[curnum]->subclass = interf->subclass;
  holy_usbms_devices[curnum]->protocol = interf->protocol;

  holy_dprintf ("usbms", "alive\n");

  /* Iterate over all endpoints of this interface, at least a
     IN and OUT bulk endpoint are required.  */
  for (j = 0; j < interf->endpointcnt; j++)
    {
      struct holy_usb_desc_endp *endp;
      endp = &usbdev->config[0].interf[interfno].descendp[j];

      if ((endp->endp_addr & 128) && (endp->attrib & 3) == 2)
	/* Bulk IN endpoint.  */
	holy_usbms_devices[curnum]->in = endp;
      else if (!(endp->endp_addr & 128) && (endp->attrib & 3) == 2)
        /* Bulk OUT endpoint.  */
	holy_usbms_devices[curnum]->out = endp;
      else if ((endp->endp_addr & 128) && (endp->attrib & 3) == 3)
        /* Interrupt (IN) endpoint.  */
	holy_usbms_devices[curnum]->intrpt = endp;
    }

  if (!holy_usbms_devices[curnum]->in || !holy_usbms_devices[curnum]->out
      || ((holy_usbms_devices[curnum]->protocol == holy_USBMS_PROTOCOL_CBI)
          && !holy_usbms_devices[curnum]->intrpt))
    {
      holy_free (holy_usbms_devices[curnum]);
      holy_usbms_devices[curnum] = 0;
      return 0;
    }

  holy_dprintf ("usbms", "alive\n");

  /* XXX: Activate the first configuration.  */
  holy_usb_set_configuration (usbdev, 1);

  /* Query the amount of LUNs.  */
  if (holy_usbms_devices[curnum]->protocol == holy_USBMS_PROTOCOL_BULK)
    { /* Only Bulk only devices support Get Max LUN command */
      err = holy_usb_control_msg (usbdev, 0xA1, 254, 0, interfno, 1, (char *) &luns);
  		
      if (err)
        {
          /* In case of a stall, clear the stall.  */
          if (err == holy_USB_ERR_STALL)
	    {
	      holy_usb_clear_halt (usbdev, holy_usbms_devices[curnum]->in->endp_addr);
	      holy_usb_clear_halt (usbdev, holy_usbms_devices[curnum]->out->endp_addr);
	    }
          /* Just set the amount of LUNs to one.  */
          holy_errno = holy_ERR_NONE;
          holy_usbms_devices[curnum]->luns = 1;
        }
      else
        /* luns = 0 means one LUN with ID 0 present ! */
        /* We get from device not number of LUNs but highest
         * LUN number. LUNs are numbered from 0, 
         * i.e. number of LUNs is luns+1 ! */
        holy_usbms_devices[curnum]->luns = luns + 1;
    }
  else
    /* XXX: Does CBI devices support multiple LUNs ?
     * I.e., should we detect number of device's LUNs ? (How?) */
    holy_usbms_devices[curnum]->luns = 1;
    
  holy_dprintf ("usbms", "alive\n");

  usbdev->config[configno].interf[interfno].detach_hook = holy_usbms_detach;

  holy_boot_time ("Attached USB mass storage");

#if 0 /* All this part should be probably deleted.
       * This make trouble on some devices if they are not in
       * Phase Error state - and there they should be not in such state...
       * Bulk only mass storage reset procedure should be used only
       * on place and in time when it is really necessary. */
  /* Reset recovery procedure */
  /* Bulk-Only Mass Storage Reset, after the reset commands
     will be accepted.  */
  holy_usbms_reset (usbdev, i);
  holy_usb_clear_halt (usbdev, usbms->in->endp_addr);
  holy_usb_clear_halt (usbdev, usbms->out->endp_addr);
#endif

  return 1;
}



static int
holy_usbms_iterate (holy_scsi_dev_iterate_hook_t hook, void *hook_data,
		    holy_disk_pull_t pull)
{
  unsigned i;

  if (pull != holy_DISK_PULL_NONE)
    return 0;

  holy_usb_poll_devices (1);

  for (i = 0; i < ARRAY_SIZE (holy_usbms_devices); i++)
    if (holy_usbms_devices[i])
      {
	if (hook (holy_SCSI_SUBSYSTEM_USBMS, i, holy_usbms_devices[i]->luns,
		  hook_data))
	  return 1;
      }

  return 0;
}

static holy_err_t
holy_usbms_transfer_bo (struct holy_scsi *scsi, holy_size_t cmdsize, char *cmd,
		        holy_size_t size, char *buf, int read_write)
{
  struct holy_usbms_cbw cbw;
  holy_usbms_dev_t dev = (holy_usbms_dev_t) scsi->data;
  struct holy_usbms_csw status;
  static holy_uint32_t tag = 0;
  holy_usb_err_t err = holy_USB_ERR_NONE;
  holy_usb_err_t errCSW = holy_USB_ERR_NONE;
  int retrycnt = 3 + 1;
  
  tag++;

 retry:
  retrycnt--;
  if (retrycnt == 0)
    return holy_error (holy_ERR_IO, "USB Mass Storage stalled");

  /* Setup the request.  */
  holy_memset (&cbw, 0, sizeof (cbw));
  cbw.signature = holy_cpu_to_le32_compile_time (0x43425355);
  cbw.tag = tag;
  cbw.transfer_length = holy_cpu_to_le32 (size);
  cbw.flags = (!read_write) << holy_USBMS_DIRECTION_BIT;
  cbw.lun = scsi->lun; /* In USB MS CBW are LUN bits on another place than in SCSI CDB, both should be set correctly. */
  cbw.length = cmdsize;
  holy_memcpy (cbw.cbwcb, cmd, cmdsize);
  
  /* Debug print of CBW content. */
  holy_dprintf ("usb", "CBW: sign=0x%08x tag=0x%08x len=0x%08x\n",
  	cbw.signature, cbw.tag, cbw.transfer_length);
  holy_dprintf ("usb", "CBW: flags=0x%02x lun=0x%02x CB_len=0x%02x\n",
  	cbw.flags, cbw.lun, cbw.length);
  holy_dprintf ("usb", "CBW: cmd:\n %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
  	cbw.cbwcb[ 0], cbw.cbwcb[ 1], cbw.cbwcb[ 2], cbw.cbwcb[ 3],
  	cbw.cbwcb[ 4], cbw.cbwcb[ 5], cbw.cbwcb[ 6], cbw.cbwcb[ 7],
  	cbw.cbwcb[ 8], cbw.cbwcb[ 9], cbw.cbwcb[10], cbw.cbwcb[11],
  	cbw.cbwcb[12], cbw.cbwcb[13], cbw.cbwcb[14], cbw.cbwcb[15]);

  /* Write the request.
   * XXX: Error recovery is maybe still not fully correct. */
  err = holy_usb_bulk_write (dev->dev, dev->out,
			     sizeof (cbw), (char *) &cbw);
  if (err)
    {
      if (err == holy_USB_ERR_STALL)
	{
	  holy_usb_clear_halt (dev->dev, dev->out->endp_addr);
	  goto CheckCSW;
	}
      goto retry;
    }

  /* Read/write the data, (maybe) according to specification.  */
  if (size && (read_write == 0))
    {
      err = holy_usb_bulk_read (dev->dev, dev->in, size, buf);
      holy_dprintf ("usb", "read: %d %d\n", err, holy_USB_ERR_STALL);
      if (err)
        {
          if (err == holy_USB_ERR_STALL)
	    holy_usb_clear_halt (dev->dev, dev->in->endp_addr);
          goto CheckCSW;
        }
      /* Debug print of received data. */
      holy_dprintf ("usb", "buf:\n");
      if (size <= 64)
	{
	  unsigned i;
	  for (i = 0; i < size; i++)
	    holy_dprintf ("usb", "0x%02x: 0x%02x\n", i, buf[i]);
	}
      else
          holy_dprintf ("usb", "Too much data for debug print...\n");
    }
  else if (size)
    {
      err = holy_usb_bulk_write (dev->dev, dev->out, size, buf);
      holy_dprintf ("usb", "write: %d %d\n", err, holy_USB_ERR_STALL);
      holy_dprintf ("usb", "First 16 bytes of sent data:\n %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
  	buf[ 0], buf[ 1], buf[ 2], buf[ 3],
  	buf[ 4], buf[ 5], buf[ 6], buf[ 7],
  	buf[ 8], buf[ 9], buf[10], buf[11],
  	buf[12], buf[13], buf[14], buf[15]);
      if (err)
        {
          if (err == holy_USB_ERR_STALL)
	    holy_usb_clear_halt (dev->dev, dev->out->endp_addr);
          goto CheckCSW;
        }
      /* Debug print of sent data. */
      if (size <= 256)
	{
	  unsigned i;
	  for (i=0; i<size; i++)
	    holy_dprintf ("usb", "0x%02x: 0x%02x\n", i, buf[i]);
	}
      else
          holy_dprintf ("usb", "Too much data for debug print...\n");
    }

  /* Read the status - (maybe) according to specification.  */
CheckCSW:
  errCSW = holy_usb_bulk_read (dev->dev, dev->in,
		    sizeof (status), (char *) &status);
  if (errCSW)
    {
      holy_usb_clear_halt (dev->dev, dev->in->endp_addr);
      errCSW = holy_usb_bulk_read (dev->dev, dev->in,
			        sizeof (status), (char *) &status);
      if (errCSW)
        { /* Bulk-only reset device. */
          holy_dprintf ("usb", "Bulk-only reset device - errCSW\n");
          holy_usbms_reset (dev);
          holy_usb_clear_halt (dev->dev, dev->in->endp_addr);
          holy_usb_clear_halt (dev->dev, dev->out->endp_addr);
	  goto retry;
        }
    }

  /* Debug print of CSW content. */
  holy_dprintf ("usb", "CSW: sign=0x%08x tag=0x%08x resid=0x%08x\n",
  	status.signature, status.tag, status.residue);
  holy_dprintf ("usb", "CSW: status=0x%02x\n", status.status);
  
  /* If phase error or not valid signature, do bulk-only reset device. */
  if ((status.status == 2) ||
      (status.signature != holy_cpu_to_le32_compile_time(0x53425355)))
    { /* Bulk-only reset device. */
      holy_dprintf ("usb", "Bulk-only reset device - bad status\n");
      holy_usbms_reset (dev);
      holy_usb_clear_halt (dev->dev, dev->in->endp_addr);
      holy_usb_clear_halt (dev->dev, dev->out->endp_addr);

      goto retry;
    }

  /* If "command failed" status or data transfer failed -> error */
  if ((status.status || err) && !read_write)
    return holy_error (holy_ERR_READ_ERROR,
		       "error communication with USB Mass Storage device");
  else if ((status.status || err) && read_write)
    return holy_error (holy_ERR_WRITE_ERROR,
		       "error communication with USB Mass Storage device");

  return holy_ERR_NONE;
}

static holy_err_t
holy_usbms_transfer_cbi (struct holy_scsi *scsi, holy_size_t cmdsize, char *cmd,
		        holy_size_t size, char *buf, int read_write)
{
  holy_usbms_dev_t dev = (holy_usbms_dev_t) scsi->data;
  int retrycnt = 3 + 1;
  holy_usb_err_t err = holy_USB_ERR_NONE;
  holy_uint8_t cbicb[holy_USBMS_CBI_CMD_SIZE];
  holy_uint16_t status;
  
 retry:
  retrycnt--;
  if (retrycnt == 0)
    return holy_error (holy_ERR_IO, "USB Mass Storage CBI failed");

  /* Setup the request.  */
  holy_memset (cbicb, 0, sizeof (cbicb));
  holy_memcpy (cbicb, cmd,
               cmdsize >= holy_USBMS_CBI_CMD_SIZE
                 ? holy_USBMS_CBI_CMD_SIZE
                 : cmdsize);
  
  /* Debug print of CBIcb content. */
  holy_dprintf ("usb", "cbicb:\n %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
  	cbicb[ 0], cbicb[ 1], cbicb[ 2], cbicb[ 3],
  	cbicb[ 4], cbicb[ 5], cbicb[ 6], cbicb[ 7],
  	cbicb[ 8], cbicb[ 9], cbicb[10], cbicb[11]);

  /* Write the request.
   * XXX: Error recovery is maybe not correct. */
  err = holy_usbms_cbi_cmd (dev->dev, dev->interface, cbicb);
  if (err)
    {
      holy_dprintf ("usb", "CBI cmdcb setup err=%d\n", err);
      if (err == holy_USB_ERR_STALL)
	{
	  /* Stall in this place probably means bad or unsupported
	   * command, so we will not try it again. */
         return holy_error (holy_ERR_IO, "USB Mass Storage CBI request failed");
	}
      else if (dev->protocol == holy_USBMS_PROTOCOL_CBI)
        {
          /* Try to get status from interrupt pipe */
          err = holy_usb_bulk_read (dev->dev, dev->intrpt,
                                    2, (char*)&status);
          holy_dprintf ("usb", "CBI cmdcb setup status: err=%d, status=0x%x\n", err, status);
        }
        /* Any other error could be transport problem, try it again */
        goto retry;
    }

  /* Read/write the data, (maybe) according to specification.  */
  if (size && (read_write == 0))
    {
      err = holy_usb_bulk_read (dev->dev, dev->in, size, buf);
      holy_dprintf ("usb", "read: %d\n", err);
      if (err)
        {
          if (err == holy_USB_ERR_STALL)
            holy_usb_clear_halt (dev->dev, dev->in->endp_addr);
          goto retry;
        }
    }
  else if (size)
    {
      err = holy_usb_bulk_write (dev->dev, dev->out, size, buf);
      holy_dprintf ("usb", "write: %d\n", err);
      if (err)
        {
          if (err == holy_USB_ERR_STALL)
	    holy_usb_clear_halt (dev->dev, dev->out->endp_addr);
          goto retry;
        }
    }

  /* XXX: It is not clear to me yet, how to check status of CBI
   * data transfer on devices without interrupt pipe.
   * AFAIK there is probably no status phase to indicate possibly
   * bad transported data.
   * Maybe we should do check on higher level, i.e. issue RequestSense
   * command (we do it already in scsi.c) and check returned values
   * (we do not it yet) - ? */
  if (dev->protocol == holy_USBMS_PROTOCOL_CBI)
    { /* Check status in interrupt pipe */
      err = holy_usb_bulk_read (dev->dev, dev->intrpt,
                                2, (char*)&status);
      holy_dprintf ("usb", "read status: %d\n", err);
      if (err)
        {
          /* Try to reset device, because it is probably not standard
           * situation */
          holy_usbms_reset (dev);
          holy_usb_clear_halt (dev->dev, dev->in->endp_addr);
          holy_usb_clear_halt (dev->dev, dev->out->endp_addr);
          holy_usb_clear_halt (dev->dev, dev->intrpt->endp_addr);
          goto retry;
        }
      if (dev->subclass == holy_USBMS_SUBCLASS_UFI)
        {
          /* These devices should return bASC and bASCQ */
          if (status != 0)
            /* Some error, currently we don't care what it is... */
            goto retry;
        }
      else if (dev->subclass == holy_USBMS_SUBCLASS_RBC)
        {
          /* XXX: I don't understand what returns RBC subclass devices,
           * so I don't check it - maybe somebody helps ? */
        }
      else
        {
          /* Any other device should return bType = 0 and some bValue */
          if (status & 0xff)
            return holy_error (holy_ERR_IO, "USB Mass Storage CBI status type != 0");
          status = (status & 0x0300) >> 8;
          switch (status)
            {
              case 0 : /* OK */
                break;
              case 1 : /* Fail */
                goto retry;
                break;
              case 2 : /* Phase error */
              case 3 : /* Persistent Failure */
                holy_dprintf ("usb", "CBI reset device - phase error or persistent failure\n");
                holy_usbms_reset (dev);
                holy_usb_clear_halt (dev->dev, dev->in->endp_addr);
                holy_usb_clear_halt (dev->dev, dev->out->endp_addr);
                holy_usb_clear_halt (dev->dev, dev->intrpt->endp_addr);
                goto retry;
                break;
            }
        }
    }

  if (err)
    return holy_error (holy_ERR_IO, "USB error %d", err);
    
  return holy_ERR_NONE;
}


static holy_err_t
holy_usbms_transfer (struct holy_scsi *scsi, holy_size_t cmdsize, char *cmd,
		        holy_size_t size, char *buf, int read_write)
{
  holy_usbms_dev_t dev = (holy_usbms_dev_t) scsi->data;

  if (dev->protocol == holy_USBMS_PROTOCOL_BULK)
    return holy_usbms_transfer_bo (scsi, cmdsize, cmd, size, buf,
                                   read_write);
  else
    return holy_usbms_transfer_cbi (scsi, cmdsize, cmd, size, buf,
                                    read_write);
}

static holy_err_t
holy_usbms_read (struct holy_scsi *scsi, holy_size_t cmdsize, char *cmd,
		 holy_size_t size, char *buf)
{
  return holy_usbms_transfer (scsi, cmdsize, cmd, size, buf, 0);
}

static holy_err_t
holy_usbms_write (struct holy_scsi *scsi, holy_size_t cmdsize, char *cmd,
		  holy_size_t size, const char *buf)
{
  return holy_usbms_transfer (scsi, cmdsize, cmd, size, (char *) buf, 1);
}

static holy_err_t
holy_usbms_open (int id, int devnum, struct holy_scsi *scsi)
{
  if (id != holy_SCSI_SUBSYSTEM_USBMS)
    return holy_error (holy_ERR_UNKNOWN_DEVICE,
		       "not USB Mass Storage device");

  if (!holy_usbms_devices[devnum])
    holy_usb_poll_devices (1);

  if (!holy_usbms_devices[devnum])
    return holy_error (holy_ERR_UNKNOWN_DEVICE,
		       "unknown USB Mass Storage device");

  scsi->data = holy_usbms_devices[devnum];
  scsi->luns = holy_usbms_devices[devnum]->luns;

  return holy_ERR_NONE;
}

static struct holy_scsi_dev holy_usbms_dev =
  {
    .iterate = holy_usbms_iterate,
    .open = holy_usbms_open,
    .read = holy_usbms_read,
    .write = holy_usbms_write
  };

static struct holy_usb_attach_desc attach_hook =
{
  .class = holy_USB_CLASS_MASS_STORAGE,
  .hook = holy_usbms_attach
};

holy_MOD_INIT(usbms)
{
  holy_usb_register_attach_hook_class (&attach_hook);
  holy_scsi_dev_register (&holy_usbms_dev);
}

holy_MOD_FINI(usbms)
{
  unsigned i;
  for (i = 0; i < ARRAY_SIZE (holy_usbms_devices); i++)
    {
      holy_usbms_devices[i]->dev->config[holy_usbms_devices[i]->config]
	.interf[holy_usbms_devices[i]->interface].detach_hook = 0;
      holy_usbms_devices[i]->dev->config[holy_usbms_devices[i]->config]
	.interf[holy_usbms_devices[i]->interface].attached = 0;
    }
  holy_usb_unregister_attach_hook_class (&attach_hook);
  holy_scsi_dev_unregister (&holy_usbms_dev);
}
