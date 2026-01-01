/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/file.h>
#include <holy/fs.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

#include "xz.h"
#include "xz_stream.h"

#define XZBUFSIZ 0x2000
#define VLI_MAX_DIGITS 9
#define XZ_STREAM_FOOTER_SIZE 12

struct holy_xzio
{
  holy_file_t file;
  struct xz_buf buf;
  struct xz_dec *dec;
  holy_uint8_t inbuf[XZBUFSIZ];
  holy_uint8_t outbuf[XZBUFSIZ];
  holy_off_t saved_offset;
};

typedef struct holy_xzio *holy_xzio_t;
static struct holy_fs holy_xzio_fs;

static holy_size_t
decode_vli (const holy_uint8_t buf[], holy_size_t size_max,
	    holy_uint64_t *num)
{
  if (size_max == 0)
    return 0;

  if (size_max > VLI_MAX_DIGITS)
    size_max = VLI_MAX_DIGITS;

  *num = buf[0] & 0x7F;
  holy_size_t i = 0;

  while (buf[i++] & 0x80)
    {
      if (i >= size_max || buf[i] == 0x00)
	return 0;

      *num |= (uint64_t) (buf[i] & 0x7F) << (i * 7);
    }

  return i;
}

static holy_ssize_t
read_vli (holy_file_t file, holy_uint64_t *num)
{
  holy_uint8_t buf[VLI_MAX_DIGITS];
  holy_ssize_t read_bytes;
  holy_size_t dec;

  read_bytes = holy_file_read (file, buf, VLI_MAX_DIGITS);
  if (read_bytes < 0)
    return -1;

  dec = decode_vli (buf, read_bytes, num);
  holy_file_seek (file, file->offset - (read_bytes - dec));
  return dec;
}

/* Function xz_dec_run() should consume header and ask for more (XZ_OK)
 * else file is corrupted (or options not supported) or not xz.  */
static int
test_header (holy_file_t file)
{
  holy_xzio_t xzio = file->data;
  enum xz_ret ret;

  xzio->buf.in_size = holy_file_read (xzio->file, xzio->inbuf,
				      STREAM_HEADER_SIZE);

  if (xzio->buf.in_size != STREAM_HEADER_SIZE)
    return 0;

  ret = xz_dec_run (xzio->dec, &xzio->buf);

  if (ret == XZ_FORMAT_ERROR)
    return 0;

  if (ret != XZ_OK)
    return 0;

  return 1;
}

/* Try to find out size of uncompressed data,
 * also do some footer sanity checks.  */
static int
test_footer (holy_file_t file)
{
  holy_xzio_t xzio = file->data;
  holy_uint8_t footer[FOOTER_MAGIC_SIZE];
  holy_uint32_t backsize;
  holy_uint8_t imarker;
  holy_uint64_t uncompressed_size_total = 0;
  holy_uint64_t uncompressed_size;
  holy_uint64_t records;

  holy_file_seek (xzio->file, xzio->file->size - FOOTER_MAGIC_SIZE);
  if (holy_file_read (xzio->file, footer, FOOTER_MAGIC_SIZE)
      != FOOTER_MAGIC_SIZE
      || holy_memcmp (footer, FOOTER_MAGIC, FOOTER_MAGIC_SIZE) != 0)
    goto ERROR;

  holy_file_seek (xzio->file, xzio->file->size - 8);
  if (holy_file_read (xzio->file, &backsize, sizeof (backsize))
      != sizeof (backsize))
    goto ERROR;

  /* Calculate real backward size.  */
  backsize = (holy_le_to_cpu32 (backsize) + 1) * 4;

  /* Set file to the beginning of stream index.  */
  holy_file_seek (xzio->file,
		  xzio->file->size - XZ_STREAM_FOOTER_SIZE - backsize);

  /* Test index marker.  */
  if (holy_file_read (xzio->file, &imarker, sizeof (imarker))
      != sizeof (imarker) && imarker != 0x00)
    goto ERROR;

  if (read_vli (xzio->file, &records) <= 0)
    goto ERROR;

  for (; records != 0; records--)
    {
      if (read_vli (xzio->file, &uncompressed_size) <= 0)	/* Ignore unpadded.  */
	goto ERROR;
      if (read_vli (xzio->file, &uncompressed_size) <= 0)	/* Uncompressed.  */
	goto ERROR;

      uncompressed_size_total += uncompressed_size;
    }

  file->size = uncompressed_size_total;
  holy_file_seek (xzio->file, STREAM_HEADER_SIZE);
  return 1;

ERROR:
  return 0;
}

static holy_file_t
holy_xzio_open (holy_file_t io,
		const char *name __attribute__ ((unused)))
{
  holy_file_t file;
  holy_xzio_t xzio;

  file = (holy_file_t) holy_zalloc (sizeof (*file));
  if (!file)
    return 0;

  xzio = holy_zalloc (sizeof (*xzio));
  if (!xzio)
    {
      holy_free (file);
      return 0;
    }

  xzio->file = io;

  file->device = io->device;
  file->data = xzio;
  file->fs = &holy_xzio_fs;
  file->size = holy_FILE_SIZE_UNKNOWN;
  file->not_easily_seekable = 1;

  if (holy_file_tell (xzio->file) != 0)
    holy_file_seek (xzio->file, 0);

  /* Allocated 64KiB for dictionary.
   * Decoder will relocate if bigger is needed.  */
  xzio->dec = xz_dec_init (1 << 16);
  if (!xzio->dec)
    {
      holy_free (file);
      holy_free (xzio);
      return 0;
    }

  xzio->buf.in = xzio->inbuf;
  xzio->buf.out = xzio->outbuf;
  xzio->buf.out_size = XZBUFSIZ;

  /* FIXME: don't test footer on not easily seekable files.  */
  if (!test_header (file) || !test_footer (file))
    {
      holy_errno = holy_ERR_NONE;
      holy_file_seek (io, 0);
      xz_dec_end (xzio->dec);
      holy_free (xzio);
      holy_free (file);

      return io;
    }

  return file;
}

static holy_ssize_t
holy_xzio_read (holy_file_t file, char *buf, holy_size_t len)
{
  holy_ssize_t ret = 0;
  holy_ssize_t readret;
  enum xz_ret xzret;
  holy_xzio_t xzio = file->data;
  holy_off_t current_offset;

  /* If seek backward need to reset decoder and start from beginning of file.
     TODO Possible improvement by jumping blocks.  */
  if (file->offset < xzio->saved_offset)
    {
      xz_dec_reset (xzio->dec);
      xzio->saved_offset = 0;
      xzio->buf.out_pos = 0;
      xzio->buf.in_pos = 0;
      xzio->buf.in_size = 0;
      holy_file_seek (xzio->file, 0);
    }

  current_offset = xzio->saved_offset;

  while (len > 0)
    {
      xzio->buf.out_size = file->offset + ret + len - current_offset;
      if (xzio->buf.out_size > XZBUFSIZ)
	xzio->buf.out_size = XZBUFSIZ;
      /* Feed input.  */
      if (xzio->buf.in_pos == xzio->buf.in_size)
	{
	  readret = holy_file_read (xzio->file, xzio->inbuf, XZBUFSIZ);
	  if (readret < 0)
	    return -1;
	  xzio->buf.in_size = readret;
	  xzio->buf.in_pos = 0;
	}

      xzret = xz_dec_run (xzio->dec, &xzio->buf);
      switch (xzret)
	{
	case XZ_MEMLIMIT_ERROR:
	case XZ_FORMAT_ERROR:
	case XZ_OPTIONS_ERROR:
	case XZ_DATA_ERROR:
	case XZ_BUF_ERROR:
	  holy_error (holy_ERR_BAD_COMPRESSED_DATA,
		      N_("xz file corrupted or unsupported block options"));
	  return -1;
	default:
	  break;
	}

      {
	holy_off_t new_offset = current_offset + xzio->buf.out_pos;
	
	if (file->offset <= new_offset)
	  /* Store first chunk of data in buffer.  */
	  {
	    holy_size_t delta = new_offset - (file->offset + ret);
	    holy_memmove (buf, xzio->buf.out + (xzio->buf.out_pos - delta),
			  delta);
	    len -= delta;
	    buf += delta;
	    ret += delta;
	  }
	current_offset = new_offset;
      }
      xzio->buf.out_pos = 0;

      if (xzret == XZ_STREAM_END)	/* Stream end, EOF.  */
	break;
    }

  if (ret >= 0)
    xzio->saved_offset = file->offset + ret;

  return ret;
}

/* Release everything, including the underlying file object.  */
static holy_err_t
holy_xzio_close (holy_file_t file)
{
  holy_xzio_t xzio = file->data;

  xz_dec_end (xzio->dec);

  holy_file_close (xzio->file);
  holy_free (xzio);

  /* Device must not be closed twice.  */
  file->device = 0;
  file->name = 0;
  return holy_errno;
}

static struct holy_fs holy_xzio_fs = {
  .name = "xzio",
  .dir = 0,
  .open = 0,
  .read = holy_xzio_read,
  .close = holy_xzio_close,
  .label = 0,
  .next = 0
};

holy_MOD_INIT (xzio)
{
  holy_file_filter_register (holy_FILE_FILTER_XZIO, holy_xzio_open);
}

holy_MOD_FINI (xzio)
{
  holy_file_filter_unregister (holy_FILE_FILTER_XZIO);
}
