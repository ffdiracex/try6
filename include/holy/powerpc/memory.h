/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MEMORY_CPU_HEADER
#define holy_MEMORY_CPU_HEADER	1

#ifndef ASM_FILE

typedef holy_addr_t holy_phys_addr_t;

static inline holy_phys_addr_t
holy_vtop (void *a)
{
  return (holy_phys_addr_t) a;
}

static inline void *
holy_map_memory (holy_phys_addr_t a, holy_size_t size __attribute__ ((unused)))
{
  return (void *) a;
}

static inline void
holy_unmap_memory (void *a __attribute__ ((unused)),
		   holy_size_t size __attribute__ ((unused)))
{
}

#endif

#endif /* ! holy_MEMORY_CPU_HEADER */
