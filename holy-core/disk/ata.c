/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/ata.h>
#include <holy/dl.h>
#include <holy/disk.h>
#include <holy/mm.h>
#include <holy/scsi.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_ata_dev_t holy_ata_dev_list;

/* Byteorder has to be changed before strings can be read.  */
static void
holy_ata_strncpy (holy_uint16_t *dst16, holy_uint16_t *src16, holy_size_t len)
{
  unsigned int i;

  for (i = 0; i < len / 2; i++)
    *(dst16++) = holy_swap_bytes16 (*(src16++));
  *dst16 = 0;
}

static void
holy_ata_dumpinfo (struct holy_ata *dev, holy_uint16_t *info)
{
  holy_uint16_t text[21];

  /* The device information was read, dump it for debugging.  */
  holy_ata_strncpy (text, info + 10, 20);
  holy_dprintf ("ata", "Serial: %s\n", (char *) text);
  holy_ata_strncpy (text, info + 23, 8);
  holy_dprintf ("ata", "Firmware: %s\n", (char *) text);
  holy_ata_strncpy (text, info + 27, 40);
  holy_dprintf ("ata", "Model: %s\n", (char *) text);

  if (! dev->atapi)
    {
      holy_dprintf ("ata", "Addressing: %d\n", dev->addr);
      holy_dprintf ("ata", "Sectors: %lld\n", (unsigned long long) dev->size);
      holy_dprintf ("ata", "Sector size: %u\n", 1U << dev->log_sector_size);
    }
}

static holy_err_t
holy_atapi_identify (struct holy_ata *dev)
{
  struct holy_disk_ata_pass_through_parms parms;
  holy_uint16_t *info;
  holy_err_t err;

  info = holy_malloc (holy_DISK_SECTOR_SIZE);
  if (! info)
    return holy_errno;

  holy_memset (&parms, 0, sizeof (parms));
  parms.taskfile.disk = 0xE0;
  parms.taskfile.cmd = holy_ATA_CMD_IDENTIFY_PACKET_DEVICE;
  parms.size = holy_DISK_SECTOR_SIZE;
  parms.buffer = info;

  err = dev->dev->readwrite (dev, &parms, *dev->present);
  if (err)
    {
      *dev->present = 0;
      return err;
    }

  if (parms.size != holy_DISK_SECTOR_SIZE)
    {
      *dev->present = 0;
      return holy_error (holy_ERR_UNKNOWN_DEVICE,
			 "device cannot be identified");
    }

  dev->atapi = 1;

  holy_ata_dumpinfo (dev, info);

  holy_free (info);

  return holy_ERR_NONE;
}

static holy_err_t
holy_ata_identify (struct holy_ata *dev)
{
  struct holy_disk_ata_pass_through_parms parms;
  holy_uint64_t *info64;
  holy_uint32_t *info32;
  holy_uint16_t *info16;
  holy_err_t err;

  if (dev->atapi)
    return holy_atapi_identify (dev);

  info64 = holy_malloc (holy_DISK_SECTOR_SIZE);
  info32 = (holy_uint32_t *) info64;
  info16 = (holy_uint16_t *) info64;
  if (! info16)
    return holy_errno;

  holy_memset (&parms, 0, sizeof (parms));
  parms.buffer = info16;
  parms.size = holy_DISK_SECTOR_SIZE;
  parms.taskfile.disk = 0xE0;

  parms.taskfile.cmd = holy_ATA_CMD_IDENTIFY_DEVICE;

  err = dev->dev->readwrite (dev, &parms, *dev->present);

  if (err || parms.size != holy_DISK_SECTOR_SIZE)
    {
      holy_uint8_t sts = parms.taskfile.status;
      holy_free (info16);
      holy_errno = holy_ERR_NONE;
      if ((sts & (holy_ATA_STATUS_BUSY | holy_ATA_STATUS_DRQ
		   | holy_ATA_STATUS_ERR)) == holy_ATA_STATUS_ERR
	  && (parms.taskfile.error & 0x04 /* ABRT */))
	/* Device without ATA IDENTIFY, try ATAPI.  */
	return holy_atapi_identify (dev);

      else if (sts == 0x00)
	{
	  *dev->present = 0;
	  /* No device, return error but don't print message.  */
	  return holy_ERR_UNKNOWN_DEVICE;
	}
      else
	{
	  *dev->present = 0;
	  /* Other Error.  */
	  return holy_error (holy_ERR_UNKNOWN_DEVICE,
			     "device cannot be identified");
	}
    }

  /* Now it is certain that this is not an ATAPI device.  */
  dev->atapi = 0;

  /* CHS is always supported.  */
  dev->addr = holy_ATA_CHS;

  /* Check if LBA is supported.  */
  if (info16[49] & holy_cpu_to_le16_compile_time ((1 << 9)))
    {
      /* Check if LBA48 is supported.  */
      if (info16[83] & holy_cpu_to_le16_compile_time ((1 << 10)))
	dev->addr = holy_ATA_LBA48;
      else
	dev->addr = holy_ATA_LBA;
    }

  /* Determine the amount of sectors.  */
  if (dev->addr != holy_ATA_LBA48)
    dev->size = holy_le_to_cpu32 (info32[30]);
  else
    dev->size = holy_le_to_cpu64 (info64[25]);

  if (info16[106] & holy_cpu_to_le16_compile_time ((1 << 12)))
    {
      holy_uint32_t secsize;
      secsize = holy_le_to_cpu32 (holy_get_unaligned32 (&info16[117]));
      if (secsize & (secsize - 1) || !secsize
	  || secsize > 1048576)
	secsize = 256;
      for (dev->log_sector_size = 0;
	   (1U << dev->log_sector_size) < secsize;
	   dev->log_sector_size++);
      dev->log_sector_size++;
    }
  else
    dev->log_sector_size = 9;

  /* Read CHS information.  */
  dev->cylinders = holy_le_to_cpu16 (info16[1]);
  dev->heads = holy_le_to_cpu16 (info16[3]);
  dev->sectors_per_track = holy_le_to_cpu16 (info16[6]);

  holy_ata_dumpinfo (dev, info16);

  holy_free (info16);

  return 0;
}

static holy_err_t
holy_ata_setaddress (struct holy_ata *dev,
		     struct holy_disk_ata_pass_through_parms *parms,
		     holy_disk_addr_t sector,
		     holy_size_t size,
		     holy_ata_addressing_t addressing)
{
  switch (addressing)
    {
    case holy_ATA_CHS:
      {
	unsigned int cylinder;
	unsigned int head;
	unsigned int sect;

	if (dev->sectors_per_track == 0
	    || dev->heads == 0)
	  return holy_error (holy_ERR_OUT_OF_RANGE,
			     "sector %d cannot be addressed "
			     "using CHS addressing", sector);

	/* Calculate the sector, cylinder and head to use.  */
	sect = ((holy_uint32_t) sector % dev->sectors_per_track) + 1;
	cylinder = (((holy_uint32_t) sector / dev->sectors_per_track)
		    / dev->heads);
	head = ((holy_uint32_t) sector / dev->sectors_per_track) % dev->heads;

	if (sect > dev->sectors_per_track
	    || cylinder > dev->cylinders
	    || head > dev->heads)
	  return holy_error (holy_ERR_OUT_OF_RANGE,
			     "sector %d cannot be addressed "
			     "using CHS addressing", sector);
	
	parms->taskfile.disk = 0xE0 | head;
	parms->taskfile.sectnum = sect;
	parms->taskfile.cyllsb = cylinder & 0xFF;
	parms->taskfile.cylmsb = cylinder >> 8;

	break;
      }

    case holy_ATA_LBA:
      if (size == 256)
	size = 0;
      parms->taskfile.disk = 0xE0 | ((sector >> 24) & 0x0F);

      parms->taskfile.sectors = size;
      parms->taskfile.lba_low = sector & 0xFF;
      parms->taskfile.lba_mid = (sector >> 8) & 0xFF;
      parms->taskfile.lba_high = (sector >> 16) & 0xFF;
      break;

    case holy_ATA_LBA48:
      if (size == 65536)
	size = 0;

      parms->taskfile.disk = 0xE0;

      /* Set "Previous".  */
      parms->taskfile.sectors = size & 0xFF;
      parms->taskfile.lba_low = sector & 0xFF;
      parms->taskfile.lba_mid = (sector >> 8) & 0xFF;
      parms->taskfile.lba_high = (sector >> 16) & 0xFF;

      /* Set "Current".  */
      parms->taskfile.sectors48 = (size >> 8) & 0xFF;
      parms->taskfile.lba48_low = (sector >> 24) & 0xFF;
      parms->taskfile.lba48_mid = (sector >> 32) & 0xFF;
      parms->taskfile.lba48_high = (sector >> 40) & 0xFF;

      break;
    }

  return holy_ERR_NONE;
}

static holy_err_t
holy_ata_readwrite (holy_disk_t disk, holy_disk_addr_t sector,
		    holy_size_t size, char *buf, int rw)
{
  struct holy_ata *ata = disk->data;

  holy_ata_addressing_t addressing = ata->addr;
  holy_size_t batch;
  int cmd, cmd_write;
  holy_size_t nsectors = 0;

  holy_dprintf("ata", "holy_ata_readwrite (size=%llu, rw=%d)\n",
	       (unsigned long long) size, rw);

  if (addressing == holy_ATA_LBA48 && ((sector + size) >> 28) != 0)
    {
      if (ata->dma)
	{
	  cmd = holy_ATA_CMD_READ_SECTORS_DMA_EXT;
	  cmd_write = holy_ATA_CMD_WRITE_SECTORS_DMA_EXT;
	}
      else
	{
	  cmd = holy_ATA_CMD_READ_SECTORS_EXT;
	  cmd_write = holy_ATA_CMD_WRITE_SECTORS_EXT;
	}
    }
  else
    {
      if (addressing == holy_ATA_LBA48)
	addressing = holy_ATA_LBA;
      if (ata->dma)
	{
	  cmd = holy_ATA_CMD_READ_SECTORS_DMA;
	  cmd_write = holy_ATA_CMD_WRITE_SECTORS_DMA;
	}
      else
	{
	  cmd = holy_ATA_CMD_READ_SECTORS;
	  cmd_write = holy_ATA_CMD_WRITE_SECTORS;
	}
    }

  if (addressing != holy_ATA_CHS)
    batch = 256;
  else
    batch = 1;

  while (nsectors < size)
    {
      struct holy_disk_ata_pass_through_parms parms;
      holy_err_t err;

      if (size - nsectors < batch)
	batch = size - nsectors;

      holy_dprintf("ata", "rw=%d, sector=%llu, batch=%llu\n", rw, (unsigned long long) sector, (unsigned long long) batch);
      holy_memset (&parms, 0, sizeof (parms));
      holy_ata_setaddress (ata, &parms, sector, batch, addressing);
      parms.taskfile.cmd = (! rw ? cmd : cmd_write);
      parms.buffer = buf;
      parms.size = batch << ata->log_sector_size;
      parms.write = rw;
      if (ata->dma)
	parms.dma = 1;
  
      err = ata->dev->readwrite (ata, &parms, 0);
      if (err)
	return err;
      if (parms.size != batch << ata->log_sector_size)
	return holy_error (holy_ERR_READ_ERROR, "incomplete read");
      buf += batch << ata->log_sector_size;
      sector += batch;
      nsectors += batch;
    }

  return holy_ERR_NONE;
}



static inline void
holy_ata_real_close (struct holy_ata *ata)
{
  if (ata->dev->close)
    ata->dev->close (ata);
}

static struct holy_ata *
holy_ata_real_open (int id, int bus)
{
  struct holy_ata *ata;
  holy_ata_dev_t p;

  ata = holy_zalloc (sizeof (*ata));
  if (!ata)
    return NULL;
  for (p = holy_ata_dev_list; p; p = p->next)
    {
      holy_err_t err;
      if (p->open (id, bus, ata))
	{
	  holy_errno = holy_ERR_NONE;
	  continue;
	}
      ata->dev = p;
      /* Use the IDENTIFY DEVICE command to query the device.  */
      err = holy_ata_identify (ata);
      if (err)
	{
	  if (!holy_errno)
	    holy_error (holy_ERR_UNKNOWN_DEVICE, "no such ATA device");
	  holy_free (ata);
	  return NULL;
	}
      return ata;
    }
  holy_free (ata);
  holy_error (holy_ERR_UNKNOWN_DEVICE, "no such ATA device");
  return NULL;
}

/* Context for holy_ata_iterate.  */
struct holy_ata_iterate_ctx
{
  holy_disk_dev_iterate_hook_t hook;
  void *hook_data;
};

/* Helper for holy_ata_iterate.  */
static int
holy_ata_iterate_iter (int id, int bus, void *data)
{
  struct holy_ata_iterate_ctx *ctx = data;
  struct holy_ata *ata;
  int ret;
  char devname[40];

  ata = holy_ata_real_open (id, bus);

  if (!ata)
    {
      holy_errno = holy_ERR_NONE;
      return 0;
    }
  if (ata->atapi)
    {
      holy_ata_real_close (ata);
      return 0;
    }
  holy_snprintf (devname, sizeof (devname),
		 "%s%d", holy_scsi_names[id], bus);
  ret = ctx->hook (devname, ctx->hook_data);
  holy_ata_real_close (ata);
  return ret;
}

static int
holy_ata_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data,
		  holy_disk_pull_t pull)
{
  struct holy_ata_iterate_ctx ctx = { hook, hook_data };
  holy_ata_dev_t p;
  
  for (p = holy_ata_dev_list; p; p = p->next)
    if (p->iterate && p->iterate (holy_ata_iterate_iter, &ctx, pull))
      return 1;
  return 0;
}

static holy_err_t
holy_ata_open (const char *name, holy_disk_t disk)
{
  unsigned id, bus;
  struct holy_ata *ata;

  for (id = 0; id < holy_SCSI_NUM_SUBSYSTEMS; id++)
    if (holy_strncmp (holy_scsi_names[id], name,
		      holy_strlen (holy_scsi_names[id])) == 0
	&& holy_isdigit (name[holy_strlen (holy_scsi_names[id])]))
      break;
  if (id == holy_SCSI_NUM_SUBSYSTEMS)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "not an ATA harddisk");
  bus = holy_strtoul (name + holy_strlen (holy_scsi_names[id]), 0, 0);
  ata = holy_ata_real_open (id, bus);
  if (!ata)
    return holy_errno;

  if (ata->atapi)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "not an ATA harddisk");

  disk->total_sectors = ata->size;
  disk->max_agglomerate = (ata->maxbuffer >> (holy_DISK_CACHE_BITS + holy_DISK_SECTOR_BITS));
  if (disk->max_agglomerate > (256U >> (holy_DISK_CACHE_BITS + holy_DISK_SECTOR_BITS - ata->log_sector_size)))
    disk->max_agglomerate = (256U >> (holy_DISK_CACHE_BITS + holy_DISK_SECTOR_BITS - ata->log_sector_size));

  disk->log_sector_size = ata->log_sector_size;

  disk->id = holy_make_scsi_id (id, bus, 0);

  disk->data = ata;

  return 0;
}

static void
holy_ata_close (holy_disk_t disk)
{
  struct holy_ata *ata = disk->data;
  holy_ata_real_close (ata);
}

static holy_err_t
holy_ata_read (holy_disk_t disk, holy_disk_addr_t sector,
	       holy_size_t size, char *buf)
{
  return holy_ata_readwrite (disk, sector, size, buf, 0);
}

static holy_err_t
holy_ata_write (holy_disk_t disk,
		holy_disk_addr_t sector,
		holy_size_t size,
		const char *buf)
{
  return holy_ata_readwrite (disk, sector, size, (char *) buf, 1);
}

static struct holy_disk_dev holy_atadisk_dev =
  {
    .name = "ATA",
    .id = holy_DISK_DEVICE_ATA_ID,
    .iterate = holy_ata_iterate,
    .open = holy_ata_open,
    .close = holy_ata_close,
    .read = holy_ata_read,
    .write = holy_ata_write,
    .next = 0
  };



/* ATAPI code.  */

static holy_err_t
holy_atapi_read (struct holy_scsi *scsi, holy_size_t cmdsize, char *cmd,
		 holy_size_t size, char *buf)
{
  struct holy_ata *dev = scsi->data;
  struct holy_disk_ata_pass_through_parms parms;
  holy_err_t err;

  holy_dprintf("ata", "holy_atapi_read (size=%llu)\n", (unsigned long long) size);
  holy_memset (&parms, 0, sizeof (parms));

  parms.taskfile.disk = 0;
  parms.taskfile.features = 0;
  parms.taskfile.atapi_ireason = 0;
  parms.taskfile.atapi_cnthigh = size >> 8;
  parms.taskfile.atapi_cntlow = size & 0xff;
  parms.taskfile.cmd = holy_ATA_CMD_PACKET;
  parms.cmd = cmd;
  parms.cmdsize = cmdsize;

  parms.size = size;
  parms.buffer = buf;
  
  err = dev->dev->readwrite (dev, &parms, 0);
  if (err)
    return err;

  if (parms.size != size)
    return holy_error (holy_ERR_READ_ERROR, "incomplete ATAPI read");
  return holy_ERR_NONE;
}

static holy_err_t
holy_atapi_write (struct holy_scsi *scsi __attribute__((unused)),
		  holy_size_t cmdsize __attribute__((unused)),
		  char *cmd __attribute__((unused)),
		  holy_size_t size __attribute__((unused)),
		  const char *buf __attribute__((unused)))
{
  // XXX: scsi.mod does not use write yet.
  return holy_error (holy_ERR_NOT_IMPLEMENTED_YET, "ATAPI write not implemented");
}

static holy_err_t
holy_atapi_open (int id, int bus, struct holy_scsi *scsi)
{
  struct holy_ata *ata;

  ata = holy_ata_real_open (id, bus);
  if (!ata)
    return holy_errno;
    
  if (! ata->atapi)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "no such ATAPI device");

  scsi->data = ata;
  scsi->luns = 1;

  return holy_ERR_NONE;
}

/* Context for holy_atapi_iterate.  */
struct holy_atapi_iterate_ctx
{
  holy_scsi_dev_iterate_hook_t hook;
  void *hook_data;
};

/* Helper for holy_atapi_iterate.  */
static int
holy_atapi_iterate_iter (int id, int bus, void *data)
{
  struct holy_atapi_iterate_ctx *ctx = data;
  struct holy_ata *ata;
  int ret;

  ata = holy_ata_real_open (id, bus);

  if (!ata)
    {
      holy_errno = holy_ERR_NONE;
      return 0;
    }
  if (!ata->atapi)
    {
      holy_ata_real_close (ata);
      return 0;
    }
  ret = ctx->hook (id, bus, 1, ctx->hook_data);
  holy_ata_real_close (ata);
  return ret;
}

static int
holy_atapi_iterate (holy_scsi_dev_iterate_hook_t hook, void *hook_data,
		    holy_disk_pull_t pull)
{
  struct holy_atapi_iterate_ctx ctx = { hook, hook_data };
  holy_ata_dev_t p;
  
  for (p = holy_ata_dev_list; p; p = p->next)
    if (p->iterate && p->iterate (holy_atapi_iterate_iter, &ctx, pull))
      return 1;
  return 0;
}

static void
holy_atapi_close (holy_scsi_t disk)
{
  struct holy_ata *ata = disk->data;
  holy_ata_real_close (ata);
}


void
holy_ata_dev_register (holy_ata_dev_t dev)
{
  dev->next = holy_ata_dev_list;
  holy_ata_dev_list = dev;
}

void
holy_ata_dev_unregister (holy_ata_dev_t dev)
{
  holy_ata_dev_t *p, q;

  for (p = &holy_ata_dev_list, q = *p; q; p = &(q->next), q = q->next)
    if (q == dev)
      {
        *p = q->next;
	break;
      }
}

static struct holy_scsi_dev holy_atapi_dev =
  {
    .iterate = holy_atapi_iterate,
    .open = holy_atapi_open,
    .close = holy_atapi_close,
    .read = holy_atapi_read,
    .write = holy_atapi_write,
    .next = 0
  };



holy_MOD_INIT(ata)
{
  holy_disk_dev_register (&holy_atadisk_dev);

  /* ATAPI devices are handled by scsi.mod.  */
  holy_scsi_dev_register (&holy_atapi_dev);
}

holy_MOD_FINI(ata)
{
  holy_scsi_dev_unregister (&holy_atapi_dev);
  holy_disk_dev_unregister (&holy_atadisk_dev);
}
