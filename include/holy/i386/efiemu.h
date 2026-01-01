/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_ARCH_EFI_EMU_HEADER
#define holy_ARCH_EFI_EMU_HEADER	1

holy_err_t
holy_arch_efiemu_relocate_symbols32 (holy_efiemu_segment_t segs,
				     struct holy_efiemu_elf_sym *elfsyms,
				     void *ehdr);
holy_err_t
holy_arch_efiemu_relocate_symbols64 (holy_efiemu_segment_t segs,
				     struct holy_efiemu_elf_sym *elfsyms,
				     void *ehdr);

int holy_arch_efiemu_check_header32 (void *ehdr);
int holy_arch_efiemu_check_header64 (void *ehdr);
#endif
