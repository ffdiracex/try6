/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/file.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/disk.h>
#include <holy/dl.h>
#include <holy/i18n.h>
#include <holy/archelp.h>

holy_MOD_LICENSE ("GPLv2+");

struct holy_archelp_data
{
  holy_disk_t disk;
  holy_off_t hofs;
  holy_off_t next_hofs;
  holy_off_t dofs;
  holy_off_t size;
};

static holy_err_t
holy_cpio_find_file (struct holy_archelp_data *data, char **name,
		     holy_int32_t *mtime, holy_uint32_t *mode)
{
  struct head hd;
  holy_size_t namesize;
  holy_uint32_t modeval;

  data->hofs = data->next_hofs;

  if (holy_disk_read (data->disk, 0, data->hofs, sizeof (hd), &hd))
    return holy_errno;

  if (holy_memcmp (hd.magic, MAGIC, sizeof (hd.magic)) != 0
#ifdef MAGIC2
      && holy_memcmp (hd.magic, MAGIC2, sizeof (hd.magic)) != 0
#endif
      )
    return holy_error (holy_ERR_BAD_FS, "invalid cpio archive");
  data->size = read_number (hd.filesize, ARRAY_SIZE (hd.filesize));
  if (mtime)
    *mtime = read_number (hd.mtime, ARRAY_SIZE (hd.mtime));
  modeval = read_number (hd.mode, ARRAY_SIZE (hd.mode));
  namesize = read_number (hd.namesize, ARRAY_SIZE (hd.namesize));

  /* Don't allow negative numbers.  */
  if (namesize >= 0x80000000)
    {
      /* Probably a corruption, don't attempt to recover.  */
      *mode = holy_ARCHELP_ATTR_END;
      return holy_ERR_NONE;
    }

  *mode = modeval;

  *name = holy_malloc (namesize + 1);
  if (*name == NULL)
    return holy_errno;

  if (holy_disk_read (data->disk, 0, data->hofs + sizeof (hd),
		      namesize, *name))
    {
      holy_free (*name);
      return holy_errno;
    }
  (*name)[namesize] = 0;

  if (data->size == 0 && modeval == 0 && namesize == 11
      && holy_memcmp(*name, "TRAILER!!!", 11) == 0)
    {
      *mode = holy_ARCHELP_ATTR_END;
      holy_free (*name);
      return holy_ERR_NONE;
    }

  data->dofs = data->hofs + ALIGN_CPIO (sizeof (hd) + namesize);
  data->next_hofs = data->dofs + ALIGN_CPIO (data->size);
  return holy_ERR_NONE;
}

static char *
holy_cpio_get_link_target (struct holy_archelp_data *data)
{
  char *ret;
  holy_err_t err;

  if (data->size == 0)
    return holy_strdup ("");
  ret = holy_malloc (data->size + 1);
  if (!ret)
    return NULL;

  err = holy_disk_read (data->disk, 0, data->dofs, data->size,
			ret);
  if (err)
    {
      holy_free (ret);
      return NULL;
    }
  ret[data->size] = '\0';
  return ret;
}

static void
holy_cpio_rewind (struct holy_archelp_data *data)
{
  data->next_hofs = 0;
}

static struct holy_archelp_ops arcops =
  {
    .find_file = holy_cpio_find_file,
    .get_link_target = holy_cpio_get_link_target,
    .rewind = holy_cpio_rewind
  };

static struct holy_archelp_data *
holy_cpio_mount (holy_disk_t disk)
{
  struct head hd;
  struct holy_archelp_data *data;

  if (holy_disk_read (disk, 0, 0, sizeof (hd), &hd))
    goto fail;

  if (holy_memcmp (hd.magic, MAGIC, sizeof (MAGIC) - 1)
#ifdef MAGIC2
      && holy_memcmp (hd.magic, MAGIC2, sizeof (MAGIC2) - 1)
#endif
      )
    goto fail;

  data = (struct holy_archelp_data *) holy_zalloc (sizeof (*data));
  if (!data)
    goto fail;

  data->disk = disk;

  return data;

fail:
  holy_error (holy_ERR_BAD_FS, "not a " FSNAME " filesystem");
  return 0;
}

static holy_err_t
holy_cpio_dir (holy_device_t device, const char *path_in,
	       holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_archelp_data *data;
  holy_err_t err;

  data = holy_cpio_mount (device->disk);
  if (!data)
    return holy_errno;

  err = holy_archelp_dir (data, &arcops,
			  path_in, hook, hook_data);

  holy_free (data);

  return err;
}

static holy_err_t
holy_cpio_open (holy_file_t file, const char *name_in)
{
  struct holy_archelp_data *data;
  holy_err_t err;

  data = holy_cpio_mount (file->device->disk);
  if (!data)
    return holy_errno;

  err = holy_archelp_open (data, &arcops, name_in);
  if (err)
    {
      holy_free (data);
    }
  else
    {
      file->data = data;
      file->size = data->size;
    }
  return err;
}

static holy_ssize_t
holy_cpio_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_archelp_data *data;
  holy_ssize_t ret;

  data = file->data;
  data->disk->read_hook = file->read_hook;
  data->disk->read_hook_data = file->read_hook_data;

  ret = (holy_disk_read (data->disk, 0, data->dofs + file->offset,
			 len, buf)) ? -1 : (holy_ssize_t) len;
  data->disk->read_hook = 0;

  return ret;
}

static holy_err_t
holy_cpio_close (holy_file_t file)
{
  struct holy_archelp_data *data;

  data = file->data;
  holy_free (data);

  return holy_errno;
}

static struct holy_fs holy_cpio_fs = {
  .name = FSNAME,
  .dir = holy_cpio_dir,
  .open = holy_cpio_open,
  .read = holy_cpio_read,
  .close = holy_cpio_close,
#ifdef holy_UTIL
  .reserved_first_sector = 0,
  .blocklist_install = 0,
#endif
};
