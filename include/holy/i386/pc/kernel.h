/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef KERNEL_MACHINE_HEADER
#define KERNEL_MACHINE_HEADER	1

#include <holy/offsets.h>

/* Enable LZMA compression */
#define ENABLE_LZMA	1

#ifndef ASM_FILE

#include <holy/symbol.h>
#include <holy/types.h>

/* The total size of module images following the kernel.  */
extern holy_int32_t holy_total_module_size;

extern holy_uint32_t EXPORT_VAR(holy_boot_device);

extern void (*EXPORT_VAR(holy_pc_net_config)) (char **device, char **path);

#endif /* ! ASM_FILE */

#endif /* ! KERNEL_MACHINE_HEADER */
