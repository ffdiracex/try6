/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/memory.h>
#include <holy/i386/memory.h>
#include <holy/mm.h>
#include <holy/misc.h>

/* Helper for holy_mmap_get_lower.  */
static int
lower_hook (holy_uint64_t addr, holy_uint64_t size, holy_memory_type_t type,
	    void *data)
{
  holy_uint64_t *lower = data;

  if (type != holy_MEMORY_AVAILABLE)
    return 0;
#ifdef holy_MACHINE_COREBOOT
  if (addr <= 0x1000)
#else
  if (addr == 0)
#endif
    *lower = size + addr;
  return 0;
}

holy_uint64_t
holy_mmap_get_lower (void)
{
  holy_uint64_t lower = 0;

  holy_mmap_iterate (lower_hook, &lower);
  if (lower > 0x100000)
    lower =  0x100000;
  return lower;
}

/* Helper for holy_mmap_get_upper.  */
static int
upper_hook (holy_uint64_t addr, holy_uint64_t size, holy_memory_type_t type,
	    void *data)
{
  holy_uint64_t *upper = data;

  if (type != holy_MEMORY_AVAILABLE)
    return 0;
  if (addr <= 0x100000 && addr + size > 0x100000)
    *upper = addr + size - 0x100000;
  return 0;
}

holy_uint64_t
holy_mmap_get_upper (void)
{
  holy_uint64_t upper = 0;

  holy_mmap_iterate (upper_hook, &upper);
  return upper;
}

/* Helper for holy_mmap_get_post64.  */
static int
post64_hook (holy_uint64_t addr, holy_uint64_t size, holy_memory_type_t type,
	     void *data)
{
  holy_uint64_t *post64 = data;
  if (type != holy_MEMORY_AVAILABLE)
    return 0;
  if (addr <= 0x4000000 && addr + size > 0x4000000)
    *post64 = addr + size - 0x4000000;
  return 0;
}

/* Count the continuous bytes after 64 MiB. */
holy_uint64_t
holy_mmap_get_post64 (void)
{
  holy_uint64_t post64 = 0;

  holy_mmap_iterate (post64_hook, &post64);
  return post64;
}
