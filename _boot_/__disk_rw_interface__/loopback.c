/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/file.h>
#include <holy/disk.h>
#include <holy/mm.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

struct holy_loopback
{
  char *devname;
  holy_file_t file;
  struct holy_loopback *next;
  unsigned long id;
};

static struct holy_loopback *loopback_list;
static unsigned long last_id = 0;

static const struct holy_arg_option options[] =
  {
    /* TRANSLATORS: The disk is simply removed from the list of available ones,
       not wiped, avoid to scare user.  */
    {"delete", 'd', 0, N_("Delete the specified loopback drive."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

/* Delete the loopback device NAME.  */
static holy_err_t
delete_loopback (const char *name)
{
  struct holy_loopback *dev;
  struct holy_loopback **prev;

  /* Search for the device.  */
  for (dev = loopback_list, prev = &loopback_list;
       dev;
       prev = &dev->next, dev = dev->next)
    if (holy_strcmp (dev->devname, name) == 0)
      break;

  if (! dev)
    return holy_error (holy_ERR_BAD_DEVICE, "device not found");

  /* Remove the device from the list.  */
  *prev = dev->next;

  holy_free (dev->devname);
  holy_file_close (dev->file);
  holy_free (dev);

  return 0;
}

/* The command to add and remove loopback devices.  */
static holy_err_t
holy_cmd_loopback (holy_extcmd_context_t ctxt, int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;
  holy_file_t file;
  struct holy_loopback *newdev;
  holy_err_t ret;

  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, "device name required");

  /* Check if `-d' was used.  */
  if (state[0].set)
      return delete_loopback (args[0]);

  if (argc < 2)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  file = holy_file_open (args[1]);
  if (! file)
    return holy_errno;

  /* First try to replace the old device.  */
  for (newdev = loopback_list; newdev; newdev = newdev->next)
    if (holy_strcmp (newdev->devname, args[0]) == 0)
      break;

  if (newdev)
    {
      holy_file_close (newdev->file);
      newdev->file = file;

      return 0;
    }

  /* Unable to replace it, make a new entry.  */
  newdev = holy_malloc (sizeof (struct holy_loopback));
  if (! newdev)
    goto fail;

  newdev->devname = holy_strdup (args[0]);
  if (! newdev->devname)
    {
      holy_free (newdev);
      goto fail;
    }

  newdev->file = file;
  newdev->id = last_id++;

  /* Add the new entry to the list.  */
  newdev->next = loopback_list;
  loopback_list = newdev;

  return 0;

fail:
  ret = holy_errno;
  holy_file_close (file);
  return ret;
}


static int
holy_loopback_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data,
		       holy_disk_pull_t pull)
{
  struct holy_loopback *d;
  if (pull != holy_DISK_PULL_NONE)
    return 0;
  for (d = loopback_list; d; d = d->next)
    {
      if (hook (d->devname, hook_data))
	return 1;
    }
  return 0;
}

static holy_err_t
holy_loopback_open (const char *name, holy_disk_t disk)
{
  struct holy_loopback *dev;

  for (dev = loopback_list; dev; dev = dev->next)
    if (holy_strcmp (dev->devname, name) == 0)
      break;

  if (! dev)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "can't open device");

  /* Use the filesize for the disk size, round up to a complete sector.  */
  if (dev->file->size != holy_FILE_SIZE_UNKNOWN)
    disk->total_sectors = ((dev->file->size + holy_DISK_SECTOR_SIZE - 1)
			   / holy_DISK_SECTOR_SIZE);
  else
    disk->total_sectors = holy_DISK_SIZE_UNKNOWN;
  /* Avoid reading more than 512M.  */
  disk->max_agglomerate = 1 << (29 - holy_DISK_SECTOR_BITS
				- holy_DISK_CACHE_BITS);

  disk->id = dev->id;

  disk->data = dev;

  return 0;
}

static holy_err_t
holy_loopback_read (holy_disk_t disk, holy_disk_addr_t sector,
		    holy_size_t size, char *buf)
{
  holy_file_t file = ((struct holy_loopback *) disk->data)->file;
  holy_off_t pos;

  holy_file_seek (file, sector << holy_DISK_SECTOR_BITS);

  holy_file_read (file, buf, size << holy_DISK_SECTOR_BITS);
  if (holy_errno)
    return holy_errno;

  /* In case there is more data read than there is available, in case
     of files that are not a multiple of holy_DISK_SECTOR_SIZE, fill
     the rest with zeros.  */
  pos = (sector + size) << holy_DISK_SECTOR_BITS;
  if (pos > file->size)
    {
      holy_size_t amount = pos - file->size;
      holy_memset (buf + (size << holy_DISK_SECTOR_BITS) - amount, 0, amount);
    }

  return 0;
}

static holy_err_t
holy_loopback_write (holy_disk_t disk __attribute ((unused)),
		     holy_disk_addr_t sector __attribute ((unused)),
		     holy_size_t size __attribute ((unused)),
		     const char *buf __attribute ((unused)))
{
  return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		     "loopback write is not supported");
}

static struct holy_disk_dev holy_loopback_dev =
  {
    .name = "loopback",
    .id = holy_DISK_DEVICE_LOOPBACK_ID,
    .iterate = holy_loopback_iterate,
    .open = holy_loopback_open,
    .read = holy_loopback_read,
    .write = holy_loopback_write,
    .next = 0
  };

static holy_extcmd_t cmd;

holy_MOD_INIT(loopback)
{
  cmd = holy_register_extcmd ("loopback", holy_cmd_loopback, 0,
			      N_("[-d] DEVICENAME FILE."),
			      /* TRANSLATORS: The file itself is not destroyed
				 or transformed into drive.  */
			      N_("Make a virtual drive from a file."), options);
  holy_disk_dev_register (&holy_loopback_dev);
}

holy_MOD_FINI(loopback)
{
  holy_unregister_extcmd (cmd);
  holy_disk_dev_unregister (&holy_loopback_dev);
}
