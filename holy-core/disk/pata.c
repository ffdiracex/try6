/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/ata.h>
#include <holy/scsi.h>
#include <holy/disk.h>
#include <holy/dl.h>
#include <holy/mm.h>
#ifndef holy_MACHINE_MIPS_QEMU_MIPS
#include <holy/pci.h>
#include <holy/cs5536.h>
#else
#define holy_MACHINE_PCI_IO_BASE  0xb4000000
#endif
#include <holy/time.h>

holy_MOD_LICENSE ("GPLv2+");

/* At the moment, only two IDE ports are supported.  */
static const holy_port_t holy_pata_ioaddress[] = { holy_ATA_CH0_PORT1,
						   holy_ATA_CH1_PORT1 };

struct holy_pata_device
{
  /* IDE port to use.  */
  int port;

  /* IO addresses on which the registers for this device can be
     found.  */
  holy_port_t ioaddress;

  /* Two devices can be connected to a single cable.  Use this field
     to select device 0 (commonly known as "master") or device 1
     (commonly known as "slave").  */
  int device;

  int present;

  struct holy_pata_device *next;
};

static struct holy_pata_device *holy_pata_devices;

static inline void
holy_pata_regset (struct holy_pata_device *dev, int reg, int val)
{
  holy_outb (val, dev->ioaddress + reg);
}

static inline holy_uint8_t
holy_pata_regget (struct holy_pata_device *dev, int reg)
{
  return holy_inb (dev->ioaddress + reg);
}

/* Wait for !BSY.  */
static holy_err_t
holy_pata_wait_not_busy (struct holy_pata_device *dev, int milliseconds)
{
  /* ATA requires 400ns (after a write to CMD register) or
     1 PIO cycle (after a DRQ block transfer) before
     first check of BSY.  */
  holy_millisleep (1);

  int i = 1;
  holy_uint8_t sts;
  while ((sts = holy_pata_regget (dev, holy_ATA_REG_STATUS))
	 & holy_ATA_STATUS_BUSY)
    {
      if (i >= milliseconds)
        {
	  holy_dprintf ("pata", "timeout: %dms, status=0x%x\n",
			milliseconds, sts);
	  return holy_error (holy_ERR_TIMEOUT, "PATA timeout");
	}

      holy_millisleep (1);
      i++;
    }

  return holy_ERR_NONE;
}

static inline holy_err_t
holy_pata_check_ready (struct holy_pata_device *dev, int spinup)
{
  if (holy_pata_regget (dev, holy_ATA_REG_STATUS) & holy_ATA_STATUS_BUSY)
    return holy_pata_wait_not_busy (dev, spinup ? holy_ATA_TOUT_SPINUP
				    : holy_ATA_TOUT_STD);

  return holy_ERR_NONE;
}

static inline void
holy_pata_wait (void)
{
  holy_millisleep (50);
}

#ifdef holy_MACHINE_MIPS_QEMU_MIPS
#define holy_ata_to_cpu16(x) ((holy_uint16_t) (x))
#define holy_cpu_to_ata16(x) ((holy_uint16_t) (x))
#else
#define holy_ata_to_cpu16 holy_le_to_cpu16
#define holy_cpu_to_ata16 holy_cpu_to_le16
#endif

static void
holy_pata_pio_read (struct holy_pata_device *dev, char *buf, holy_size_t size)
{ 
  unsigned int i;

  /* Read in the data, word by word.  */
  for (i = 0; i < size / 2; i++)
    holy_set_unaligned16 (buf + 2 * i,
			  holy_ata_to_cpu16 (holy_inw(dev->ioaddress
						     + holy_ATA_REG_DATA)));
  if (size & 1)
    buf[size - 1] = (char) holy_ata_to_cpu16 (holy_inw (dev->ioaddress
						       + holy_ATA_REG_DATA));
}

static void
holy_pata_pio_write (struct holy_pata_device *dev, char *buf, holy_size_t size)
{
  unsigned int i;

  /* Write the data, word by word.  */
  for (i = 0; i < size / 2; i++)
    holy_outw(holy_cpu_to_ata16 (holy_get_unaligned16 (buf + 2 * i)), dev->ioaddress + holy_ATA_REG_DATA);
}

/* ATA pass through support, used by hdparm.mod.  */
static holy_err_t
holy_pata_readwrite (struct holy_ata *disk,
		     struct holy_disk_ata_pass_through_parms *parms,
		     int spinup)
{
  struct holy_pata_device *dev = (struct holy_pata_device *) disk->data;
  holy_size_t nread = 0;
  int i;

  if (! (parms->cmdsize == 0 || parms->cmdsize == 12))
    return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		       "ATAPI non-12 byte commands not supported");

  holy_dprintf ("pata", "pata_pass_through: cmd=0x%x, features=0x%x, sectors=0x%x\n",
		parms->taskfile.cmd,
		parms->taskfile.features,
		parms->taskfile.sectors);
  holy_dprintf ("pata", "lba_high=0x%x, lba_mid=0x%x, lba_low=0x%x, size=%"
		PRIuholy_SIZE "\n",
	        parms->taskfile.lba_high,
	        parms->taskfile.lba_mid,
	        parms->taskfile.lba_low, parms->size);

  /* Set registers.  */
  holy_pata_regset (dev, holy_ATA_REG_DISK, (dev->device << 4)
		    | (parms->taskfile.disk & 0xef));
  if (holy_pata_check_ready (dev, spinup))
    return holy_errno;

  for (i = holy_ATA_REG_SECTORS; i <= holy_ATA_REG_LBAHIGH; i++)
    holy_pata_regset (dev, i,
		     parms->taskfile.raw[7 + (i - holy_ATA_REG_SECTORS)]);
  for (i = holy_ATA_REG_FEATURES; i <= holy_ATA_REG_LBAHIGH; i++)
    holy_pata_regset (dev, i, parms->taskfile.raw[i - holy_ATA_REG_FEATURES]);

  /* Start command. */
  holy_pata_regset (dev, holy_ATA_REG_CMD, parms->taskfile.cmd);

  /* Wait for !BSY.  */
  if (holy_pata_wait_not_busy (dev, holy_ATA_TOUT_DATA))
    return holy_errno;

  /* Check status.  */
  holy_int8_t sts = holy_pata_regget (dev, holy_ATA_REG_STATUS);
  holy_dprintf ("pata", "status=0x%x\n", sts);

  if (parms->cmdsize)
    {
      holy_uint8_t irs;
      /* Wait for !BSY.  */
      if (holy_pata_wait_not_busy (dev, holy_ATA_TOUT_DATA))
	return holy_errno;

      irs = holy_pata_regget (dev, holy_ATAPI_REG_IREASON);
      /* OK if DRQ is asserted and interrupt reason is as expected.  */
      if (!((sts & holy_ATA_STATUS_DRQ)
	    && (irs & holy_ATAPI_IREASON_MASK) == holy_ATAPI_IREASON_CMD_OUT))
	return holy_error (holy_ERR_READ_ERROR, "ATAPI protocol error");
      /* Write the packet.  */
      holy_pata_pio_write (dev, parms->cmd, parms->cmdsize);
    }

  /* Transfer data.  */
  while (nread < parms->size
	 && (sts & (holy_ATA_STATUS_DRQ | holy_ATA_STATUS_ERR))
	 == holy_ATA_STATUS_DRQ)
    {
      unsigned cnt;

      /* Wait for !BSY.  */
      if (holy_pata_wait_not_busy (dev, holy_ATA_TOUT_DATA))
	return holy_errno;

      if (parms->cmdsize)
	{
	  if ((holy_pata_regget (dev, holy_ATAPI_REG_IREASON)
	       & holy_ATAPI_IREASON_MASK) != holy_ATAPI_IREASON_DATA_IN)
	    return holy_error (holy_ERR_READ_ERROR, "ATAPI protocol error");

	  cnt = holy_pata_regget (dev, holy_ATAPI_REG_CNTHIGH) << 8
	    | holy_pata_regget (dev, holy_ATAPI_REG_CNTLOW);
	  holy_dprintf("pata", "DRQ count=%u\n", cnt);

	  /* Count of last transfer may be uneven.  */
	  if (! (0 < cnt && cnt <= parms->size - nread
		 && (! (cnt & 1) || cnt == parms->size - nread)))
	    return holy_error (holy_ERR_READ_ERROR,
			       "invalid ATAPI transfer count");
	}
      else
	cnt = holy_DISK_SECTOR_SIZE;
      if (cnt > parms->size - nread)
	cnt = parms->size - nread;

      if (parms->write)
	holy_pata_pio_write (dev, (char *) parms->buffer + nread, cnt);
      else
	holy_pata_pio_read (dev, (char *) parms->buffer + nread, cnt);

      nread += cnt;
    }
  if (parms->write)
    {
      /* Check for write error.  */
      if (holy_pata_wait_not_busy (dev, holy_ATA_TOUT_DATA))
	return holy_errno;

      if (holy_pata_regget (dev, holy_ATA_REG_STATUS)
	  & (holy_ATA_STATUS_DRQ | holy_ATA_STATUS_ERR))
	return holy_error (holy_ERR_WRITE_ERROR, "ATA write error");
    }
  parms->size = nread;

  /* Wait for !BSY.  */
  if (holy_pata_wait_not_busy (dev, holy_ATA_TOUT_DATA))
    return holy_errno;

  /* Return registers.  */
  for (i = holy_ATA_REG_ERROR; i <= holy_ATA_REG_STATUS; i++)
    parms->taskfile.raw[i - holy_ATA_REG_FEATURES] = holy_pata_regget (dev, i);

  holy_dprintf ("pata", "status=0x%x, error=0x%x, sectors=0x%x\n",
	        parms->taskfile.status,
	        parms->taskfile.error,
		parms->taskfile.sectors);

  if (parms->taskfile.status
      & (holy_ATA_STATUS_DRQ | holy_ATA_STATUS_ERR))
    return holy_error (holy_ERR_READ_ERROR, "PATA passthrough failed");

  return holy_ERR_NONE;
}

static holy_err_t
check_device (struct holy_pata_device *dev)
{
  holy_pata_regset (dev, holy_ATA_REG_DISK, dev->device << 4);
  holy_pata_wait ();

  /* Try to detect if the port is in use by writing to it,
     waiting for a while and reading it again.  If the value
     was preserved, there is a device connected.  */
  holy_pata_regset (dev, holy_ATA_REG_SECTORS, 0x5A);
  holy_pata_wait ();
  holy_uint8_t sec = holy_pata_regget (dev, holy_ATA_REG_SECTORS);
  holy_dprintf ("ata", "sectors=0x%x\n", sec);
  if (sec != 0x5A)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "no device connected");

  /* The above test may detect a second (slave) device
     connected to a SATA controller which supports only one
     (master) device.  It is not safe to use the status register
     READY bit to check for controller channel existence.  Some
     ATAPI commands (RESET, DIAGNOSTIC) may clear this bit.  */

  return holy_ERR_NONE;
}

static holy_err_t
holy_pata_device_initialize (int port, int device, int addr)
{
  struct holy_pata_device *dev;
  struct holy_pata_device **devp;
  holy_err_t err;

  holy_dprintf ("pata", "detecting device %d,%d (0x%x)\n",
		port, device, addr);

  dev = holy_malloc (sizeof(*dev));
  if (! dev)
    return holy_errno;

  /* Setup the device information.  */
  dev->port = port;
  dev->device = device;
  dev->ioaddress = addr + holy_MACHINE_PCI_IO_BASE;
  dev->present = 1;
  dev->next = NULL;

  /* Register the device.  */
  for (devp = &holy_pata_devices; *devp; devp = &(*devp)->next);
  *devp = dev;

  err = check_device (dev);
  if (err)
    holy_print_error ();

  return 0;
}

#ifndef holy_MACHINE_MIPS_QEMU_MIPS
static int
holy_pata_pciinit (holy_pci_device_t dev,
		   holy_pci_id_t pciid,
		   void *data __attribute__ ((unused)))
{
  static int compat_use[2] = { 0 };
  holy_pci_address_t addr;
  holy_uint32_t class;
  holy_uint32_t bar1;
  holy_uint32_t bar2;
  int rega;
  int i;
  static int controller = 0;
  int cs5536 = 0;
  int nports = 2;

  /* Read class.  */
  addr = holy_pci_make_address (dev, holy_PCI_REG_CLASS);
  class = holy_pci_read (addr);

  /* AMD CS5536 Southbridge.  */
  if (pciid == holy_CS5536_PCIID)
    {
      cs5536 = 1;
      nports = 1;
    }

  /* Check if this class ID matches that of a PCI IDE Controller.  */
  if (!cs5536 && (class >> 16 != 0x0101))
    return 0;

  for (i = 0; i < nports; i++)
    {
      /* Set to 0 when the channel operated in compatibility mode.  */
      int compat;

      /* We don't support non-compatibility mode for CS5536.  */
      if (cs5536)
	compat = 0;
      else
	compat = (class >> (8 + 2 * i)) & 1;

      rega = 0;

      /* If the channel is in compatibility mode, just assign the
	 default registers.  */
      if (compat == 0 && !compat_use[i])
	{
	  rega = holy_pata_ioaddress[i];
	  compat_use[i] = 1;
	}
      else if (compat)
	{
	  /* Read the BARs, which either contain a mmapped IO address
	     or the IO port address.  */
	  addr = holy_pci_make_address (dev, holy_PCI_REG_ADDRESSES
					+ sizeof (holy_uint64_t) * i);
	  bar1 = holy_pci_read (addr);
	  addr = holy_pci_make_address (dev, holy_PCI_REG_ADDRESSES
					+ sizeof (holy_uint64_t) * i
					+ sizeof (holy_uint32_t));
	  bar2 = holy_pci_read (addr);

	  /* Check if the BARs describe an IO region.  */
	  if ((bar1 & 1) && (bar2 & 1) && (bar1 & ~3))
	    {
	      rega = bar1 & ~3;
	      addr = holy_pci_make_address (dev, holy_PCI_REG_COMMAND);
	      holy_pci_write_word (addr, holy_pci_read_word (addr)
				   | holy_PCI_COMMAND_IO_ENABLED
				   | holy_PCI_COMMAND_MEM_ENABLED
				   | holy_PCI_COMMAND_BUS_MASTER);

	    }
	}

      holy_dprintf ("pata",
		    "PCI dev (%d,%d,%d) compat=%d rega=0x%x\n",
		    holy_pci_get_bus (dev), holy_pci_get_device (dev),
		    holy_pci_get_function (dev), compat, rega);

      if (rega)
	{
	  holy_errno = holy_ERR_NONE;
	  holy_pata_device_initialize (controller * 2 + i, 0, rega);

	  /* Most errors raised by holy_ata_device_initialize() are harmless.
	     They just indicate this particular drive is not responding, most
	     likely because it doesn't exist.  We might want to ignore specific
	     error types here, instead of printing them.  */
	  if (holy_errno)
	    {
	      holy_print_error ();
	      holy_errno = holy_ERR_NONE;
	    }

	  holy_pata_device_initialize (controller * 2 + i, 1, rega);

	  /* Likewise.  */
	  if (holy_errno)
	    {
	      holy_print_error ();
	      holy_errno = holy_ERR_NONE;
	    }
	}
    }

  controller++;

  return 0;
}

static holy_err_t
holy_pata_initialize (void)
{
  holy_pci_iterate (holy_pata_pciinit, NULL);
  return 0;
}
#else
static holy_err_t
holy_pata_initialize (void)
{
  int i;
  for (i = 0; i < 2; i++)
    {
      holy_pata_device_initialize (i, 0, holy_pata_ioaddress[i]);
      holy_pata_device_initialize (i, 1, holy_pata_ioaddress[i]);
    }
  return 0;
}
#endif

static holy_err_t
holy_pata_open (int id, int devnum, struct holy_ata *ata)
{
  struct holy_pata_device *dev;
  struct holy_pata_device *devfnd = 0;
  holy_err_t err;

  if (id != holy_SCSI_SUBSYSTEM_PATA)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "not a PATA device");

  for (dev = holy_pata_devices; dev; dev = dev->next)
    {
      if (dev->port * 2 + dev->device == devnum)
	{
	  devfnd = dev;
	  break;
	}
    }

  holy_dprintf ("pata", "opening PATA dev `ata%d'\n", devnum);

  if (! devfnd)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "no such PATA device");

  err = check_device (devfnd);
  if (err)
    return err;

  ata->data = devfnd;
  ata->dma = 0;
  ata->maxbuffer = 256 * 512;
  ata->present = &devfnd->present;

  return holy_ERR_NONE;
}

static int
holy_pata_iterate (holy_ata_dev_iterate_hook_t hook, void *hook_data,
		   holy_disk_pull_t pull)
{
  struct holy_pata_device *dev;

  if (pull != holy_DISK_PULL_NONE)
    return 0;

  for (dev = holy_pata_devices; dev; dev = dev->next)
    if (hook (holy_SCSI_SUBSYSTEM_PATA, dev->port * 2 + dev->device,
	      hook_data))
      return 1;

  return 0;
}


static struct holy_ata_dev holy_pata_dev =
  {
    .iterate = holy_pata_iterate,
    .open = holy_pata_open,
    .readwrite = holy_pata_readwrite,
  };




holy_MOD_INIT(ata_pthru)
{
  holy_stop_disk_firmware ();

  /* ATA initialization.  */
  holy_pata_initialize ();

  holy_ata_dev_register (&holy_pata_dev);
}

holy_MOD_FINI(ata_pthru)
{
  holy_ata_dev_unregister (&holy_pata_dev);
}
