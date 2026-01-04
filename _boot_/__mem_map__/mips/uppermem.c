/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/memory.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/cpu/memory.h>

/* Helper for holy_mmap_get_lower.  */
static int
lower_hook (holy_uint64_t addr, holy_uint64_t size, holy_memory_type_t type,
	    void *data)
{
  holy_uint64_t *lower = data;

  if (type != holy_MEMORY_AVAILABLE)
    return 0;
  if (addr == 0)
    *lower = size;
  return 0;
}

holy_uint64_t
holy_mmap_get_lower (void)
{
  holy_uint64_t lower = 0;

  holy_mmap_iterate (lower_hook, &lower);
  if (lower > holy_ARCH_LOWMEMMAXSIZE)
    lower = holy_ARCH_LOWMEMMAXSIZE;
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
  if (addr <= holy_ARCH_HIGHMEMPSTART && addr + size
      > holy_ARCH_HIGHMEMPSTART)
    *upper = addr + size - holy_ARCH_HIGHMEMPSTART;
  return 0;
}

holy_uint64_t
holy_mmap_get_upper (void)
{
  holy_uint64_t upper = 0;

  holy_mmap_iterate (upper_hook, &upper);
  return upper;
}
