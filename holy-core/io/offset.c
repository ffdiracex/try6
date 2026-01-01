/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/file.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

struct holy_offset_file
{
  holy_file_t parent;
  holy_off_t off;
};

static holy_ssize_t
holy_offset_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_offset_file *data = file->data;
  if (holy_file_seek (data->parent, data->off + file->offset) == (holy_off_t) -1)
    return -1;
  return holy_file_read (data->parent, buf, len);
}

static holy_err_t
holy_offset_close (holy_file_t file)
{
  struct holy_offset_file *data = file->data;

  if (data->parent)
    holy_file_close (data->parent);

  /* No need to close the same device twice.  */
  file->device = 0;

  return 0;
}

static struct holy_fs holy_offset_fs = {
  .name = "offset",
  .dir = 0,
  .open = 0,
  .read = holy_offset_read,
  .close = holy_offset_close,
  .label = 0,
  .next = 0
};

void
holy_file_offset_close (holy_file_t file)
{
  struct holy_offset_file *off_data = file->data;
  off_data->parent = NULL;
  holy_file_close (file);
}

holy_file_t
holy_file_offset_open (holy_file_t parent, holy_off_t start, holy_off_t size)
{
  struct holy_offset_file *off_data;
  holy_file_t off_file, last_off_file;
  holy_file_filter_id_t filter;

  off_file = holy_zalloc (sizeof (*off_file));
  off_data = holy_zalloc (sizeof (*off_data));
  if (!off_file || !off_data)
    {
      holy_free (off_file);
      holy_free (off_data);
      return 0;
    }

  off_data->off = start;
  off_data->parent = parent;

  off_file->device = parent->device;
  off_file->data = off_data;
  off_file->fs = &holy_offset_fs;
  off_file->size = size;

  last_off_file = NULL;
  for (filter = holy_FILE_FILTER_COMPRESSION_FIRST;
       off_file && filter <= holy_FILE_FILTER_COMPRESSION_LAST; filter++)
    if (holy_file_filters_enabled[filter])
      {
	last_off_file = off_file;
	off_file = holy_file_filters_enabled[filter] (off_file, parent->name);
      }

  if (!off_file)
    {
      off_data->parent = NULL;
      holy_file_close (last_off_file);
      return 0;
    }
  return off_file;
}
