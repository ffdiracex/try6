/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/file.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/disk.h>
#include <holy/dl.h>
#include <holy/types.h>
#include <holy/zfs/zfs.h>
#include <holy/zfs/zio.h>
#include <holy/zfs/dnode.h>
#include <holy/zfs/uberblock_impl.h>
#include <holy/zfs/vdev_impl.h>
#include <holy/zfs/zio_checksum.h>
#include <holy/zfs/zap_impl.h>
#include <holy/zfs/zap_leaf.h>
#include <holy/zfs/zfs_znode.h>
#include <holy/zfs/dmu.h>
#include <holy/zfs/dmu_objset.h>
#include <holy/zfs/dsl_dir.h>
#include <holy/zfs/dsl_dataset.h>

#define	MATCH_BITS	6
#define	MATCH_MIN	3
#define	OFFSET_MASK	((1 << (16 - MATCH_BITS)) - 1)

/*
 * Decompression Entry - lzjb
 */
#ifndef	NBBY
#define	NBBY	8
#endif

holy_err_t
lzjb_decompress (void *s_start, void *d_start, holy_size_t s_len,
		 holy_size_t d_len);

holy_err_t
lzjb_decompress (void *s_start, void *d_start, holy_size_t s_len,
		 holy_size_t d_len)
{
  holy_uint8_t *src = s_start;
  holy_uint8_t *dst = d_start;
  holy_uint8_t *d_end = (holy_uint8_t *) d_start + d_len;
  holy_uint8_t *s_end = (holy_uint8_t *) s_start + s_len;
  holy_uint8_t *cpy, copymap = 0;
  int copymask = 1 << (NBBY - 1);

  while (dst < d_end && src < s_end)
    {
      if ((copymask <<= 1) == (1 << NBBY))
	{
	  copymask = 1;
	  copymap = *src++;
	}
      if (src >= s_end)
	return holy_error (holy_ERR_BAD_FS, "lzjb decompression failed");
      if (copymap & copymask)
	{
	  int mlen = (src[0] >> (NBBY - MATCH_BITS)) + MATCH_MIN;
	  int offset = ((src[0] << NBBY) | src[1]) & OFFSET_MASK;
	  src += 2;
	  cpy = dst - offset;
	  if (src > s_end || cpy < (holy_uint8_t *) d_start)
	    return holy_error (holy_ERR_BAD_FS, "lzjb decompression failed");
	  while (--mlen >= 0 && dst < d_end)
	    *dst++ = *cpy++;
	}
      else
	*dst++ = *src++;
    }
  if (dst < d_end)
    return holy_error (holy_ERR_BAD_FS, "lzjb decompression failed");
  return holy_ERR_NONE;
}
