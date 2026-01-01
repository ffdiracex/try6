/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_KERNEL_MACHINE_HEADER
#define holy_KERNEL_MACHINE_HEADER	1

#ifndef ASM_FILE

#include <holy/symbol.h>
#include <holy/types.h>

#endif /* ! ASM_FILE */

#define holy_KERNEL_MACHINE_STACK_SIZE holy_KERNEL_ARM_STACK_SIZE
#define holy_KERNEL_MACHINE_HEAP_SIZE  (holy_size_t) (16 * 1024 * 1024)

#endif /* ! holy_KERNEL_MACHINE_HEADER */
