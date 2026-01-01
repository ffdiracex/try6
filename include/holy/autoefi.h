/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_AUTOEFI_HEADER
#define holy_AUTOEFI_HEADER	1


#ifdef holy_MACHINE_EFI
# include <holy/efi/efi.h>
# define holy_autoefi_get_memory_map holy_efi_get_memory_map
# define holy_autoefi_finish_boot_services holy_efi_finish_boot_services
# define holy_autoefi_exit_boot_services holy_efi_exit_boot_services
# define holy_autoefi_system_table holy_efi_system_table
# define holy_autoefi_mmap_iterate holy_machine_mmap_iterate
# define holy_autoefi_set_virtual_address_map holy_efi_set_virtual_address_map
static inline holy_err_t holy_autoefi_prepare (void)
{
  return holy_ERR_NONE;
};
# define SYSTEM_TABLE_SIZEOF(x) (sizeof(holy_efi_system_table->x))
# define SYSTEM_TABLE_VAR(x) ((void *)&(holy_efi_system_table->x))
# define SYSTEM_TABLE_PTR(x) ((void *)(holy_efi_system_table->x))
# define SIZEOF_OF_UINTN sizeof (holy_efi_uintn_t)
# define SYSTEM_TABLE(x) (holy_efi_system_table->x)
# define holy_autoefi_finish_boot_services holy_efi_finish_boot_services
# define EFI_PRESENT 1
#else
# include <holy/efiemu/efiemu.h>
# define holy_autoefi_get_memory_map holy_efiemu_get_memory_map
# define holy_autoefi_finish_boot_services holy_efiemu_finish_boot_services
# define holy_autoefi_exit_boot_services holy_efiemu_exit_boot_services
# define holy_autoefi_system_table holy_efiemu_system_table
# define holy_autoefi_mmap_iterate holy_efiemu_mmap_iterate
# define holy_autoefi_prepare holy_efiemu_prepare
# define holy_autoefi_set_virtual_address_map holy_efiemu_set_virtual_address_map
# define SYSTEM_TABLE_SIZEOF holy_EFIEMU_SYSTEM_TABLE_SIZEOF
# define SYSTEM_TABLE_VAR holy_EFIEMU_SYSTEM_TABLE_VAR
# define SYSTEM_TABLE_PTR holy_EFIEMU_SYSTEM_TABLE_PTR
# define SIZEOF_OF_UINTN holy_EFIEMU_SIZEOF_OF_UINTN
# define SYSTEM_TABLE holy_EFIEMU_SYSTEM_TABLE
# define holy_efi_allocate_pages(x,y) (x)
# define holy_efi_free_pages(x,y) holy_EFI_SUCCESS
# define holy_autoefi_finish_boot_services holy_efiemu_finish_boot_services
# define EFI_PRESENT 1
#endif

#endif
