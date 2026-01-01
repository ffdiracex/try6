/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config-util.h>

#include <holy/fs.h>
#include <holy/file.h>
#include <holy/disk.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/dl.h>
#include <holy/util/misc.h>
#include <holy/emu/hostdisk.h>
#include <holy/i18n.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>

static int
is_dir (const char *path, const char *name)
{
  int len1 = strlen(path);
  int len2 = strlen(name);
  int ret;

  char *pathname = xmalloc (len1 + 1 + len2 + 1 + 13);
  strcpy (pathname, path);

  /* Avoid UNC-path "//name" on Cygwin.  */
  if (len1 > 0 && pathname[len1 - 1] != '/')
    strcat (pathname, "/");

  strcat (pathname, name);

  ret = holy_util_is_directory (pathname);
  free (pathname);
  return ret;
}

struct holy_hostfs_data
{
  char *filename;
  holy_util_fd_t f;
};

static holy_err_t
holy_hostfs_dir (holy_device_t device, const char *path,
		 holy_fs_dir_hook_t hook, void *hook_data)
{
  holy_util_fd_dir_t dir;

  /* Check if the disk is our dummy disk.  */
  if (holy_strcmp (device->disk->name, "host"))
    return holy_error (holy_ERR_BAD_FS, "not a hostfs");

  dir = holy_util_fd_opendir (path);
  if (! dir)
    return holy_error (holy_ERR_BAD_FILENAME,
		       N_("can't open `%s': %s"), path,
		       holy_util_fd_strerror ());

  while (1)
    {
      holy_util_fd_dirent_t de;
      struct holy_dirhook_info info;
      holy_memset (&info, 0, sizeof (info));

      de = holy_util_fd_readdir (dir);
      if (! de)
	break;

      info.dir = !! is_dir (path, de->d_name);
      hook (de->d_name, &info, hook_data);

    }

  holy_util_fd_closedir (dir);

  return holy_ERR_NONE;
}

/* Open a file named NAME and initialize FILE.  */
static holy_err_t
holy_hostfs_open (struct holy_file *file, const char *name)
{
  holy_util_fd_t f;
  struct holy_hostfs_data *data;

  f = holy_util_fd_open (name, holy_UTIL_FD_O_RDONLY);
  if (! holy_UTIL_FD_IS_VALID (f))
    return holy_error (holy_ERR_BAD_FILENAME,
		       N_("can't open `%s': %s"), name,
		       strerror (errno));
  data = holy_malloc (sizeof (*data));
  if (!data)
    {
      holy_util_fd_close (f);
      return holy_errno;
    }
  data->filename = holy_strdup (name);
  if (!data->filename)
    {
      holy_free (data);
      holy_util_fd_close (f);
      return holy_errno;
    }

  data->f = f;  

  file->data = data;

  file->size = holy_util_get_fd_size (f, name, NULL);

  return holy_ERR_NONE;
}

static holy_ssize_t
holy_hostfs_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_hostfs_data *data;

  data = file->data;
  if (holy_util_fd_seek (data->f, file->offset) != 0)
    {
      holy_error (holy_ERR_OUT_OF_RANGE, N_("cannot seek `%s': %s"),
		  data->filename, holy_util_fd_strerror ());
      return -1;
    }

  unsigned int s = holy_util_fd_read (data->f, buf, len);
  if (s != len)
    holy_error (holy_ERR_FILE_READ_ERROR, N_("cannot read `%s': %s"),
		data->filename, holy_util_fd_strerror ());

  return (signed) s;
}

static holy_err_t
holy_hostfs_close (holy_file_t file)
{
  struct holy_hostfs_data *data;

  data = file->data;
  holy_util_fd_close (data->f);
  holy_free (data->filename);
  holy_free (data);

  return holy_ERR_NONE;
}

static holy_err_t
holy_hostfs_label (holy_device_t device __attribute ((unused)),
		   char **label __attribute ((unused)))
{
  *label = 0;
  return holy_ERR_NONE;
}

static struct holy_fs holy_hostfs_fs =
  {
    .name = "hostfs",
    .dir = holy_hostfs_dir,
    .open = holy_hostfs_open,
    .read = holy_hostfs_read,
    .close = holy_hostfs_close,
    .label = holy_hostfs_label,
    .next = 0
  };



holy_MOD_INIT(hostfs)
{
  holy_fs_register (&holy_hostfs_fs);
}

holy_MOD_FINI(hostfs)
{
  holy_fs_unregister (&holy_hostfs_fs);
}
