/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_ELFLOAD_HEADER
#define holy_ELFLOAD_HEADER	1

#include <holy/err.h>
#include <holy/elf.h>
#include <holy/file.h>
#include <holy/symbol.h>
#include <holy/types.h>

struct holy_elf_file
{
  holy_file_t file;
  union {
    Elf64_Ehdr ehdr64;
    Elf32_Ehdr ehdr32;
  } ehdr;
  void *phdrs;
  char *filename;
};
typedef struct holy_elf_file *holy_elf_t;

typedef int (*holy_elf32_phdr_iterate_hook_t)
  (holy_elf_t elf, Elf32_Phdr *phdr, void *arg);
typedef int (*holy_elf64_phdr_iterate_hook_t)
  (holy_elf_t elf, Elf64_Phdr *phdr, void *arg);

holy_elf_t holy_elf_open (const char *);
holy_elf_t holy_elf_file (holy_file_t file, const char *filename);
holy_err_t holy_elf_close (holy_elf_t);

int holy_elf_is_elf32 (holy_elf_t);
holy_size_t holy_elf32_size (holy_elf_t,
			     Elf32_Addr *, holy_uint32_t *);
enum holy_elf_load_flags
  {
    holy_ELF_LOAD_FLAGS_NONE = 0,
    holy_ELF_LOAD_FLAGS_LOAD_PT_DYNAMIC = 1,
    holy_ELF_LOAD_FLAGS_BITS = 6,
    holy_ELF_LOAD_FLAGS_ALL_BITS = 0,
    holy_ELF_LOAD_FLAGS_28BITS = 2,
    holy_ELF_LOAD_FLAGS_30BITS = 4,
    holy_ELF_LOAD_FLAGS_62BITS = 6,
  };
holy_err_t holy_elf32_load (holy_elf_t, const char *filename,
			    void *load_offset, enum holy_elf_load_flags flags, holy_addr_t *,
			    holy_size_t *);

int holy_elf_is_elf64 (holy_elf_t);
holy_size_t holy_elf64_size (holy_elf_t,
			     Elf64_Addr *, holy_uint64_t *);
holy_err_t holy_elf64_load (holy_elf_t, const char *filename,
			    void *load_offset, enum holy_elf_load_flags flags, holy_addr_t *,
			    holy_size_t *);
holy_err_t holy_elf32_load_phdrs (holy_elf_t elf);
holy_err_t holy_elf64_load_phdrs (holy_elf_t elf);

#define FOR_ELF32_PHDRS(elf, phdr) \
  for (holy_elf32_load_phdrs (elf), phdr = elf->phdrs; \
       phdr && phdr < (Elf32_Phdr *) elf->phdrs + elf->ehdr.ehdr32.e_phnum; phdr++)
#define FOR_ELF64_PHDRS(elf, phdr) \
  for (holy_elf64_load_phdrs (elf), phdr = elf->phdrs;		\
       phdr && phdr < (Elf64_Phdr *) elf->phdrs + elf->ehdr.ehdr64.e_phnum; phdr++)

#endif /* ! holy_ELFLOAD_HEADER */
