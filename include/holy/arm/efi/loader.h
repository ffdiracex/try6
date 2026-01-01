/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_LOADER_MACHINE_HEADER
#define holy_LOADER_MACHINE_HEADER	1

holy_err_t EXPORT_FUNC (holy_efi_prepare_platform) (void);
void * EXPORT_FUNC (holy_efi_allocate_loader_memory) (holy_uint32_t min_offset,
						      holy_uint32_t size);

#endif /* ! holy_LOADER_MACHINE_HEADER */
