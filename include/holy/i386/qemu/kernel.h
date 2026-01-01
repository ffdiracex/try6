/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_KERNEL_MACHINE_HEADER
#define holy_KERNEL_MACHINE_HEADER	1

#include <holy/offsets.h>

#ifndef ASM_FILE

#include <holy/symbol.h>
#include <holy/types.h>

extern holy_addr_t holy_core_entry_addr;

void holy_qemu_init_cirrus (void);

#endif /* ! ASM_FILE */

#endif /* ! holy_KERNEL_MACHINE_HEADER */
