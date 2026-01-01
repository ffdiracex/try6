/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_FDTLOAD_CPU_HEADER
#define holy_FDTLOAD_CPU_HEADER 1

#include <holy/types.h>
#include <holy/err.h>

void *
holy_fdt_load (holy_size_t additional_size);
void
holy_fdt_unload (void);
holy_err_t
holy_fdt_install (void);

#define holy_EFI_PAGE_SHIFT	12
#define holy_EFI_BYTES_TO_PAGES(bytes)   (((bytes) + 0xfff) >> holy_EFI_PAGE_SHIFT)

#endif
