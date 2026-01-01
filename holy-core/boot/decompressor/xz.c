/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/decompressor.h>

#include "xz.h"
#include "xz_stream.h"

void
holy_decompress_core (void *src, void *dst, unsigned long srcsize,
		      unsigned long dstsize)
{
  struct xz_dec *dec;
  struct xz_buf buf;

  find_scratch (src, dst, srcsize, dstsize);

  dec = xz_dec_init (holy_DECOMPRESSOR_DICT_SIZE);

  buf.in = src;
  buf.in_pos = 0;
  buf.in_size = srcsize;
  buf.out = dst;
  buf.out_pos = 0;
  buf.out_size = dstsize;

  while (buf.in_pos != buf.in_size)
    {
      enum xz_ret xzret;
      xzret = xz_dec_run (dec, &buf);
      switch (xzret)
	{
	case XZ_MEMLIMIT_ERROR:
	case XZ_FORMAT_ERROR:
	case XZ_OPTIONS_ERROR:
	case XZ_DATA_ERROR:
	case XZ_BUF_ERROR:
	  return;
	default:
	  break;
	}
    }
}
