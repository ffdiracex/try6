/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MEMORY_MACHINE_HEADER
#define holy_MEMORY_MACHINE_HEADER	1

#include <holy/err.h>
#include <holy/types.h>

#define holy_MMAP_REGISTER_BY_FIRMWARE  1

holy_err_t holy_machine_mmap_register (holy_uint64_t start, holy_uint64_t size,
				       int type, int handle);
holy_err_t holy_machine_mmap_unregister (int handle);

#endif /* ! holy_MEMORY_MACHINE_HEADER */
