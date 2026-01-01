/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_KERNEL_MACHINE_HEADER
#define holy_KERNEL_MACHINE_HEADER	1

#include <holy/symbol.h>
#include <holy/cpu/kernel.h>

#define holy_ARCH_MACHINE_YEELOONG 0
#define holy_ARCH_MACHINE_FULOONG2F 1
#define holy_ARCH_MACHINE_FULOONG2E 2
#define holy_ARCH_MACHINE_YEELOONG_3A 3

#ifndef ASM_FILE

extern holy_uint32_t EXPORT_VAR (holy_arch_machine) __attribute__ ((section(".text")));

#endif

#endif /* ! holy_KERNEL_MACHINE_HEADER */
