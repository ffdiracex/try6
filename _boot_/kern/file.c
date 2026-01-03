/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/err.h>
#include <holy/file.h>
#include <holy/net.h>
#include <holy/mm.h>
#include <holy/fs.h>
#include <holy/device.h>
#include <holy/i18n.h>

void (*EXPORT_VAR (holy_holynet_fini)) (void);

holy_file_filter_t holy_file_filters_all[holy_FILE_FILTER_MAX];
holy_file_filter_t holy_file_filters_enabled[holy_FILE_FILTER_MAX];

/* Get the device part of the filename NAME. It is enclosed by parentheses.  */
char *
holy_file_get_device_name (const char *name)
{
  if (name[0] == '(')
    {
      char *p = holy_strchr (name, ')');
      char *ret;

      if (! p)
	{
	  holy_error (holy_ERR_BAD_FILENAME, N_("missing `%c' symbol"), ')');
	  return 0;
	}

      ret = (char *) holy_malloc (p - name);
      if (! ret)
	return 0;

      holy_memcpy (ret, name + 1, p - name - 1);
      ret[p - name - 1] = '\0';
      return ret;
    }

  return 0;
}

holy_file_t
holy_file_open (const char *name)
{
  holy_device_t device = 0;
  holy_file_t file = 0, last_file = 0;
  char *device_name;
  const char *file_name;
  holy_file_filter_id_t filter;

  device_name = holy_file_get_device_name (name);
  if (holy_errno)
    goto fail;

  /* Get the file part of NAME.  */
  file_name = (name[0] == '(') ? holy_strchr (name, ')') : NULL;
  if (file_name)
    file_name++;
  else
    file_name = name;

  device = holy_device_open (device_name);
  holy_free (device_name);
  if (! device)
    goto fail;

  file = (holy_file_t) holy_zalloc (sizeof (*file));
  if (! file)
    goto fail;

  file->device = device;

  /* In case of relative pathnames and non-Unix systems (like Windows)
   * name of host files may not start with `/'. Blocklists for host files
   * are meaningless as well (for a start, host disk does not allow any direct
   * access - it is just a marker). So skip host disk in this case.
   */
  if (device->disk && file_name[0] != '/'
#if defined(holy_UTIL) || defined(holy_MACHINE_EMU)
      && holy_strcmp (device->disk->name, "host")
#endif
     )
    /* This is a block list.  */
    file->fs = &holy_fs_blocklist;
  else
    {
      file->fs = holy_fs_probe (device);
      if (! file->fs)
	goto fail;
    }

  if ((file->fs->open) (file, file_name) != holy_ERR_NONE)
    goto fail;

  file->name = holy_strdup (name);
  holy_errno = holy_ERR_NONE;

  for (filter = 0; file && filter < ARRAY_SIZE (holy_file_filters_enabled);
       filter++)
    if (holy_file_filters_enabled[filter])
      {
	last_file = file;
	file = holy_file_filters_enabled[filter] (file, name);
      }
  if (!file)
    holy_file_close (last_file);
    
  holy_memcpy (holy_file_filters_enabled, holy_file_filters_all,
	       sizeof (holy_file_filters_enabled));

  return file;

 fail:
  if (device)
    holy_device_close (device);

  /* if (net) holy_net_close (net);  */

  holy_free (file);

  holy_memcpy (holy_file_filters_enabled, holy_file_filters_all,
	       sizeof (holy_file_filters_enabled));

  return 0;
}

holy_disk_read_hook_t holy_file_progress_hook;

holy_ssize_t
holy_file_read (holy_file_t file, void *buf, holy_size_t len)
{
  holy_ssize_t res;
  holy_disk_read_hook_t read_hook;
  void *read_hook_data;

  if (file->offset > file->size)
    {
      holy_error (holy_ERR_OUT_OF_RANGE,
		  N_("attempt to read past the end of file"));
      return -1;
    }

  if (len == 0)
    return 0;

  if (len > file->size - file->offset)
    len = file->size - file->offset;

  /* Prevent an overflow.  */
  if ((holy_ssize_t) len < 0)
    len >>= 1;

  if (len == 0)
    return 0;
  read_hook = file->read_hook;
  read_hook_data = file->read_hook_data;
  if (!file->read_hook)
    {
      file->read_hook = holy_file_progress_hook;
      file->read_hook_data = file;
      file->progress_offset = file->offset;
    }
  res = (file->fs->read) (file, buf, len);
  file->read_hook = read_hook;
  file->read_hook_data = read_hook_data;
  if (res > 0)
    file->offset += res;

  return res;
}

holy_err_t
holy_file_close (holy_file_t file)
{
  if (file->fs->close)
    (file->fs->close) (file);

  if (file->device)
    holy_device_close (file->device);
  holy_free (file->name);
  holy_free (file);
  return holy_errno;
}

holy_off_t
holy_file_seek (holy_file_t file, holy_off_t offset)
{
  holy_off_t old;

  if (offset > file->size)
    {
      holy_error (holy_ERR_OUT_OF_RANGE,
		  N_("attempt to seek outside of the file"));
      return -1;
    }
  
  old = file->offset;
  file->offset = offset;
    
  return old;
}
