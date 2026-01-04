/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/cache.h>

void
holy_arch_sync_caches (void *address, holy_size_t len)
{
  /* Cache line length is at least 32.  */
  len += (holy_uint64_t)address & 0x1f;
  holy_uint64_t a = (holy_uint64_t)address & ~0x1f;

  /* Flush data.  */
  for (len = (len + 31) & ~0x1f; len > 0; len -= 0x20, a += 0x20)
    asm volatile ("fc.i %0" : : "r" (a));
  /* Sync and serialize.  Maybe extra.  */
  asm volatile (";; sync.i;; srlz.i;;");
}
