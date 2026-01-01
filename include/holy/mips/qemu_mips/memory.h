/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MEMORY_MACHINE_HEADER
#define holy_MEMORY_MACHINE_HEADER	1

#ifndef ASM_FILE
#include <holy/symbol.h>
#include <holy/err.h>
#include <holy/types.h>
#endif

#define holy_MACHINE_MEMORY_STACK_HIGH       0x80f00000
#define holy_MACHINE_MEMORY_USABLE       0x81000000

#ifndef ASM_FILE

extern holy_uint32_t holy_arch_memsize __attribute__ ((section(".text")));

static inline holy_err_t
holy_machine_mmap_register (holy_uint64_t start __attribute__ ((unused)),
			    holy_uint64_t size __attribute__ ((unused)),
			    int type __attribute__ ((unused)),
			    int handle __attribute__ ((unused)))
{
  return holy_ERR_NONE;
}
static inline holy_err_t
holy_machine_mmap_unregister (int handle  __attribute__ ((unused)))
{
  return holy_ERR_NONE;
}
#endif

#endif
