/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_XEN_FILE_HEADER
#define holy_XEN_FILE_HEADER 1

#include <holy/types.h>
#include <holy/elf.h>
#include <holy/elfload.h>

holy_elf_t holy_xen_file (holy_file_t file);
holy_elf_t holy_xen_file_and_cmdline (holy_file_t file,
				      char *cmdline,
				      holy_size_t cmdline_max_len);

struct holy_xen_file_info
{
  holy_uint64_t kern_start, kern_end;
  holy_uint64_t virt_base;
  holy_uint64_t entry_point;
  holy_uint64_t hypercall_page;
  holy_uint64_t paddr_offset;
  holy_uint64_t p2m_base;
  int has_hypercall_page;
  int has_note;
  int has_xen_guest;
  int has_p2m_base;
  int extended_cr3;
  int unmapped_initrd;
  enum
  {
    holy_XEN_FILE_I386 = 1,
    holy_XEN_FILE_I386_PAE = 2,
    holy_XEN_FILE_I386_PAE_BIMODE = 3,
    holy_XEN_FILE_X86_64 = 4
  } arch;
};

holy_err_t
holy_xen_get_info32 (holy_elf_t elf, struct holy_xen_file_info *xi);
holy_err_t
holy_xen_get_info64 (holy_elf_t elf, struct holy_xen_file_info *xi);
holy_err_t holy_xen_get_info (holy_elf_t elf, struct holy_xen_file_info *xi);

#endif
