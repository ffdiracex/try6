/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/machine/biosdisk.h>
#include <holy/machine/kernel.h>
#include <holy/machine/memory.h>
#include <holy/machine/int.h>
#include <holy/disk.h>
#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/types.h>
#include <holy/misc.h>
#include <holy/err.h>
#include <holy/term.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static int cd_drive = 0;
static int holy_biosdisk_rw_int13_extensions (int ah, int drive, void *dap);

static int holy_biosdisk_get_num_floppies (void)
{
  struct holy_bios_int_registers regs;
  int drive;

  /* reset the disk system first */
  regs.eax = 0;
  regs.edx = 0;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;

  holy_bios_interrupt (0x13, &regs);

  for (drive = 0; drive < 2; drive++)
    {
      regs.flags = holy_CPU_INT_FLAGS_DEFAULT | holy_CPU_INT_FLAGS_CARRY;
      regs.edx = drive;

      /* call GET DISK TYPE */
      regs.eax = 0x1500;
      holy_bios_interrupt (0x13, &regs);
      if (regs.flags & holy_CPU_INT_FLAGS_CARRY)
	break;

      /* check if this drive exists */
      if (!(regs.eax & 0x300))
	break;
    }

  return drive;
}

/*
 *   Call IBM/MS INT13 Extensions (int 13 %ah=AH) for DRIVE. DAP
 *   is passed for disk address packet. If an error occurs, return
 *   non-zero, otherwise zero.
 */

static int 
holy_biosdisk_rw_int13_extensions (int ah, int drive, void *dap)
{
  struct holy_bios_int_registers regs;
  regs.eax = ah << 8;
  /* compute the address of disk_address_packet */
  regs.ds = (((holy_addr_t) dap) & 0xffff0000) >> 4;
  regs.esi = (((holy_addr_t) dap) & 0xffff);
  regs.edx = drive;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;

  holy_bios_interrupt (0x13, &regs);
  return (regs.eax >> 8) & 0xff;
}

/*
 *   Call standard and old INT13 (int 13 %ah=AH) for DRIVE. Read/write
 *   NSEC sectors from COFF/HOFF/SOFF into SEGMENT. If an error occurs,
 *   return non-zero, otherwise zero.
 */
static int 
holy_biosdisk_rw_standard (int ah, int drive, int coff, int hoff,
			   int soff, int nsec, int segment)
{
  int ret, i;

  /* Try 3 times.  */
  for (i = 0; i < 3; i++)
    {
      struct holy_bios_int_registers regs;

      /* set up CHS information */
      /* set %ch to low eight bits of cylinder */
      regs.ecx = (coff << 8) & 0xff00;
      /* set bits 6-7 of %cl to high two bits of cylinder */
      regs.ecx |= (coff >> 2) & 0xc0;
      /* set bits 0-5 of %cl to sector */
      regs.ecx |= soff & 0x3f;

      /* set %dh to head and %dl to drive */  
      regs.edx = (drive & 0xff) | ((hoff << 8) & 0xff00);
      /* set %ah to AH */
      regs.eax = (ah << 8) & 0xff00;
      /* set %al to NSEC */
      regs.eax |= nsec & 0xff;

      regs.ebx = 0;
      regs.es = segment;

      regs.flags = holy_CPU_INT_FLAGS_DEFAULT;

      holy_bios_interrupt (0x13, &regs);
      /* check if successful */
      if (!(regs.flags & holy_CPU_INT_FLAGS_CARRY))
	return 0;

      /* save return value */
      ret = regs.eax >> 8;

      /* if fail, reset the disk system */
      regs.eax = 0;
      regs.edx = (drive & 0xff);
      regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
      holy_bios_interrupt (0x13, &regs);
    }
  return ret;
}

/*
 *   Check if LBA is supported for DRIVE. If it is supported, then return
 *   the major version of extensions, otherwise zero.
 */
static int
holy_biosdisk_check_int13_extensions (int drive)
{
  struct holy_bios_int_registers regs;

  regs.edx = drive & 0xff;
  regs.eax = 0x4100;
  regs.ebx = 0x55aa;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x13, &regs);
  
  if (regs.flags & holy_CPU_INT_FLAGS_CARRY)
    return 0;

  if ((regs.ebx & 0xffff) != 0xaa55)
    return 0;

  /* check if AH=0x42 is supported */
  if (!(regs.ecx & 1))
    return 0;

  return (regs.eax >> 8) & 0xff;
}

/*
 *   Return the geometry of DRIVE in CYLINDERS, HEADS and SECTORS. If an
 *   error occurs, then return non-zero, otherwise zero.
 */
static int 
holy_biosdisk_get_diskinfo_standard (int drive,
				     unsigned long *cylinders,
				     unsigned long *heads,
				     unsigned long *sectors)
{
  struct holy_bios_int_registers regs;

  regs.eax = 0x0800;
  regs.edx = drive & 0xff;

  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x13, &regs);

  /* Check if unsuccessful. Ignore return value if carry isn't set to 
     workaround some buggy BIOSes. */
  if ((regs.flags & holy_CPU_INT_FLAGS_CARRY) && ((regs.eax & 0xff00) != 0))
    return (regs.eax & 0xff00) >> 8;

  /* bogus BIOSes may not return an error number */  
  /* 0 sectors means no disk */
  if (!(regs.ecx & 0x3f))
    /* XXX 0x60 is one of the unused error numbers */
    return 0x60;

  /* the number of heads is counted from zero */
  *heads = ((regs.edx >> 8) & 0xff) + 1;
  *cylinders = (((regs.ecx >> 8) & 0xff) | ((regs.ecx << 2) & 0x0300)) + 1;
  *sectors = regs.ecx & 0x3f;
  return 0;
}

static int
holy_biosdisk_get_diskinfo_real (int drive, void *drp, holy_uint16_t ax)
{
  struct holy_bios_int_registers regs;

  regs.eax = ax;

  /* compute the address of drive parameters */
  regs.esi = ((holy_addr_t) drp) & 0xf;
  regs.ds = ((holy_addr_t) drp) >> 4;
  regs.edx = drive & 0xff;

  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x13, &regs);

  /* Check if unsuccessful. Ignore return value if carry isn't set to 
     workaround some buggy BIOSes. */
  if ((regs.flags & holy_CPU_INT_FLAGS_CARRY) && ((regs.eax & 0xff00) != 0))
    return (regs.eax & 0xff00) >> 8;

  return 0;
}

/*
 *   Return the cdrom information of DRIVE in CDRP. If an error occurs,
 *   then return non-zero, otherwise zero.
 */
static int
holy_biosdisk_get_cdinfo_int13_extensions (int drive, void *cdrp)
{
  return holy_biosdisk_get_diskinfo_real (drive, cdrp, 0x4b01);
}

/*
 *   Return the geometry of DRIVE in a drive parameters, DRP. If an error
 *   occurs, then return non-zero, otherwise zero.
 */
static int
holy_biosdisk_get_diskinfo_int13_extensions (int drive, void *drp)
{
  return holy_biosdisk_get_diskinfo_real (drive, drp, 0x4800);
}

static int
holy_biosdisk_get_drive (const char *name)
{
  unsigned long drive;

  if (name[0] == 'c' && name[1] == 'd' && name[2] == 0 && cd_drive)
    return cd_drive;

  if ((name[0] != 'f' && name[0] != 'h') || name[1] != 'd')
    goto fail;

  drive = holy_strtoul (name + 2, 0, 10);
  if (holy_errno != holy_ERR_NONE)
    goto fail;

  if (name[0] == 'h')
    drive += 0x80;

  return (int) drive ;

 fail:
  holy_error (holy_ERR_UNKNOWN_DEVICE, "not a biosdisk");
  return -1;
}

static int
holy_biosdisk_call_hook (holy_disk_dev_iterate_hook_t hook, void *hook_data,
			 int drive)
{
  char name[10];

  if (cd_drive && drive == cd_drive)
    {
      holy_dprintf ("biosdisk", "iterating cd\n");
      return hook ("cd", hook_data);
    }

  holy_snprintf (name, sizeof (name),
		 (drive & 0x80) ? "hd%d" : "fd%d", drive & (~0x80));
  holy_dprintf ("biosdisk", "iterating %s\n", name);
  return hook (name, hook_data);
}

static int
holy_biosdisk_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data,
		       holy_disk_pull_t pull)
{
  int num_floppies;
  int drive;

  /* For hard disks, attempt to read the MBR.  */
  switch (pull)
    {
    case holy_DISK_PULL_NONE:
      for (drive = 0x80; drive < 0x90; drive++)
	{
	  if (holy_biosdisk_rw_standard (0x02, drive, 0, 0, 1, 1,
					 holy_MEMORY_MACHINE_SCRATCH_SEG) != 0)
	    {
	      holy_dprintf ("biosdisk", "Read error when probing drive 0x%2x\n", drive);
	      break;
	    }

	  if (holy_biosdisk_call_hook (hook, hook_data, drive))
	    return 1;
	}
      return 0;

    case holy_DISK_PULL_REMOVABLE:
      if (cd_drive)
	{
	  if (holy_biosdisk_call_hook (hook, hook_data, cd_drive))
	    return 1;
	}

      /* For floppy disks, we can get the number safely.  */
      num_floppies = holy_biosdisk_get_num_floppies ();
      for (drive = 0; drive < num_floppies; drive++)
	if (holy_biosdisk_call_hook (hook, hook_data, drive))
	  return 1;
      return 0;
    default:
      return 0;
    }
  return 0;
}

static holy_err_t
holy_biosdisk_open (const char *name, holy_disk_t disk)
{
  holy_uint64_t total_sectors = 0;
  int drive;
  struct holy_biosdisk_data *data;

  holy_dprintf ("biosdisk", "opening %s\n", name);

  drive = holy_biosdisk_get_drive (name);
  if (drive < 0)
    return holy_errno;

  disk->id = drive;

  data = (struct holy_biosdisk_data *) holy_zalloc (sizeof (*data));
  if (! data)
    return holy_errno;

  data->drive = drive;

  if ((cd_drive) && (drive == cd_drive))
    {
      data->flags = holy_BIOSDISK_FLAG_LBA | holy_BIOSDISK_FLAG_CDROM;
      data->sectors = 8;
      disk->log_sector_size = 11;
      /* TODO: get the correct size.  */
      total_sectors = holy_DISK_SIZE_UNKNOWN;
    }
  else
    {
      /* HDD */
      int version;

      disk->log_sector_size = 9;

      version = holy_biosdisk_check_int13_extensions (drive);
      if (version)
	{
	  struct holy_biosdisk_drp *drp
	    = (struct holy_biosdisk_drp *) holy_MEMORY_MACHINE_SCRATCH_ADDR;

	  /* Clear out the DRP.  */
	  holy_memset (drp, 0, sizeof (*drp));
	  drp->size = sizeof (*drp);
	  if (! holy_biosdisk_get_diskinfo_int13_extensions (drive, drp))
	    {
	      data->flags = holy_BIOSDISK_FLAG_LBA;

	      if (drp->total_sectors)
		total_sectors = drp->total_sectors;
	      else
                /* Some buggy BIOSes doesn't return the total sectors
                   correctly but returns zero. So if it is zero, compute
                   it by C/H/S returned by the LBA BIOS call.  */
                total_sectors = ((holy_uint64_t) drp->cylinders)
		  * drp->heads * drp->sectors;
	      if (drp->bytes_per_sector
		  && !(drp->bytes_per_sector & (drp->bytes_per_sector - 1))
		  && drp->bytes_per_sector >= 512
		  && drp->bytes_per_sector <= 16384)
		{
		  for (disk->log_sector_size = 0;
		       (1 << disk->log_sector_size) < drp->bytes_per_sector;
		       disk->log_sector_size++);
		}

	      holy_dprintf ("biosdisk",
			    "LBA total = 0x%llx block size = 0x%lx\n",
			    (unsigned long long) total_sectors,
			    1L << disk->log_sector_size);
	    }
	}
    }

  if (! (data->flags & holy_BIOSDISK_FLAG_CDROM))
    {
      if (holy_biosdisk_get_diskinfo_standard (drive,
					       &data->cylinders,
					       &data->heads,
					       &data->sectors) != 0)
        {
	  if (total_sectors && (data->flags & holy_BIOSDISK_FLAG_LBA))
	    {
	      data->sectors = 63;
	      data->heads = 255;
	      data->cylinders
		= holy_divmod64 (total_sectors
				 + data->heads * data->sectors - 1,
				 data->heads * data->sectors, 0);
	    }
	  else
	    {
	      holy_free (data);
	      return holy_error (holy_ERR_BAD_DEVICE, "%s cannot get C/H/S values", disk->name);
	    }
        }

      if (data->sectors == 0)
	data->sectors = 63;
      if (data->heads == 0)
	data->heads = 255;

      if (! total_sectors)
        total_sectors = ((holy_uint64_t) data->cylinders)
	  * data->heads * data->sectors;

      holy_dprintf ("biosdisk", "C/H/S %lu/%lu/%lu\n",
		    data->cylinders, data->heads, data->sectors);
    }

  disk->total_sectors = total_sectors;
  /* Limit the max to 0x7f because of Phoenix EDD.  */
  disk->max_agglomerate = 0x7f >> holy_DISK_CACHE_BITS;
  COMPILE_TIME_ASSERT ((0x7f >> holy_DISK_CACHE_BITS
			<< (holy_DISK_SECTOR_BITS + holy_DISK_CACHE_BITS))
		       + sizeof (struct holy_biosdisk_dap)
		       < holy_MEMORY_MACHINE_SCRATCH_SIZE);

  disk->data = data;

  holy_dprintf ("biosdisk", "opening %s succeeded\n", name);

  return holy_ERR_NONE;
}

static void
holy_biosdisk_close (holy_disk_t disk)
{
  holy_dprintf ("biosdisk", "closing %s\n", disk->name);
  holy_free (disk->data);
}

/* For readability.  */
#define holy_BIOSDISK_READ	0
#define holy_BIOSDISK_WRITE	1

#define holy_BIOSDISK_CDROM_RETRY_COUNT 3

static holy_err_t
holy_biosdisk_rw (int cmd, holy_disk_t disk,
		  holy_disk_addr_t sector, holy_size_t size,
		  unsigned segment)
{
  struct holy_biosdisk_data *data = disk->data;

  /* VirtualBox fails with sectors above 2T on CDs.
     Since even BD-ROMS are never that big anyway, return error.  */
  if ((data->flags & holy_BIOSDISK_FLAG_CDROM)
      && (sector >> 32))
    return holy_error (holy_ERR_OUT_OF_RANGE,
		       N_("attempt to read or write outside of disk `%s'"),
		       disk->name);

  if (data->flags & holy_BIOSDISK_FLAG_LBA)
    {
      struct holy_biosdisk_dap *dap;

      dap = (struct holy_biosdisk_dap *) (holy_MEMORY_MACHINE_SCRATCH_ADDR
					  + (data->sectors
					     << disk->log_sector_size));
      dap->length = sizeof (*dap);
      dap->reserved = 0;
      dap->blocks = size;
      dap->buffer = segment << 16;	/* The format SEGMENT:ADDRESS.  */
      dap->block = sector;

      if (data->flags & holy_BIOSDISK_FLAG_CDROM)
        {
	  int i;

	  if (cmd)
	    return holy_error (holy_ERR_WRITE_ERROR, N_("cannot write to CD-ROM"));

	  for (i = 0; i < holy_BIOSDISK_CDROM_RETRY_COUNT; i++)
            if (! holy_biosdisk_rw_int13_extensions (0x42, data->drive, dap))
	      break;

	  if (i == holy_BIOSDISK_CDROM_RETRY_COUNT)
	    return holy_error (holy_ERR_READ_ERROR, N_("failure reading sector 0x%llx "
						       "from `%s'"),
			       (unsigned long long) sector,
			       disk->name);
	}
      else
        if (holy_biosdisk_rw_int13_extensions (cmd + 0x42, data->drive, dap))
	  {
	    /* Fall back to the CHS mode.  */
	    data->flags &= ~holy_BIOSDISK_FLAG_LBA;
	    disk->total_sectors = data->cylinders * data->heads * data->sectors;
	    return holy_biosdisk_rw (cmd, disk, sector, size, segment);
	  }
    }
  else
    {
      unsigned coff, hoff, soff;
      unsigned head;

      /* It is impossible to reach over 8064 MiB (a bit less than LBA24) with
	 the traditional CHS access.  */
      if (sector >
	  1024 /* cylinders */ *
	  256 /* heads */ *
	  63 /* spt */)
	return holy_error (holy_ERR_OUT_OF_RANGE,
			   N_("attempt to read or write outside of disk `%s'"),
			   disk->name);

      soff = ((holy_uint32_t) sector) % data->sectors + 1;
      head = ((holy_uint32_t) sector) / data->sectors;
      hoff = head % data->heads;
      coff = head / data->heads;

      if (coff >= data->cylinders)
	return holy_error (holy_ERR_OUT_OF_RANGE,
			   N_("attempt to read or write outside of disk `%s'"),
			   disk->name);

      if (holy_biosdisk_rw_standard (cmd + 0x02, data->drive,
				     coff, hoff, soff, size, segment))
	{
	  switch (cmd)
	    {
	    case holy_BIOSDISK_READ:
	      return holy_error (holy_ERR_READ_ERROR, N_("failure reading sector 0x%llx "
							 "from `%s'"),
				 (unsigned long long) sector,
				 disk->name);
	    case holy_BIOSDISK_WRITE:
	      return holy_error (holy_ERR_WRITE_ERROR, N_("failure writing sector 0x%llx "
							  "to `%s'"),
				 (unsigned long long) sector,
				 disk->name);
	    }
	}
    }

  return holy_ERR_NONE;
}

/* Return the number of sectors which can be read safely at a time.  */
static holy_size_t
get_safe_sectors (holy_disk_t disk, holy_disk_addr_t sector)
{
  holy_size_t size;
  holy_uint64_t offset;
  struct holy_biosdisk_data *data = disk->data;
  holy_uint32_t sectors = data->sectors;

  /* OFFSET = SECTOR % SECTORS */
  holy_divmod64 (sector, sectors, &offset);

  size = sectors - offset;

  return size;
}

static holy_err_t
holy_biosdisk_read (holy_disk_t disk, holy_disk_addr_t sector,
		    holy_size_t size, char *buf)
{
  holy_dprintf ("biosdisk", "reading 0x%lx sectors at 0x%llx from %s\n",
		(unsigned long) size, (unsigned long long) sector, disk->name);

  while (size)
    {
      holy_size_t len;

      len = get_safe_sectors (disk, sector);

      if (len > size)
	len = size;

      if (holy_biosdisk_rw (holy_BIOSDISK_READ, disk, sector, len,
			    holy_MEMORY_MACHINE_SCRATCH_SEG))
	return holy_errno;

      holy_memcpy (buf, (void *) holy_MEMORY_MACHINE_SCRATCH_ADDR,
		   len << disk->log_sector_size);

      buf += len << disk->log_sector_size;
      sector += len;
      size -= len;
    }

  return holy_errno;
}

static holy_err_t
holy_biosdisk_write (holy_disk_t disk, holy_disk_addr_t sector,
		     holy_size_t size, const char *buf)
{
  struct holy_biosdisk_data *data = disk->data;

  holy_dprintf ("biosdisk", "writing 0x%lx sectors at 0x%llx to %s\n",
		(unsigned long) size, (unsigned long long) sector, disk->name);

  if (data->flags & holy_BIOSDISK_FLAG_CDROM)
    return holy_error (holy_ERR_IO, N_("cannot write to CD-ROM"));

  while (size)
    {
      holy_size_t len;

      len = get_safe_sectors (disk, sector);
      if (len > size)
	len = size;

      holy_memcpy ((void *) holy_MEMORY_MACHINE_SCRATCH_ADDR, buf,
		   len << disk->log_sector_size);

      if (holy_biosdisk_rw (holy_BIOSDISK_WRITE, disk, sector, len,
			    holy_MEMORY_MACHINE_SCRATCH_SEG))
	return holy_errno;

      buf += len << disk->log_sector_size;
      sector += len;
      size -= len;
    }

  return holy_errno;
}

static struct holy_disk_dev holy_biosdisk_dev =
  {
    .name = "biosdisk",
    .id = holy_DISK_DEVICE_BIOSDISK_ID,
    .iterate = holy_biosdisk_iterate,
    .open = holy_biosdisk_open,
    .close = holy_biosdisk_close,
    .read = holy_biosdisk_read,
    .write = holy_biosdisk_write,
    .next = 0
  };

static void
holy_disk_biosdisk_fini (void)
{
  holy_disk_dev_unregister (&holy_biosdisk_dev);
}

holy_MOD_INIT(biosdisk)
{
  struct holy_biosdisk_cdrp *cdrp
    = (struct holy_biosdisk_cdrp *) holy_MEMORY_MACHINE_SCRATCH_ADDR;
  holy_uint8_t boot_drive;

  if (holy_disk_firmware_is_tainted)
    {
      holy_puts_ (N_("Native disk drivers are in use. "
		     "Refusing to use firmware disk interface."));
      return;
    }
  holy_disk_firmware_fini = holy_disk_biosdisk_fini;

  holy_memset (cdrp, 0, sizeof (*cdrp));
  cdrp->size = sizeof (*cdrp);
  cdrp->media_type = 0xFF;
  boot_drive = (holy_boot_device >> 24);
  if ((! holy_biosdisk_get_cdinfo_int13_extensions (boot_drive, cdrp))
      && ((cdrp->media_type & holy_BIOSDISK_CDTYPE_MASK)
	  == holy_BIOSDISK_CDTYPE_NO_EMUL))
    cd_drive = cdrp->drive_no;
  /* Since diskboot.S rejects devices over 0x90 it must be a CD booted with
     cdboot.S
   */
  if (boot_drive >= 0x90)
    cd_drive = boot_drive;

  holy_disk_dev_register (&holy_biosdisk_dev);
}

holy_MOD_FINI(biosdisk)
{
  holy_disk_biosdisk_fini ();
}
