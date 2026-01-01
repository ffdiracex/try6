/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EFI_EMU_RUNTIME_HEADER
#define holy_EFI_EMU_RUNTIME_HEADER	1

struct holy_efiemu_ptv_rel
{
  holy_uint64_t addr;
  holy_efi_memory_type_t plustype;
  holy_efi_memory_type_t minustype;
  holy_uint32_t size;
} holy_PACKED;

struct efi_variable
{
  holy_efi_packed_guid_t guid;
  holy_uint32_t namelen;
  holy_uint32_t size;
  holy_efi_uint32_t attributes;
} holy_PACKED;
#endif /* ! holy_EFI_EMU_RUNTIME_HEADER */
