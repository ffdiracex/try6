/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/disk.h>
#include <holy/dl.h>
#include <holy/kernel.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/types.h>
#include <holy/scsi.h>
#include <holy/scsicmd.h>
#include <holy/time.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");


static holy_scsi_dev_t holy_scsi_dev_list;

const char holy_scsi_names[holy_SCSI_NUM_SUBSYSTEMS][5] = {
  [holy_SCSI_SUBSYSTEM_USBMS] = "usb",
  [holy_SCSI_SUBSYSTEM_PATA] = "ata",
  [holy_SCSI_SUBSYSTEM_AHCI] = "ahci"
};

void
holy_scsi_dev_register (holy_scsi_dev_t dev)
{
  dev->next = holy_scsi_dev_list;
  holy_scsi_dev_list = dev;
}

void
holy_scsi_dev_unregister (holy_scsi_dev_t dev)
{
  holy_scsi_dev_t *p, q;

  for (p = &holy_scsi_dev_list, q = *p; q; p = &(q->next), q = q->next)
    if (q == dev)
      {
        *p = q->next;
	break;
      }
}


/* Check result of previous operation.  */
static holy_err_t
holy_scsi_request_sense (holy_scsi_t scsi)
{
  struct holy_scsi_request_sense rs;
  struct holy_scsi_request_sense_data rsd;
  holy_err_t err;

  rs.opcode = holy_scsi_cmd_request_sense;
  rs.lun = scsi->lun << holy_SCSI_LUN_SHIFT;
  rs.reserved1 = 0;
  rs.reserved2 = 0;
  rs.alloc_length = 0x12; /* XXX: Hardcoded for now */
  rs.control = 0;
  holy_memset (rs.pad, 0, sizeof(rs.pad));

  err = scsi->dev->read (scsi, sizeof (rs), (char *) &rs,
			 sizeof (rsd), (char *) &rsd);
  if (err)
    return err;

  return holy_ERR_NONE;
}
/* Self commenting... */
static holy_err_t
holy_scsi_test_unit_ready (holy_scsi_t scsi)
{
  struct holy_scsi_test_unit_ready tur;
  holy_err_t err;
  holy_err_t err_sense;
  
  tur.opcode = holy_scsi_cmd_test_unit_ready;
  tur.lun = scsi->lun << holy_SCSI_LUN_SHIFT;
  tur.reserved1 = 0;
  tur.reserved2 = 0;
  tur.reserved3 = 0;
  tur.control = 0;
  holy_memset (tur.pad, 0, sizeof(tur.pad));

  err = scsi->dev->read (scsi, sizeof (tur), (char *) &tur,
			 0, NULL);

  /* Each SCSI command should be followed by Request Sense.
     If not so, many devices STALLs or definitely freezes. */
  err_sense = holy_scsi_request_sense (scsi);
  if (err_sense != holy_ERR_NONE)
  	holy_errno = err;
  /* err_sense is ignored for now and Request Sense Data also... */
  
  if (err)
    return err;

  return holy_ERR_NONE;
}

/* Determine if the device is removable and the type of the device
   SCSI.  */
static holy_err_t
holy_scsi_inquiry (holy_scsi_t scsi)
{
  struct holy_scsi_inquiry iq;
  struct holy_scsi_inquiry_data iqd;
  holy_err_t err;
  holy_err_t err_sense;

  iq.opcode = holy_scsi_cmd_inquiry;
  iq.lun = scsi->lun << holy_SCSI_LUN_SHIFT;
  iq.page = 0;
  iq.reserved = 0;
  iq.alloc_length = 0x24; /* XXX: Hardcoded for now */
  iq.control = 0;
  holy_memset (iq.pad, 0, sizeof(iq.pad));

  err = scsi->dev->read (scsi, sizeof (iq), (char *) &iq,
			 sizeof (iqd), (char *) &iqd);

  /* Each SCSI command should be followed by Request Sense.
     If not so, many devices STALLs or definitely freezes. */
  err_sense = holy_scsi_request_sense (scsi);
  if (err_sense != holy_ERR_NONE)
  	holy_errno = err;
  /* err_sense is ignored for now and Request Sense Data also... */

  if (err)
    return err;

  scsi->devtype = iqd.devtype & holy_SCSI_DEVTYPE_MASK;
  scsi->removable = iqd.rmb >> holy_SCSI_REMOVABLE_BIT;

  return holy_ERR_NONE;
}

/* Read the capacity and block size of SCSI.  */
static holy_err_t
holy_scsi_read_capacity10 (holy_scsi_t scsi)
{
  struct holy_scsi_read_capacity10 rc;
  struct holy_scsi_read_capacity10_data rcd;
  holy_err_t err;
  holy_err_t err_sense;

  rc.opcode = holy_scsi_cmd_read_capacity10;
  rc.lun = scsi->lun << holy_SCSI_LUN_SHIFT;
  rc.logical_block_addr = 0;
  rc.reserved1 = 0;
  rc.reserved2 = 0;
  rc.PMI = 0;
  rc.control = 0;
  rc.pad = 0;
	
  err = scsi->dev->read (scsi, sizeof (rc), (char *) &rc,
			 sizeof (rcd), (char *) &rcd);

  /* Each SCSI command should be followed by Request Sense.
     If not so, many devices STALLs or definitely freezes. */
  err_sense = holy_scsi_request_sense (scsi);
  if (err_sense != holy_ERR_NONE)
  	holy_errno = err;
/* err_sense is ignored for now and Request Sense Data also... */

  if (err)
    return err;

  scsi->last_block = holy_be_to_cpu32 (rcd.last_block);
  scsi->blocksize = holy_be_to_cpu32 (rcd.blocksize);

  return holy_ERR_NONE;
}

/* Read the capacity and block size of SCSI.  */
static holy_err_t
holy_scsi_read_capacity16 (holy_scsi_t scsi)
{
  struct holy_scsi_read_capacity16 rc;
  struct holy_scsi_read_capacity16_data rcd;
  holy_err_t err;
  holy_err_t err_sense;

  rc.opcode = holy_scsi_cmd_read_capacity16;
  rc.lun = (scsi->lun << holy_SCSI_LUN_SHIFT) | 0x10;
  rc.logical_block_addr = 0;
  rc.alloc_len = holy_cpu_to_be32_compile_time (sizeof (rcd));
  rc.PMI = 0;
  rc.control = 0;
	
  err = scsi->dev->read (scsi, sizeof (rc), (char *) &rc,
			 sizeof (rcd), (char *) &rcd);

  /* Each SCSI command should be followed by Request Sense.
     If not so, many devices STALLs or definitely freezes. */
  err_sense = holy_scsi_request_sense (scsi);
  if (err_sense != holy_ERR_NONE)
  	holy_errno = err;
/* err_sense is ignored for now and Request Sense Data also... */

  if (err)
    return err;

  scsi->last_block = holy_be_to_cpu64 (rcd.last_block);
  scsi->blocksize = holy_be_to_cpu32 (rcd.blocksize);

  return holy_ERR_NONE;
}

/* Send a SCSI request for DISK: read SIZE sectors starting with
   sector SECTOR to BUF.  */
static holy_err_t
holy_scsi_read10 (holy_disk_t disk, holy_disk_addr_t sector,
		  holy_size_t size, char *buf)
{
  holy_scsi_t scsi;
  struct holy_scsi_read10 rd;
  holy_err_t err;
  holy_err_t err_sense;

  scsi = disk->data;

  rd.opcode = holy_scsi_cmd_read10;
  rd.lun = scsi->lun << holy_SCSI_LUN_SHIFT;
  rd.lba = holy_cpu_to_be32 (sector);
  rd.reserved = 0;
  rd.size = holy_cpu_to_be16 (size);
  rd.reserved2 = 0;
  rd.pad = 0;

  err = scsi->dev->read (scsi, sizeof (rd), (char *) &rd, size * scsi->blocksize, buf);

  /* Each SCSI command should be followed by Request Sense.
     If not so, many devices STALLs or definitely freezes. */
  err_sense = holy_scsi_request_sense (scsi);
  if (err_sense != holy_ERR_NONE)
  	holy_errno = err;
  /* err_sense is ignored for now and Request Sense Data also... */

  return err;
}

/* Send a SCSI request for DISK: read SIZE sectors starting with
   sector SECTOR to BUF.  */
static holy_err_t
holy_scsi_read12 (holy_disk_t disk, holy_disk_addr_t sector,
		  holy_size_t size, char *buf)
{
  holy_scsi_t scsi;
  struct holy_scsi_read12 rd;
  holy_err_t err;
  holy_err_t err_sense;

  scsi = disk->data;

  rd.opcode = holy_scsi_cmd_read12;
  rd.lun = scsi->lun << holy_SCSI_LUN_SHIFT;
  rd.lba = holy_cpu_to_be32 (sector);
  rd.size = holy_cpu_to_be32 (size);
  rd.reserved = 0;
  rd.control = 0;

  err = scsi->dev->read (scsi, sizeof (rd), (char *) &rd, size * scsi->blocksize, buf);

  /* Each SCSI command should be followed by Request Sense.
     If not so, many devices STALLs or definitely freezes. */
  err_sense = holy_scsi_request_sense (scsi);
  if (err_sense != holy_ERR_NONE)
  	holy_errno = err;
  /* err_sense is ignored for now and Request Sense Data also... */

  return err;
}

/* Send a SCSI request for DISK: read SIZE sectors starting with
   sector SECTOR to BUF.  */
static holy_err_t
holy_scsi_read16 (holy_disk_t disk, holy_disk_addr_t sector,
		  holy_size_t size, char *buf)
{
  holy_scsi_t scsi;
  struct holy_scsi_read16 rd;
  holy_err_t err;
  holy_err_t err_sense;

  scsi = disk->data;

  rd.opcode = holy_scsi_cmd_read16;
  rd.lun = scsi->lun << holy_SCSI_LUN_SHIFT;
  rd.lba = holy_cpu_to_be64 (sector);
  rd.size = holy_cpu_to_be32 (size);
  rd.reserved = 0;
  rd.control = 0;

  err = scsi->dev->read (scsi, sizeof (rd), (char *) &rd, size * scsi->blocksize, buf);

  /* Each SCSI command should be followed by Request Sense.
     If not so, many devices STALLs or definitely freezes. */
  err_sense = holy_scsi_request_sense (scsi);
  if (err_sense != holy_ERR_NONE)
  	holy_errno = err;
  /* err_sense is ignored for now and Request Sense Data also... */

  return err;
}

/* Send a SCSI request for DISK: write the data stored in BUF to SIZE
   sectors starting with SECTOR.  */
static holy_err_t
holy_scsi_write10 (holy_disk_t disk, holy_disk_addr_t sector,
		   holy_size_t size, const char *buf)
{
  holy_scsi_t scsi;
  struct holy_scsi_write10 wr;
  holy_err_t err;
  holy_err_t err_sense;

  scsi = disk->data;

  wr.opcode = holy_scsi_cmd_write10;
  wr.lun = scsi->lun << holy_SCSI_LUN_SHIFT;
  wr.lba = holy_cpu_to_be32 (sector);
  wr.reserved = 0;
  wr.size = holy_cpu_to_be16 (size);
  wr.reserved2 = 0;
  wr.pad = 0;

  err = scsi->dev->write (scsi, sizeof (wr), (char *) &wr, size * scsi->blocksize, buf);

  /* Each SCSI command should be followed by Request Sense.
     If not so, many devices STALLs or definitely freezes. */
  err_sense = holy_scsi_request_sense (scsi);
  if (err_sense != holy_ERR_NONE)
  	holy_errno = err;
  /* err_sense is ignored for now and Request Sense Data also... */

  return err;
}

#if 0

/* Send a SCSI request for DISK: write the data stored in BUF to SIZE
   sectors starting with SECTOR.  */
static holy_err_t
holy_scsi_write12 (holy_disk_t disk, holy_disk_addr_t sector,
		   holy_size_t size, char *buf)
{
  holy_scsi_t scsi;
  struct holy_scsi_write12 wr;
  holy_err_t err;
  holy_err_t err_sense;

  scsi = disk->data;

  wr.opcode = holy_scsi_cmd_write12;
  wr.lun = scsi->lun << holy_SCSI_LUN_SHIFT;
  wr.lba = holy_cpu_to_be32 (sector);
  wr.size = holy_cpu_to_be32 (size);
  wr.reserved = 0;
  wr.control = 0;

  err = scsi->dev->write (scsi, sizeof (wr), (char *) &wr, size * scsi->blocksize, buf);

  /* Each SCSI command should be followed by Request Sense.
     If not so, many devices STALLs or definitely freezes. */
  err_sense = holy_scsi_request_sense (scsi);
  if (err_sense != holy_ERR_NONE)
  	holy_errno = err;
  /* err_sense is ignored for now and Request Sense Data also... */

  return err;
}
#endif

/* Send a SCSI request for DISK: write the data stored in BUF to SIZE
   sectors starting with SECTOR.  */
static holy_err_t
holy_scsi_write16 (holy_disk_t disk, holy_disk_addr_t sector,
		   holy_size_t size, const char *buf)
{
  holy_scsi_t scsi;
  struct holy_scsi_write16 wr;
  holy_err_t err;
  holy_err_t err_sense;

  scsi = disk->data;

  wr.opcode = holy_scsi_cmd_write16;
  wr.lun = scsi->lun << holy_SCSI_LUN_SHIFT;
  wr.lba = holy_cpu_to_be64 (sector);
  wr.size = holy_cpu_to_be32 (size);
  wr.reserved = 0;
  wr.control = 0;

  err = scsi->dev->write (scsi, sizeof (wr), (char *) &wr, size * scsi->blocksize, buf);

  /* Each SCSI command should be followed by Request Sense.
     If not so, many devices STALLs or definitely freezes. */
  err_sense = holy_scsi_request_sense (scsi);
  if (err_sense != holy_ERR_NONE)
  	holy_errno = err;
  /* err_sense is ignored for now and Request Sense Data also... */

  return err;
}



/* Context for holy_scsi_iterate.  */
struct holy_scsi_iterate_ctx
{
  holy_disk_dev_iterate_hook_t hook;
  void *hook_data;
};

/* Helper for holy_scsi_iterate.  */
static int
scsi_iterate (int id, int bus, int luns, void *data)
{
  struct holy_scsi_iterate_ctx *ctx = data;
  int i;

  /* In case of a single LUN, just return `usbX'.  */
  if (luns == 1)
    {
      char *sname;
      int ret;
      sname = holy_xasprintf ("%s%d", holy_scsi_names[id], bus);
      if (!sname)
	return 1;
      ret = ctx->hook (sname, ctx->hook_data);
      holy_free (sname);
      return ret;
    }

  /* In case of multiple LUNs, every LUN will get a prefix to
     distinguish it.  */
  for (i = 0; i < luns; i++)
    {
      char *sname;
      int ret;
      sname = holy_xasprintf ("%s%d%c", holy_scsi_names[id], bus, 'a' + i);
      if (!sname)
	return 1;
      ret = ctx->hook (sname, ctx->hook_data);
      holy_free (sname);
      if (ret)
	return 1;
    }
  return 0;
}

static int
holy_scsi_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data,
		   holy_disk_pull_t pull)
{
  struct holy_scsi_iterate_ctx ctx = { hook, hook_data };
  holy_scsi_dev_t p;

  for (p = holy_scsi_dev_list; p; p = p->next)
    if (p->iterate && (p->iterate) (scsi_iterate, &ctx, pull))
      return 1;

  return 0;
}

static holy_err_t
holy_scsi_open (const char *name, holy_disk_t disk)
{
  holy_scsi_dev_t p;
  holy_scsi_t scsi;
  holy_err_t err;
  int lun, bus;
  holy_uint64_t maxtime;
  const char *nameend;
  unsigned id;

  nameend = name + holy_strlen (name) - 1;
  /* Try to detect a LUN ('a'-'z'), otherwise just use the first
     LUN.  */
  if (nameend >= name && *nameend >= 'a' && *nameend <= 'z')
    {
      lun = *nameend - 'a';
      nameend--;
    }
  else
    lun = 0;

  while (nameend >= name && holy_isdigit (*nameend))
    nameend--;

  if (!nameend[1] || !holy_isdigit (nameend[1]))
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "not a SCSI disk");

  bus = holy_strtoul (nameend + 1, 0, 0);

  scsi = holy_malloc (sizeof (*scsi));
  if (! scsi)
    return holy_errno;

  for (id = 0; id < ARRAY_SIZE (holy_scsi_names); id++)
    if (holy_strncmp (holy_scsi_names[id], name, nameend - name) == 0)
      break;

  if (id == ARRAY_SIZE (holy_scsi_names))
    {
      holy_free (scsi);
      return holy_error (holy_ERR_UNKNOWN_DEVICE, "not a SCSI disk");
    }

  for (p = holy_scsi_dev_list; p; p = p->next)
    {
      if (p->open (id, bus, scsi))
	{
	  holy_errno = holy_ERR_NONE;
	  continue;
	}

      disk->id = holy_make_scsi_id (id, bus, lun);
      disk->data = scsi;
      scsi->dev = p;
      scsi->lun = lun;
      scsi->bus = bus;

      holy_dprintf ("scsi", "dev opened\n");

      err = holy_scsi_inquiry (scsi);
      if (err)
	{
	  holy_free (scsi);
	  holy_dprintf ("scsi", "inquiry failed\n");
	  return err;
	}

      holy_dprintf ("scsi", "inquiry: devtype=0x%02x removable=%d\n",
		    scsi->devtype, scsi->removable);

      /* Try to be conservative about the device types
	 supported.  */
      if (scsi->devtype != holy_scsi_devtype_direct
	  && scsi->devtype != holy_scsi_devtype_cdrom)
	{
	  holy_free (scsi);
	  return holy_error (holy_ERR_UNKNOWN_DEVICE,
			     "unknown SCSI device");
	}

      /* According to USB MS tests specification, issue Test Unit Ready
       * until OK */
      maxtime = holy_get_time_ms () + 5000; /* It is safer value */
      do
        {
	  /* Timeout is necessary - for example in case when we have
	   * universal card reader with more LUNs and we have only
	   * one card inserted (or none), so only one LUN (or none)
	   * will be ready - and we want not to hang... */
	  if (holy_get_time_ms () > maxtime)
            {
              err = holy_ERR_READ_ERROR;
              holy_free (scsi);
              holy_dprintf ("scsi", "LUN is not ready - timeout\n");
              return err;
            }
          err = holy_scsi_test_unit_ready (scsi);
        }
      while (err == holy_ERR_READ_ERROR);
      /* Reset holy_errno !
       * It is set to some error code in loop before... */
      holy_errno = holy_ERR_NONE;

      /* Read capacity of media */
      err = holy_scsi_read_capacity10 (scsi);
      if (err)
	{
	  holy_free (scsi);
	  holy_dprintf ("scsi", "READ CAPACITY10 failed\n");
	  return err;
	}

      if (scsi->last_block == 0xffffffff)
	{
	  err = holy_scsi_read_capacity16 (scsi);
	  if (err)
	    {
	      holy_free (scsi);
	      holy_dprintf ("scsi", "READ CAPACITY16 failed\n");
	      return err;
	    }
	}

      disk->total_sectors = scsi->last_block + 1;
      /* PATA doesn't support more than 32K reads.
	 Not sure about AHCI and USB. If it's confirmed that either of
	 them can do bigger reads reliably this value can be moved to 'scsi'
	 structure.  */
      disk->max_agglomerate = 32768 >> (holy_DISK_SECTOR_BITS
					+ holy_DISK_CACHE_BITS);

      if (scsi->blocksize & (scsi->blocksize - 1) || !scsi->blocksize)
	{
	  holy_error (holy_ERR_IO, "invalid sector size %d",
		      scsi->blocksize);
	  holy_free (scsi);
	  return holy_errno;
	}
      for (disk->log_sector_size = 0;
	   (1U << disk->log_sector_size) < scsi->blocksize;
	   disk->log_sector_size++);

      holy_dprintf ("scsi", "last_block=%" PRIuholy_UINT64_T ", blocksize=%u\n",
		    scsi->last_block, scsi->blocksize);
      holy_dprintf ("scsi", "Disk total sectors = %llu\n",
		    (unsigned long long) disk->total_sectors);

      return holy_ERR_NONE;
    }

  holy_free (scsi);
  return holy_error (holy_ERR_UNKNOWN_DEVICE, "not a SCSI disk");
}

static void
holy_scsi_close (holy_disk_t disk)
{
  holy_scsi_t scsi;

  scsi = disk->data;
  if (scsi->dev->close)
    scsi->dev->close (scsi);
  holy_free (scsi);
}

static holy_err_t
holy_scsi_read (holy_disk_t disk, holy_disk_addr_t sector,
		holy_size_t size, char *buf)
{
  holy_scsi_t scsi;

  scsi = disk->data;

  holy_err_t err;
  /* Depending on the type, select a read function.  */
  switch (scsi->devtype)
    {
    case holy_scsi_devtype_direct:
      if (sector >> 32)
	err = holy_scsi_read16 (disk, sector, size, buf);
      else
	err = holy_scsi_read10 (disk, sector, size, buf);
      if (err)
	return err;
      break;

    case holy_scsi_devtype_cdrom:
      if (sector >> 32)
	err = holy_scsi_read16 (disk, sector, size, buf);
      else
	err = holy_scsi_read12 (disk, sector, size, buf);
      if (err)
	return err;
      break;
    }

  return holy_ERR_NONE;

#if 0 /* Workaround - it works - but very slowly, from some reason
       * unknown to me (specially on OHCI). Do not use it. */
  /* Split transfer requests to device sector size because */
  /* some devices are not able to transfer more than 512-1024 bytes */
  holy_err_t err = holy_ERR_NONE;

  for ( ; size; size--)
    {
      /* Depending on the type, select a read function.  */
      switch (scsi->devtype)
        {
          case holy_scsi_devtype_direct:
            err = holy_scsi_read10 (disk, sector, 1, buf);
            break;

          case holy_scsi_devtype_cdrom:
            err = holy_scsi_read12 (disk, sector, 1, buf);
            break;

          default: /* This should not happen */
            return holy_ERR_READ_ERROR;
        }
      if (err)
        return err;
      sector++;
      buf += scsi->blocksize;
    }

  return err;
#endif
}

static holy_err_t
holy_scsi_write (holy_disk_t disk,
		 holy_disk_addr_t sector,
		 holy_size_t size,
		 const char *buf)
{
  holy_scsi_t scsi;

  scsi = disk->data;

  if (scsi->devtype == holy_scsi_devtype_cdrom)
    return holy_error (holy_ERR_IO, N_("cannot write to CD-ROM"));

  holy_err_t err;
  /* Depending on the type, select a read function.  */
  switch (scsi->devtype)
    {
    case holy_scsi_devtype_direct:
      if (sector >> 32)
	err = holy_scsi_write16 (disk, sector, size, buf);
      else
	err = holy_scsi_write10 (disk, sector, size, buf);
      if (err)
	return err;
      break;
    }

  return holy_ERR_NONE;
}


static struct holy_disk_dev holy_scsi_dev =
  {
    .name = "scsi",
    .id = holy_DISK_DEVICE_SCSI_ID,
    .iterate = holy_scsi_iterate,
    .open = holy_scsi_open,
    .close = holy_scsi_close,
    .read = holy_scsi_read,
    .write = holy_scsi_write,
    .next = 0
  };

holy_MOD_INIT(scsi)
{
  holy_disk_dev_register (&holy_scsi_dev);
}

holy_MOD_FINI(scsi)
{
  holy_disk_dev_unregister (&holy_scsi_dev);
}
