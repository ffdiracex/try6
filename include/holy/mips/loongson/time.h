/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef KERNEL_MACHINE_TIME_HEADER
#define KERNEL_MACHINE_TIME_HEADER	1

#include <holy/symbol.h>
#include <holy/cpu/time.h>

extern holy_uint32_t EXPORT_VAR (holy_arch_busclock) __attribute__ ((section(".text")));

#endif /* ! KERNEL_MACHINE_TIME_HEADER */
