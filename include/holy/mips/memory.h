/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MEMORY_CPU_HEADER
#define holy_MEMORY_CPU_HEADER	1

#ifndef ASM_FILE
#include <holy/symbol.h>
#include <holy/err.h>
#include <holy/types.h>
#endif

#define holy_ARCH_LOWMEMVSTART 0x80000000
#define holy_ARCH_LOWMEMPSTART 0x00000000
#define holy_ARCH_LOWMEMMAXSIZE 0x10000000
#define holy_ARCH_HIGHMEMPSTART 0x10000000

#ifndef ASM_FILE

typedef holy_addr_t holy_phys_addr_t;

static inline holy_phys_addr_t
holy_vtop (void *a)
{
  return ((holy_phys_addr_t) a) & 0x1fffffff;
}

static inline void *
holy_map_memory (holy_phys_addr_t a, holy_size_t size __attribute__ ((unused)))
{
  return (void *) (a | 0x80000000);
}

static inline void
holy_unmap_memory (void *a __attribute__ ((unused)),
		   holy_size_t size __attribute__ ((unused)))
{
}

holy_uint64_t holy_mmap_get_lower (void);
holy_uint64_t holy_mmap_get_upper (void);

#endif

#endif
