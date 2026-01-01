/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef _holy_MEMORY_MACHINE_LB_HEADER
#define _holy_MEMORY_MACHINE_LB_HEADER      1

#include <holy/symbol.h>

#ifndef ASM_FILE
#include <holy/err.h>
#include <holy/types.h>
#include <holy/memory.h>
#endif

#include <holy/i386/memory.h>
#include <holy/i386/memory_raw.h>

#ifndef ASM_FILE

void holy_machine_mmap_init (void);

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

#endif /* ! _holy_MEMORY_MACHINE_HEADER */
