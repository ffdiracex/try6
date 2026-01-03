/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/types.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/fs.h>
#include <holy/bufio.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

#define holy_BUFIO_DEF_SIZE	8192
#define holy_BUFIO_MAX_SIZE	1048576

struct holy_bufio
{
  holy_file_t file;
  holy_size_t block_size;
  holy_size_t buffer_len;
  holy_off_t buffer_at;
  char buffer[0];
};
typedef struct holy_bufio *holy_bufio_t;

static struct holy_fs holy_bufio_fs;

holy_file_t
holy_bufio_open (holy_file_t io, int size)
{
  holy_file_t file;
  holy_bufio_t bufio = 0;

  file = (holy_file_t) holy_zalloc (sizeof (*file));
  if (! file)
    return 0;

  if (size == 0)
    size = holy_BUFIO_DEF_SIZE;
  else if (size > holy_BUFIO_MAX_SIZE)
    size = holy_BUFIO_MAX_SIZE;

  if ((size < 0) || ((unsigned) size > io->size))
    size = ((io->size > holy_BUFIO_MAX_SIZE) ? holy_BUFIO_MAX_SIZE :
            io->size);

  bufio = holy_zalloc (sizeof (struct holy_bufio) + size);
  if (! bufio)
    {
      holy_free (file);
      return 0;
    }

  bufio->file = io;
  bufio->block_size = size;

  file->device = io->device;
  file->size = io->size;
  file->data = bufio;
  file->fs = &holy_bufio_fs;
  file->not_easily_seekable = io->not_easily_seekable;

  return file;
}

holy_file_t
holy_buffile_open (const char *name, int size)
{
  holy_file_t io, file;

  io = holy_file_open (name);
  if (! io)
    return 0;

  file = holy_bufio_open (io, size);
  if (! file)
    {
      holy_file_close (io);
      return 0;
    }

  return file;
}

static holy_ssize_t
holy_bufio_read (holy_file_t file, char *buf, holy_size_t len)
{
  holy_size_t res = 0;
  holy_off_t next_buf;
  holy_bufio_t bufio = file->data;
  holy_ssize_t really_read;

  if (file->size == holy_FILE_SIZE_UNKNOWN)
    file->size = bufio->file->size;

  /* First part: use whatever we already have in the buffer.  */
  if ((file->offset >= bufio->buffer_at) &&
      (file->offset < bufio->buffer_at + bufio->buffer_len))
    {
      holy_size_t n;
      holy_uint64_t pos;

      pos = file->offset - bufio->buffer_at;
      n = bufio->buffer_len - pos;
      if (n > len)
        n = len;

      holy_memcpy (buf, &bufio->buffer[pos], n);
      len -= n;
      res += n;

      buf += n;
    }
  if (len == 0)
    return res;

  /* Need to read some more.  */
  next_buf = (file->offset + res + len - 1) & ~((holy_off_t) bufio->block_size - 1);
  /* Now read between file->offset + res and bufio->buffer_at.  */
  if (file->offset + res < next_buf)
    {
      holy_size_t read_now;
      read_now = next_buf - (file->offset + res);
      holy_file_seek (bufio->file, file->offset + res);
      really_read = holy_file_read (bufio->file, buf, read_now);
      if (really_read < 0)
	return -1;
      if (file->size == holy_FILE_SIZE_UNKNOWN)
	file->size = bufio->file->size;
      len -= really_read;
      buf += really_read;
      res += really_read;

      /* Partial read. File ended unexpectedly. Save the last chunk in buffer.
       */
      if (really_read != (holy_ssize_t) read_now)
	{
	  bufio->buffer_len = really_read;
	  if (bufio->buffer_len > bufio->block_size)
	    bufio->buffer_len = bufio->block_size;
	  bufio->buffer_at = file->offset + res - bufio->buffer_len;
	  holy_memcpy (&bufio->buffer[0], buf - bufio->buffer_len,
		       bufio->buffer_len);
	  return res;
	}
    }

  /* Read into buffer.  */
  holy_file_seek (bufio->file, next_buf);
  really_read = holy_file_read (bufio->file, bufio->buffer,
				bufio->block_size);
  if (really_read < 0)
    return -1;
  bufio->buffer_at = next_buf;
  bufio->buffer_len = really_read;

  if (file->size == holy_FILE_SIZE_UNKNOWN)
    file->size = bufio->file->size;

  if (len > bufio->buffer_len)
    len = bufio->buffer_len;
  holy_memcpy (buf, &bufio->buffer[file->offset + res - next_buf], len);
  res += len;

  return res;
}

static holy_err_t
holy_bufio_close (holy_file_t file)
{
  holy_bufio_t bufio = file->data;

  holy_file_close (bufio->file);
  holy_free (bufio);

  file->device = 0;

  return holy_errno;
}

static struct holy_fs holy_bufio_fs =
  {
    .name = "bufio",
    .dir = 0,
    .open = 0,
    .read = holy_bufio_read,
    .close = holy_bufio_close,
    .label = 0,
    .next = 0
  };
