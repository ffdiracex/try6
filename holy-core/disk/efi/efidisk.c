/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/disk.h>
#include <holy/partition.h>
#include <holy/mm.h>
#include <holy/types.h>
#include <holy/misc.h>
#include <holy/err.h>
#include <holy/term.h>
#include <holy/efi/api.h>
#include <holy/efi/efi.h>
#include <holy/efi/disk.h>

struct holy_efidisk_data
{
  holy_efi_handle_t handle;
  holy_efi_device_path_t *device_path;
  holy_efi_device_path_t *last_device_path;
  holy_efi_block_io_t *block_io;
  struct holy_efidisk_data *next;
};

/* GUID.  */
static holy_efi_guid_t block_io_guid = holy_EFI_BLOCK_IO_GUID;

static struct holy_efidisk_data *fd_devices;
static struct holy_efidisk_data *hd_devices;
static struct holy_efidisk_data *cd_devices;

static struct holy_efidisk_data *
make_devices (void)
{
  holy_efi_uintn_t num_handles;
  holy_efi_handle_t *handles;
  holy_efi_handle_t *handle;
  struct holy_efidisk_data *devices = 0;

  /* Find handles which support the disk io interface.  */
  handles = holy_efi_locate_handle (holy_EFI_BY_PROTOCOL, &block_io_guid,
				    0, &num_handles);
  if (! handles)
    return 0;

  /* Make a linked list of devices.  */
  for (handle = handles; num_handles--; handle++)
    {
      holy_efi_device_path_t *dp;
      holy_efi_device_path_t *ldp;
      struct holy_efidisk_data *d;
      holy_efi_block_io_t *bio;

      dp = holy_efi_get_device_path (*handle);
      if (! dp)
	continue;

      ldp = holy_efi_find_last_device_path (dp);
      if (! ldp)
	/* This is empty. Why?  */
	continue;

      bio = holy_efi_open_protocol (*handle, &block_io_guid,
				    holy_EFI_OPEN_PROTOCOL_GET_PROTOCOL);
      if (! bio)
	/* This should not happen... Why?  */
	continue;

      /* iPXE adds stub Block IO protocol to loaded image device handle. It is
         completely non-functional and simply returns an error for every method.
        So attempt to detect and skip it. Magic number is literal "iPXE" and
        check block size as well */
      /* FIXME: shoud we close it? We do not do it elsewhere */
      if (bio->media && bio->media->media_id == 0x69505845U &&
         bio->media->block_size == 1)
         continue;

      d = holy_malloc (sizeof (*d));
      if (! d)
	{
	  /* Uggh.  */
	  holy_free (handles);
	  while (devices)
	    {
	      d = devices->next;
	      holy_free (devices);
	      devices = d;
	    }
	  return 0;
	}

      d->handle = *handle;
      d->device_path = dp;
      d->last_device_path = ldp;
      d->block_io = bio;
      d->next = devices;
      devices = d;
    }

  holy_free (handles);

  return devices;
}

/* Find the parent device.  */
static struct holy_efidisk_data *
find_parent_device (struct holy_efidisk_data *devices,
		    struct holy_efidisk_data *d)
{
  holy_efi_device_path_t *dp, *ldp;
  struct holy_efidisk_data *parent;

  dp = holy_efi_duplicate_device_path (d->device_path);
  if (! dp)
    return 0;

  ldp = holy_efi_find_last_device_path (dp);
  ldp->type = holy_EFI_END_DEVICE_PATH_TYPE;
  ldp->subtype = holy_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;
  ldp->length = sizeof (*ldp);

  for (parent = devices; parent; parent = parent->next)
    {
      /* Ignore itself.  */
      if (parent == d)
	continue;

      if (holy_efi_compare_device_paths (parent->device_path, dp) == 0)
	break;
    }

  holy_free (dp);
  return parent;
}

static int
is_child (struct holy_efidisk_data *child,
	  struct holy_efidisk_data *parent)
{
  holy_efi_device_path_t *dp, *ldp;
  int ret;

  dp = holy_efi_duplicate_device_path (child->device_path);
  if (! dp)
    return 0;

  ldp = holy_efi_find_last_device_path (dp);
  ldp->type = holy_EFI_END_DEVICE_PATH_TYPE;
  ldp->subtype = holy_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;
  ldp->length = sizeof (*ldp);

  ret = (holy_efi_compare_device_paths (dp, parent->device_path) == 0);
  holy_free (dp);
  return ret;
}

#define FOR_CHILDREN(p, dev) for (p = dev; p; p = p->next) if (is_child (p, d))

/* Add a device into a list of devices in an ascending order.  */
static void
add_device (struct holy_efidisk_data **devices, struct holy_efidisk_data *d)
{
  struct holy_efidisk_data **p;
  struct holy_efidisk_data *n;

  for (p = devices; *p; p = &((*p)->next))
    {
      int ret;

      ret = holy_efi_compare_device_paths (holy_efi_find_last_device_path ((*p)->device_path),
					   holy_efi_find_last_device_path (d->device_path));
      if (ret == 0)
	ret = holy_efi_compare_device_paths ((*p)->device_path,
					     d->device_path);
      if (ret == 0)
	return;
      else if (ret > 0)
	break;
    }

  n = holy_malloc (sizeof (*n));
  if (! n)
    return;

  holy_memcpy (n, d, sizeof (*n));
  n->next = (*p);
  (*p) = n;
}

/* Name the devices.  */
static void
name_devices (struct holy_efidisk_data *devices)
{
  struct holy_efidisk_data *d;

  /* First, identify devices by media device paths.  */
  for (d = devices; d; d = d->next)
    {
      holy_efi_device_path_t *dp;

      dp = d->last_device_path;
      if (! dp)
	continue;

      if (holy_EFI_DEVICE_PATH_TYPE (dp) == holy_EFI_MEDIA_DEVICE_PATH_TYPE)
	{
	  int is_hard_drive = 0;

	  switch (holy_EFI_DEVICE_PATH_SUBTYPE (dp))
	    {
	    case holy_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE:
	      is_hard_drive = 1;
	      /* Intentionally fall through.  */
	    case holy_EFI_CDROM_DEVICE_PATH_SUBTYPE:
	      {
		struct holy_efidisk_data *parent, *parent2;

		parent = find_parent_device (devices, d);
		if (!parent)
		  {
#ifdef DEBUG_NAMES
		    holy_printf ("skipping orphaned partition: ");
		    holy_efi_print_device_path (d->device_path);
#endif
		    break;
		  }
		parent2 = find_parent_device (devices, parent);
		if (parent2)
		  {
#ifdef DEBUG_NAMES
		    holy_printf ("skipping subpartition: ");
		    holy_efi_print_device_path (d->device_path);
#endif
		    /* Mark itself as used.  */
		    d->last_device_path = 0;
		    break;
		  }
		if (!parent->last_device_path)
		  {
		    d->last_device_path = 0;
		    break;
		  }
		if (is_hard_drive)
		  {
#ifdef DEBUG_NAMES
		    holy_printf ("adding a hard drive by a partition: ");
		    holy_efi_print_device_path (parent->device_path);
#endif
		    add_device (&hd_devices, parent);
		  }
		else
		  {
#ifdef DEBUG_NAMES
		    holy_printf ("adding a cdrom by a partition: ");
		    holy_efi_print_device_path (parent->device_path);
#endif
		    add_device (&cd_devices, parent);
		  }

		/* Mark the parent as used.  */
		parent->last_device_path = 0;
	      }
	      /* Mark itself as used.  */
	      d->last_device_path = 0;
	      break;

	    default:
#ifdef DEBUG_NAMES
	      holy_printf ("skipping other type: ");
	      holy_efi_print_device_path (d->device_path);
#endif
	      /* For now, ignore the others.  */
	      break;
	    }
	}
      else
	{
#ifdef DEBUG_NAMES
	  holy_printf ("skipping non-media: ");
	  holy_efi_print_device_path (d->device_path);
#endif
	}
    }

  /* Let's see what can be added more.  */
  for (d = devices; d; d = d->next)
    {
      holy_efi_device_path_t *dp;
      holy_efi_block_io_media_t *m;
      int is_floppy = 0;

      dp = d->last_device_path;
      if (! dp)
	continue;

      /* Ghosts proudly presented by Apple.  */
      if (holy_EFI_DEVICE_PATH_TYPE (dp) == holy_EFI_MEDIA_DEVICE_PATH_TYPE
	  && holy_EFI_DEVICE_PATH_SUBTYPE (dp)
	  == holy_EFI_VENDOR_MEDIA_DEVICE_PATH_SUBTYPE)
	{
	  holy_efi_vendor_device_path_t *vendor = (holy_efi_vendor_device_path_t *) dp;
	  const struct holy_efi_guid apple = holy_EFI_VENDOR_APPLE_GUID;

	  if (vendor->header.length == sizeof (*vendor)
	      && holy_memcmp (&vendor->vendor_guid, &apple,
			      sizeof (vendor->vendor_guid)) == 0
	      && find_parent_device (devices, d))
	    continue;
	}

      m = d->block_io->media;
      if (holy_EFI_DEVICE_PATH_TYPE (dp) == holy_EFI_ACPI_DEVICE_PATH_TYPE
	  && holy_EFI_DEVICE_PATH_SUBTYPE (dp)
	  == holy_EFI_ACPI_DEVICE_PATH_SUBTYPE)
	{
	  holy_efi_acpi_device_path_t *acpi
	    = (holy_efi_acpi_device_path_t *) dp;
	  /* Floppy EISA ID.  */ 
	  if (acpi->hid == 0x60441d0 || acpi->hid == 0x70041d0
	      || acpi->hid == 0x70141d1)
	    is_floppy = 1;
	}
      if (is_floppy)
	{
#ifdef DEBUG_NAMES
	  holy_printf ("adding a floppy: ");
	  holy_efi_print_device_path (d->device_path);
#endif
	  add_device (&fd_devices, d);
	}
      else if (m->read_only && m->block_size > holy_DISK_SECTOR_SIZE)
	{
	  /* This check is too heuristic, but assume that this is a
	     CDROM drive.  */
#ifdef DEBUG_NAMES
	  holy_printf ("adding a cdrom by guessing: ");
	  holy_efi_print_device_path (d->device_path);
#endif
	  add_device (&cd_devices, d);
	}
      else
	{
	  /* The default is a hard drive.  */
#ifdef DEBUG_NAMES
	  holy_printf ("adding a hard drive by guessing: ");
	  holy_efi_print_device_path (d->device_path);
#endif
	  add_device (&hd_devices, d);
	}
    }
}

static void
free_devices (struct holy_efidisk_data *devices)
{
  struct holy_efidisk_data *p, *q;

  for (p = devices; p; p = q)
    {
      q = p->next;
      holy_free (p);
    }
}

/* Enumerate all disks to name devices.  */
static void
enumerate_disks (void)
{
  struct holy_efidisk_data *devices;

  devices = make_devices ();
  if (! devices)
    return;

  name_devices (devices);
  free_devices (devices);
}

static int
holy_efidisk_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data,
		      holy_disk_pull_t pull)
{
  struct holy_efidisk_data *d;
  char buf[16];
  int count;

  switch (pull)
    {
    case holy_DISK_PULL_NONE:
      for (d = hd_devices, count = 0; d; d = d->next, count++)
	{
	  holy_snprintf (buf, sizeof (buf), "hd%d", count);
	  holy_dprintf ("efidisk", "iterating %s\n", buf);
	  if (hook (buf, hook_data))
	    return 1;
	}
      break;
    case holy_DISK_PULL_REMOVABLE:
      for (d = fd_devices, count = 0; d; d = d->next, count++)
	{
	  holy_snprintf (buf, sizeof (buf), "fd%d", count);
	  holy_dprintf ("efidisk", "iterating %s\n", buf);
	  if (hook (buf, hook_data))
	    return 1;
	}

      for (d = cd_devices, count = 0; d; d = d->next, count++)
	{
	  holy_snprintf (buf, sizeof (buf), "cd%d", count);
	  holy_dprintf ("efidisk", "iterating %s\n", buf);
	  if (hook (buf, hook_data))
	    return 1;
	}
      break;
    default:
      return 0;
    }

  return 0;
}

static int
get_drive_number (const char *name)
{
  unsigned long drive;

  if ((name[0] != 'f' && name[0] != 'h' && name[0] != 'c') || name[1] != 'd')
    goto fail;

  drive = holy_strtoul (name + 2, 0, 10);
  if (holy_errno != holy_ERR_NONE)
    goto fail;

  return (int) drive ;

 fail:
  holy_error (holy_ERR_UNKNOWN_DEVICE, "not a efidisk");
  return -1;
}

static struct holy_efidisk_data *
get_device (struct holy_efidisk_data *devices, int num)
{
  struct holy_efidisk_data *d;

  for (d = devices; d && num; d = d->next, num--)
    ;

  if (num == 0)
    return d;

  return 0;
}

static holy_err_t
holy_efidisk_open (const char *name, struct holy_disk *disk)
{
  int num;
  struct holy_efidisk_data *d = 0;
  holy_efi_block_io_media_t *m;

  holy_dprintf ("efidisk", "opening %s\n", name);

  num = get_drive_number (name);
  if (num < 0)
    return holy_errno;

  switch (name[0])
    {
    case 'f':
      d = get_device (fd_devices, num);
      break;
    case 'c':
      d = get_device (cd_devices, num);
      break;
    case 'h':
      d = get_device (hd_devices, num);
      break;
    default:
      /* Never reach here.  */
      break;
    }

  if (! d)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "no such device");

  disk->id = ((num << holy_CHAR_BIT) | name[0]);
  m = d->block_io->media;
  /* FIXME: Probably it is better to store the block size in the disk,
     and total sectors should be replaced with total blocks.  */
  holy_dprintf ("efidisk",
		"m = %p, last block = %llx, block size = %x, io align = %x\n",
		m, (unsigned long long) m->last_block, m->block_size,
		m->io_align);

  /* Ensure required buffer alignment is a power of two (or is zero). */
  if (m->io_align & (m->io_align - 1))
    return holy_error (holy_ERR_IO, "invalid buffer alignment %d", m->io_align);

  disk->total_sectors = m->last_block + 1;
  /* Don't increase this value due to bug in some EFI.  */
  disk->max_agglomerate = 0xa0000 >> (holy_DISK_CACHE_BITS + holy_DISK_SECTOR_BITS);
  if (m->block_size & (m->block_size - 1) || !m->block_size)
    return holy_error (holy_ERR_IO, "invalid sector size %d",
		       m->block_size);
  for (disk->log_sector_size = 0;
       (1U << disk->log_sector_size) < m->block_size;
       disk->log_sector_size++);
  disk->data = d;

  holy_dprintf ("efidisk", "opening %s succeeded\n", name);

  return holy_ERR_NONE;
}

static void
holy_efidisk_close (struct holy_disk *disk __attribute__ ((unused)))
{
  /* EFI disks do not allocate extra memory, so nothing to do here.  */
  holy_dprintf ("efidisk", "closing %s\n", disk->name);
}

static holy_efi_status_t
holy_efidisk_readwrite (struct holy_disk *disk, holy_disk_addr_t sector,
			holy_size_t size, char *buf, int wr)
{
  struct holy_efidisk_data *d;
  holy_efi_block_io_t *bio;
  holy_efi_status_t status;
  holy_size_t io_align, num_bytes;
  char *aligned_buf;

  d = disk->data;
  bio = d->block_io;

  /* Set alignment to 1 if 0 specified */
  io_align = bio->media->io_align ? bio->media->io_align : 1;
  num_bytes = size << disk->log_sector_size;

  if ((holy_addr_t) buf & (io_align - 1))
    {
      aligned_buf = holy_memalign (io_align, num_bytes);
      if (! aligned_buf)
	return holy_EFI_OUT_OF_RESOURCES;
      if (wr)
	holy_memcpy (aligned_buf, buf, num_bytes);
    }
  else
    {
      aligned_buf = buf;
    }

  status =  efi_call_5 ((wr ? bio->write_blocks : bio->read_blocks), bio,
			bio->media->media_id, (holy_efi_uint64_t) sector,
			(holy_efi_uintn_t) num_bytes, aligned_buf);

  if ((holy_addr_t) buf & (io_align - 1))
    {
      if (!wr)
	holy_memcpy (buf, aligned_buf, num_bytes);
      holy_free (aligned_buf);
    }

  return status;
}

static holy_err_t
holy_efidisk_read (struct holy_disk *disk, holy_disk_addr_t sector,
		   holy_size_t size, char *buf)
{
  holy_efi_status_t status;

  holy_dprintf ("efidisk",
		"reading 0x%lx sectors at the sector 0x%llx from %s\n",
		(unsigned long) size, (unsigned long long) sector, disk->name);

  status = holy_efidisk_readwrite (disk, sector, size, buf, 0);

  if (status == holy_EFI_NO_MEDIA)
    return holy_error (holy_ERR_OUT_OF_RANGE, N_("no media in `%s'"), disk->name);
  else if (status != holy_EFI_SUCCESS)
    return holy_error (holy_ERR_READ_ERROR,
		       N_("failure reading sector 0x%llx from `%s'"),
		       (unsigned long long) sector,
		       disk->name);

  return holy_ERR_NONE;
}

static holy_err_t
holy_efidisk_write (struct holy_disk *disk, holy_disk_addr_t sector,
		    holy_size_t size, const char *buf)
{
  holy_efi_status_t status;

  holy_dprintf ("efidisk",
		"writing 0x%lx sectors at the sector 0x%llx to %s\n",
		(unsigned long) size, (unsigned long long) sector, disk->name);

  status = holy_efidisk_readwrite (disk, sector, size, (char *) buf, 1);

  if (status == holy_EFI_NO_MEDIA)
    return holy_error (holy_ERR_OUT_OF_RANGE, N_("no media in `%s'"), disk->name);
  else if (status != holy_EFI_SUCCESS)
    return holy_error (holy_ERR_WRITE_ERROR,
		       N_("failure writing sector 0x%llx to `%s'"),
		       (unsigned long long) sector, disk->name);

  return holy_ERR_NONE;
}

static struct holy_disk_dev holy_efidisk_dev =
  {
    .name = "efidisk",
    .id = holy_DISK_DEVICE_EFIDISK_ID,
    .iterate = holy_efidisk_iterate,
    .open = holy_efidisk_open,
    .close = holy_efidisk_close,
    .read = holy_efidisk_read,
    .write = holy_efidisk_write,
    .next = 0
  };

void
holy_efidisk_fini (void)
{
  free_devices (fd_devices);
  free_devices (hd_devices);
  free_devices (cd_devices);
  fd_devices = 0;
  hd_devices = 0;
  cd_devices = 0;
  holy_disk_dev_unregister (&holy_efidisk_dev);
}

void
holy_efidisk_init (void)
{
  holy_disk_firmware_fini = holy_efidisk_fini;

  enumerate_disks ();
  holy_disk_dev_register (&holy_efidisk_dev);
}

/* Some utility functions to map holy devices with EFI devices.  */
holy_efi_handle_t
holy_efidisk_get_device_handle (holy_disk_t disk)
{
  struct holy_efidisk_data *d;
  char type;

  if (disk->dev->id != holy_DISK_DEVICE_EFIDISK_ID)
    return 0;

  d = disk->data;
  type = disk->name[0];

  switch (type)
    {
    case 'f':
      /* This is the simplest case.  */
      return d->handle;

    case 'c':
      /* FIXME: probably this is not correct.  */
      return d->handle;

    case 'h':
      /* If this is the whole disk, just return its own data.  */
      if (! disk->partition)
	return d->handle;

      /* Otherwise, we must query the corresponding device to the firmware.  */
      {
	struct holy_efidisk_data *devices;
	holy_efi_handle_t handle = 0;
	struct holy_efidisk_data *c;

	devices = make_devices ();
	FOR_CHILDREN (c, devices)
	  {
	    holy_efi_hard_drive_device_path_t *hd;

	    hd = (holy_efi_hard_drive_device_path_t *) c->last_device_path;

	    if ((holy_EFI_DEVICE_PATH_TYPE (c->last_device_path)
		 == holy_EFI_MEDIA_DEVICE_PATH_TYPE)
		&& (holy_EFI_DEVICE_PATH_SUBTYPE (c->last_device_path)
		    == holy_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE)
		&& (holy_partition_get_start (disk->partition)
		    == (hd->partition_start << (disk->log_sector_size
						- holy_DISK_SECTOR_BITS)))
		&& (holy_partition_get_len (disk->partition)
		    == (hd->partition_size << (disk->log_sector_size
					       - holy_DISK_SECTOR_BITS))))
	      {
		handle = c->handle;
		break;
	      }
	  }

	free_devices (devices);

	if (handle != 0)
	  return handle;
      }
      break;

    default:
      break;
    }

  return 0;
}

#define NEEDED_BUFLEN sizeof ("XdXXXXXXXXXX")
static inline int
get_diskname_from_path_real (const holy_efi_device_path_t *path,
			     struct holy_efidisk_data *head,
			     char *buf)
{
  int count = 0;
  struct holy_efidisk_data *d;
  for (d = head, count = 0; d; d = d->next, count++)
    if (holy_efi_compare_device_paths (d->device_path, path) == 0)
      {
	holy_snprintf (buf, NEEDED_BUFLEN - 1, "d%d", count);
	return 1;
      }
  return 0;
}

static inline int
get_diskname_from_path (const holy_efi_device_path_t *path,
			char *buf)
{
  if (get_diskname_from_path_real (path, hd_devices, buf + 1))
    {
      buf[0] = 'h';
      return 1;
    }

  if (get_diskname_from_path_real (path, fd_devices, buf + 1))
    {
      buf[0] = 'f';
      return 1;
    }

  if (get_diskname_from_path_real (path, cd_devices, buf + 1))
    {
      buf[0] = 'c';
      return 1;
    }
  return 0;
}

/* Context for holy_efidisk_get_device_name.  */
struct holy_efidisk_get_device_name_ctx
{
  char *partition_name;
  holy_efi_hard_drive_device_path_t *hd;
};

/* Helper for holy_efidisk_get_device_name.
   Find the identical partition.  */
static int
holy_efidisk_get_device_name_iter (holy_disk_t disk,
				   const holy_partition_t part, void *data)
{
  struct holy_efidisk_get_device_name_ctx *ctx = data;

  if (holy_partition_get_start (part)
      == (ctx->hd->partition_start << (disk->log_sector_size
				       - holy_DISK_SECTOR_BITS))
      && holy_partition_get_len (part)
      == (ctx->hd->partition_size << (disk->log_sector_size
				      - holy_DISK_SECTOR_BITS)))
    {
      ctx->partition_name = holy_partition_get_name (part);
      return 1;
    }

  return 0;
}

char *
holy_efidisk_get_device_name (holy_efi_handle_t *handle)
{
  holy_efi_device_path_t *dp, *ldp;
  char device_name[NEEDED_BUFLEN];

  dp = holy_efi_get_device_path (handle);
  if (! dp)
    return 0;

  ldp = holy_efi_find_last_device_path (dp);
  if (! ldp)
    return 0;

  if (holy_EFI_DEVICE_PATH_TYPE (ldp) == holy_EFI_MEDIA_DEVICE_PATH_TYPE
      && (holy_EFI_DEVICE_PATH_SUBTYPE (ldp) == holy_EFI_CDROM_DEVICE_PATH_SUBTYPE
	  || holy_EFI_DEVICE_PATH_SUBTYPE (ldp) == holy_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE))
    {
      struct holy_efidisk_get_device_name_ctx ctx;
      char *dev_name;
      holy_efi_device_path_t *dup_dp;
      holy_disk_t parent = 0;

      /* It is necessary to duplicate the device path so that holy
	 can overwrite it.  */
      dup_dp = holy_efi_duplicate_device_path (dp);
      if (! dup_dp)
	return 0;

      while (1)
	{
	  holy_efi_device_path_t *dup_ldp;
	  dup_ldp = holy_efi_find_last_device_path (dup_dp);
	  if (!(holy_EFI_DEVICE_PATH_TYPE (dup_ldp) == holy_EFI_MEDIA_DEVICE_PATH_TYPE
		&& (holy_EFI_DEVICE_PATH_SUBTYPE (dup_ldp) == holy_EFI_CDROM_DEVICE_PATH_SUBTYPE
		    || holy_EFI_DEVICE_PATH_SUBTYPE (dup_ldp) == holy_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE)))
	    break;

	  dup_ldp->type = holy_EFI_END_DEVICE_PATH_TYPE;
	  dup_ldp->subtype = holy_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;
	  dup_ldp->length = sizeof (*dup_ldp);
	}

      if (!get_diskname_from_path (dup_dp, device_name))
	{
	  holy_free (dup_dp);
	  return 0;
	}

      parent = holy_disk_open (device_name);
      holy_free (dup_dp);

      if (! parent)
	return 0;

      /* Find a partition which matches the hard drive device path.  */
      ctx.partition_name = NULL;
      ctx.hd = (holy_efi_hard_drive_device_path_t *) ldp;
      if (ctx.hd->partition_start == 0
	  && (ctx.hd->partition_size << (parent->log_sector_size
					 - holy_DISK_SECTOR_BITS))
	  == holy_disk_get_size (parent))
	{
	  dev_name = holy_strdup (parent->name);
	}
      else
	{
	  holy_partition_iterate (parent, holy_efidisk_get_device_name_iter,
				  &ctx);

	  if (! ctx.partition_name)
	    {
	      /* No partition found. In most cases partition is embed in
		 the root path anyway, so this is not critical.
		 This happens only if partition is on partmap that holy
		 doesn't need to access root.
	       */
	      holy_disk_close (parent);
	      return holy_strdup (device_name);
	    }

	  dev_name = holy_xasprintf ("%s,%s", parent->name,
				     ctx.partition_name);
	  holy_free (ctx.partition_name);
	}
      holy_disk_close (parent);

      return dev_name;
    }
  /* This may be guessed device - floppy, cdrom or entire disk.  */
  if (!get_diskname_from_path (dp, device_name))
    return 0;
  return holy_strdup (device_name);
}
