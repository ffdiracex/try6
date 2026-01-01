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

void
fletcher_2(const void *buf, holy_uint64_t size, holy_zfs_endian_t endian,
	   zio_cksum_t *zcp)
{
  const holy_uint64_t *ip = buf;
  const holy_uint64_t *ipend = ip + (size / sizeof (holy_uint64_t));
  holy_uint64_t a0, b0, a1, b1;
  
  for (a0 = b0 = a1 = b1 = 0; ip < ipend; ip += 2) 
    {
      a0 += holy_zfs_to_cpu64 (ip[0], endian);
      a1 += holy_zfs_to_cpu64 (ip[1], endian);
      b0 += a0;
      b1 += a1;
    }

  zcp->zc_word[0] = holy_cpu_to_zfs64 (a0, endian);
  zcp->zc_word[1] = holy_cpu_to_zfs64 (a1, endian);
  zcp->zc_word[2] = holy_cpu_to_zfs64 (b0, endian);
  zcp->zc_word[3] = holy_cpu_to_zfs64 (b1, endian);
}

void
fletcher_4 (const void *buf, holy_uint64_t size, holy_zfs_endian_t endian,
	    zio_cksum_t *zcp)
{
  const holy_uint32_t *ip = buf;
  const holy_uint32_t *ipend = ip + (size / sizeof (holy_uint32_t));
  holy_uint64_t a, b, c, d;
  
  for (a = b = c = d = 0; ip < ipend; ip++) 
    {
      a += holy_zfs_to_cpu32 (ip[0], endian);;
      b += a;
      c += b;
      d += c;
    }

  zcp->zc_word[0] = holy_cpu_to_zfs64 (a, endian);
  zcp->zc_word[1] = holy_cpu_to_zfs64 (b, endian);
  zcp->zc_word[2] = holy_cpu_to_zfs64 (c, endian);
  zcp->zc_word[3] = holy_cpu_to_zfs64 (d, endian);
}

