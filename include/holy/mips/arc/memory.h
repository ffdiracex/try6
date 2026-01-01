/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MEMORY_MACHINE_HEADER
#define holy_MEMORY_MACHINE_HEADER	1

#ifdef holy_CPU_MIPSEL
#define holy_MACHINE_MEMORY_STACK_HIGH       0x806ffff0
#else
#define holy_MACHINE_MEMORY_STACK_HIGH       0x881ffff0
#endif

#ifndef ASM_FILE

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
