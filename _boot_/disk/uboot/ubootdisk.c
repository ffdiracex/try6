/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/disk.h>
#include <holy/err.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/partition.h>
#include <holy/term.h>
#include <holy/types.h>
#include <holy/uboot/disk.h>
#include <holy/uboot/uboot.h>
#include <holy/uboot/api_public.h>

static struct ubootdisk_data *hd_devices;
static int hd_num;
static int hd_max;

/*
 * holy_ubootdisk_register():
 *   Called for each disk device enumerated as part of U-Boot initialization
 *   code.
 */
holy_err_t
holy_ubootdisk_register (struct device_info *newdev)
{
  struct ubootdisk_data *d;

#define STOR_TYPE(x) ((x) & 0x0ff0)
  switch (STOR_TYPE (newdev->type))
    {
    case DT_STOR_IDE:
    case DT_STOR_SATA:
    case DT_STOR_SCSI:
    case DT_STOR_MMC:
    case DT_STOR_USB:
      /* hd */
      if (hd_num == hd_max)
	{
	  int new_num;
	  new_num = (hd_max ? hd_max * 2 : 1);
	  d = holy_realloc(hd_devices,
			   sizeof (struct ubootdisk_data) * new_num);
	  if (!d)
	    return holy_errno;
	  hd_devices = d;
	  hd_max = new_num;
	}

      d = &hd_devices[hd_num];
      hd_num++;
      break;
    default:
      return holy_ERR_BAD_DEVICE;
      break;
    }

  d->dev = newdev;
  d->cookie = newdev->cookie;
  d->opencount = 0;

  return 0;
}

/*
 * uboot_disk_iterate():
 *   Iterator over enumerated disk devices.
 */
static int
uboot_disk_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data,
		    holy_disk_pull_t pull)
{
  char buf[16];
  int count;

  switch (pull)
    {
    case holy_DISK_PULL_NONE:
      /* "hd" - built-in mass-storage */
      for (count = 0 ; count < hd_num; count++)
	{
	  holy_snprintf (buf, sizeof (buf) - 1, "hd%d", count);
	  holy_dprintf ("ubootdisk", "iterating %s\n", buf);
	  if (hook (buf, hook_data))
	    return 1;
	}
      break;
    default:
      return 0;
    }

  return 0;
}

/* Helper function for uboot_disk_open. */
static struct ubootdisk_data *
get_hd_device (int num)
{
  if (num < hd_num)
    return &hd_devices[num];

  return NULL;
}

/*
 * uboot_disk_open():
 *   Opens a disk device already enumerated.
 */
static holy_err_t
uboot_disk_open (const char *name, struct holy_disk *disk)
{
  struct ubootdisk_data *d;
  struct device_info *devinfo;
  int num;
  int retval;

  holy_dprintf ("ubootdisk", "Opening '%s'\n", name);

  num = holy_strtoul (name + 2, 0, 10);
  if (holy_errno != holy_ERR_NONE)
    {
      holy_dprintf ("ubootdisk", "Opening '%s' failed, invalid number\n",
		    name);
      goto fail;
    }

  if (name[1] != 'd')
    {
      holy_dprintf ("ubootdisk", "Opening '%s' failed, invalid name\n", name);
      goto fail;
    }

  switch (name[0])
    {
    case 'h':
      d = get_hd_device (num);
      break;
    default:
      goto fail;
    }

  if (!d)
    goto fail;

  /*
   * Subsystems may call open on the same device recursively - but U-Boot
   * does not deal with this. So simply keep track of number of calls and
   * return success if already open.
   */
  if (d->opencount > 0)
    {
      holy_dprintf ("ubootdisk", "(%s) already open\n", disk->name);
      d->opencount++;
      retval = 0;
    }
  else
    {
      retval = holy_uboot_dev_open (d->dev);
      if (retval != 0)
	goto fail;
      d->opencount = 1;
    }

  holy_dprintf ("ubootdisk", "cookie: 0x%08x\n", (holy_addr_t) d->cookie);
  disk->id = (holy_addr_t) d->cookie;

  devinfo = d->dev;

  d->block_size = devinfo->di_stor.block_size;
  if (d->block_size == 0)
    return holy_error (holy_ERR_IO, "no block size");

  for (disk->log_sector_size = 0;
       (1U << disk->log_sector_size) < d->block_size;
       disk->log_sector_size++);

  holy_dprintf ("ubootdisk", "(%s) blocksize=%d, log_sector_size=%d\n",
		disk->name, d->block_size, disk->log_sector_size);

  if (devinfo->di_stor.block_count)
    disk->total_sectors = devinfo->di_stor.block_count;
  else
    disk->total_sectors = holy_DISK_SIZE_UNKNOWN;

  disk->data = d;

  return holy_ERR_NONE;

fail:
  return holy_error (holy_ERR_UNKNOWN_DEVICE, "no such device");
}

static void
uboot_disk_close (struct holy_disk *disk)
{
  struct ubootdisk_data *d;
  int retval;

  d = disk->data;

  /*
   * In mirror of open function, keep track of number of calls to close and
   * send on to U-Boot only when opencount would decrease to 0.
   */
  if (d->opencount > 1)
    {
      holy_dprintf ("ubootdisk", "Closed (%s)\n", disk->name);

      d->opencount--;
    }
  else if (d->opencount == 1)
    {
      retval = holy_uboot_dev_close (d->dev);
      d->opencount--;
      holy_dprintf ("ubootdisk", "closed %s (%d)\n", disk->name, retval);
    }
  else
    {
      holy_dprintf ("ubootdisk", "device %s not open!\n", disk->name);
    }
}

/*
 * uboot_disk_read():
 *   Called from within disk subsystem to read a sequence of blocks into the
 *   disk cache. Maps directly on top of U-Boot API, only wrap in some error
 *   handling.
 */
static holy_err_t
uboot_disk_read (struct holy_disk *disk,
		 holy_disk_addr_t offset, holy_size_t numblocks, char *buf)
{
  struct ubootdisk_data *d;
  holy_size_t real_size;
  int retval;

  d = disk->data;

  retval = holy_uboot_dev_read (d->dev, buf, numblocks, offset, &real_size);
  holy_dprintf ("ubootdisk",
		"retval=%d, numblocks=%d, real_size=%llu, sector=%llu\n",
		retval, numblocks, (holy_uint64_t) real_size,
		(holy_uint64_t) offset);
  if (retval != 0)
    return holy_error (holy_ERR_IO, "U-Boot disk read error");

  return holy_ERR_NONE;
}

static holy_err_t
uboot_disk_write (struct holy_disk *disk __attribute__ ((unused)),
		  holy_disk_addr_t sector __attribute__ ((unused)),
		  holy_size_t size __attribute__ ((unused)),
		  const char *buf __attribute__ ((unused)))
{
  return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		     "attempt to write (not supported)");
}

static struct holy_disk_dev holy_ubootdisk_dev = {
  .name = "ubootdisk",
  .id = holy_DISK_DEVICE_UBOOTDISK_ID,
  .iterate = uboot_disk_iterate,
  .open = uboot_disk_open,
  .close = uboot_disk_close,
  .read = uboot_disk_read,
  .write = uboot_disk_write,
  .next = 0
};

void
holy_ubootdisk_init (void)
{
  holy_disk_dev_register (&holy_ubootdisk_dev);
}

void
holy_ubootdisk_fini (void)
{
  holy_disk_dev_unregister (&holy_ubootdisk_dev);
}
