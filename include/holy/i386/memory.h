/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MEMORY_CPU_HEADER
#define holy_MEMORY_CPU_HEADER	1

#define PAGE_SHIFT		12

/* The flag for protected mode.  */
#define holy_MEMORY_CPU_CR0_PE_ON		0x1
#define holy_MEMORY_CPU_CR4_PAE_ON		0x00000020
#define holy_MEMORY_CPU_CR4_PSE_ON		0x00000010
#define holy_MEMORY_CPU_CR0_PAGING_ON		0x80000000
#define holy_MEMORY_CPU_AMD64_MSR		0xc0000080
#define holy_MEMORY_CPU_AMD64_MSR_ON		0x00000100

#define holy_MEMORY_MACHINE_UPPER_START			0x100000	/* 1 MiB */
#define holy_MEMORY_MACHINE_LOWER_SIZE			holy_MEMORY_MACHINE_UPPER_START

/* Some PTE definitions. */
#define holy_PAGE_PRESENT			0x00000001
#define holy_PAGE_RW				0x00000002
#define holy_PAGE_USER				0x00000004

#ifndef ASM_FILE

#define holy_MMAP_MALLOC_LOW 1

#include <holy/types.h>

holy_uint64_t holy_mmap_get_upper (void);
holy_uint64_t holy_mmap_get_lower (void);
holy_uint64_t holy_mmap_get_post64 (void);

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
