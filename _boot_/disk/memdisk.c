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

holy_MOD_LICENSE ("GPLv2+");

static char *memdisk_addr;
static holy_off_t memdisk_size = 0;

static int
holy_memdisk_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data,
		      holy_disk_pull_t pull)
{
  if (pull != holy_DISK_PULL_NONE)
    return 0;

  return hook ("memdisk", hook_data);
}

static holy_err_t
holy_memdisk_open (const char *name, holy_disk_t disk)
{
  if (holy_strcmp (name, "memdisk"))
      return holy_error (holy_ERR_UNKNOWN_DEVICE, "not a memdisk");

  disk->total_sectors = memdisk_size / holy_DISK_SECTOR_SIZE;
  disk->max_agglomerate = holy_DISK_MAX_MAX_AGGLOMERATE;
  disk->id = 0;

  return holy_ERR_NONE;
}

static void
holy_memdisk_close (holy_disk_t disk __attribute((unused)))
{
}

static holy_err_t
holy_memdisk_read (holy_disk_t disk __attribute((unused)), holy_disk_addr_t sector,
		    holy_size_t size, char *buf)
{
  holy_memcpy (buf, memdisk_addr + (sector << holy_DISK_SECTOR_BITS), size << holy_DISK_SECTOR_BITS);
  return 0;
}

static holy_err_t
holy_memdisk_write (holy_disk_t disk __attribute((unused)), holy_disk_addr_t sector,
		     holy_size_t size, const char *buf)
{
  holy_memcpy (memdisk_addr + (sector << holy_DISK_SECTOR_BITS), buf, size << holy_DISK_SECTOR_BITS);
  return 0;
}

static struct holy_disk_dev holy_memdisk_dev =
  {
    .name = "memdisk",
    .id = holy_DISK_DEVICE_MEMDISK_ID,
    .iterate = holy_memdisk_iterate,
    .open = holy_memdisk_open,
    .close = holy_memdisk_close,
    .read = holy_memdisk_read,
    .write = holy_memdisk_write,
    .next = 0
  };

holy_MOD_INIT(memdisk)
{
  struct holy_module_header *header;
  FOR_MODULES (header)
    if (header->type == OBJ_TYPE_MEMDISK)
      {
	char *memdisk_orig_addr;
	memdisk_orig_addr = (char *) header + sizeof (struct holy_module_header);

	holy_dprintf ("memdisk", "Found memdisk image at %p\n", memdisk_orig_addr);

	memdisk_size = header->size - sizeof (struct holy_module_header);
	memdisk_addr = holy_malloc (memdisk_size);

	holy_dprintf ("memdisk", "Copying memdisk image to dynamic memory\n");
	holy_memmove (memdisk_addr, memdisk_orig_addr, memdisk_size);

	holy_disk_dev_register (&holy_memdisk_dev);
	break;
      }
}

holy_MOD_FINI(memdisk)
{
  if (! memdisk_size)
    return;
  holy_free (memdisk_addr);
  holy_disk_dev_unregister (&holy_memdisk_dev);
}
