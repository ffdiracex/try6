/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EFI_DISK_HEADER
#define holy_EFI_DISK_HEADER	1

#include <holy/efi/api.h>
#include <holy/symbol.h>
#include <holy/disk.h>

holy_efi_handle_t
EXPORT_FUNC(holy_efidisk_get_device_handle) (holy_disk_t disk);
char *EXPORT_FUNC(holy_efidisk_get_device_name) (holy_efi_handle_t *handle);

void holy_efidisk_init (void);
void holy_efidisk_fini (void);

#endif /* ! holy_EFI_DISK_HEADER */
