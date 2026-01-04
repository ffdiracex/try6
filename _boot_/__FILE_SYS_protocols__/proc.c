/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/procfs.h>
#include <holy/disk.h>
#include <holy/fs.h>
#include <holy/file.h>
#include <holy/mm.h>
#include <holy/dl.h>
#include <holy/archelp.h>

holy_MOD_LICENSE ("GPLv2+");

struct holy_procfs_entry *holy_procfs_entries;

static int
holy_procdev_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data,
			 holy_disk_pull_t pull)
{
  if (pull != holy_DISK_PULL_NONE)
    return 0;

  return hook ("proc", hook_data);
}

static holy_err_t
holy_procdev_open (const char *name, holy_disk_t disk)
{
  if (holy_strcmp (name, "proc"))
      return holy_error (holy_ERR_UNKNOWN_DEVICE, "not a procfs disk");

  disk->total_sectors = 0;
  disk->id = 0;

  disk->data = 0;

  return holy_ERR_NONE;
}

static void
holy_procdev_close (holy_disk_t disk __attribute((unused)))
{
}

static holy_err_t
holy_procdev_read (holy_disk_t disk __attribute((unused)),
		holy_disk_addr_t sector __attribute((unused)),
		holy_size_t size __attribute((unused)),
		char *buf __attribute((unused)))
{
  return holy_ERR_OUT_OF_RANGE;
}

static holy_err_t
holy_procdev_write (holy_disk_t disk __attribute ((unused)),
		       holy_disk_addr_t sector __attribute ((unused)),
		       holy_size_t size __attribute ((unused)),
		       const char *buf __attribute ((unused)))
{
  return holy_ERR_OUT_OF_RANGE;
}

struct holy_archelp_data
{
  struct holy_procfs_entry *entry, *next_entry;
};

static void
holy_procfs_rewind (struct holy_archelp_data *data)
{
  data->entry = NULL;
  data->next_entry = holy_procfs_entries;
}

static holy_err_t
holy_procfs_find_file (struct holy_archelp_data *data, char **name,
		     holy_int32_t *mtime,
		     holy_uint32_t *mode)
{
  data->entry = data->next_entry;
  if (!data->entry)
    {
      *mode = holy_ARCHELP_ATTR_END;
      return holy_ERR_NONE;
    }
  data->next_entry = data->entry->next;
  *mode = holy_ARCHELP_ATTR_FILE | holy_ARCHELP_ATTR_NOTIME;
  *name = holy_strdup (data->entry->name);
  *mtime = 0;
  if (!*name)
    return holy_errno;
  return holy_ERR_NONE;
}

static struct holy_archelp_ops arcops =
  {
    .find_file = holy_procfs_find_file,
    .rewind = holy_procfs_rewind
  };

static holy_ssize_t
holy_procfs_read (holy_file_t file, char *buf, holy_size_t len)
{
  char *data = file->data;

  holy_memcpy (buf, data + file->offset, len);

  return len;
}

static holy_err_t
holy_procfs_close (holy_file_t file)
{
  char *data;

  data = file->data;
  holy_free (data);

  return holy_ERR_NONE;
}

static holy_err_t
holy_procfs_dir (holy_device_t device, const char *path,
		 holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_archelp_data data;

  /* Check if the disk is our dummy disk.  */
  if (holy_strcmp (device->disk->name, "proc"))
    return holy_error (holy_ERR_BAD_FS, "not a procfs");

  holy_procfs_rewind (&data);

  return holy_archelp_dir (&data, &arcops,
			   path, hook, hook_data);
}

static holy_err_t
holy_procfs_open (struct holy_file *file, const char *path)
{
  holy_err_t err;
  struct holy_archelp_data data;
  holy_size_t sz;

  holy_procfs_rewind (&data);

  err = holy_archelp_open (&data, &arcops, path);
  if (err)
    return err;
  file->data = data.entry->get_contents (&sz);
  if (!file->data)
    return holy_errno;
  file->size = sz;
  return holy_ERR_NONE;
}

static struct holy_disk_dev holy_procfs_dev = {
  .name = "proc",
  .id = holy_DISK_DEVICE_PROCFS_ID,
  .iterate = holy_procdev_iterate,
  .open = holy_procdev_open,
  .close = holy_procdev_close,
  .read = holy_procdev_read,
  .write = holy_procdev_write,
  .next = 0
};

static struct holy_fs holy_procfs_fs =
  {
    .name = "procfs",
    .dir = holy_procfs_dir,
    .open = holy_procfs_open,
    .read = holy_procfs_read,
    .close = holy_procfs_close,
    .next = 0
  };

holy_MOD_INIT (procfs)
{
  holy_disk_dev_register (&holy_procfs_dev);
  holy_fs_register (&holy_procfs_fs);
}

holy_MOD_FINI (procfs)
{
  holy_disk_dev_unregister (&holy_procfs_dev);
  holy_fs_unregister (&holy_procfs_fs);
}
