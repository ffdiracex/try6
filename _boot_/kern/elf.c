/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/elf.h>
#include <holy/elfload.h>
#include <holy/file.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/dl.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

#pragma GCC diagnostic ignored "-Wcast-align"

#if defined(__powerpc__) && defined(holy_MACHINE_IEEE1275)
#define holy_ELF_ENABLE_BI_ENDIAN 1
#else
#define holy_ELF_ENABLE_BI_ENDIAN 0
#endif

#if defined(holy_CPU_WORDS_BIGENDIAN)
#define holy_ELF_NATIVE_ENDIANNESS ELFDATA2MSB
#define holy_ELF_OPPOSITE_ENDIANNESS ELFDATA2LSB
#else
#define holy_ELF_NATIVE_ENDIANNESS ELFDATA2LSB
#define holy_ELF_OPPOSITE_ENDIANNESS ELFDATA2MSB
#endif

static int holy_elf32_check_endianess_and_bswap_ehdr (holy_elf_t elf);
static int holy_elf64_check_endianess_and_bswap_ehdr (holy_elf_t elf);

/* Check if EHDR is a valid ELF header.  */
static holy_err_t
holy_elf_check_header (holy_elf_t elf)
{
  Elf32_Ehdr *e = &elf->ehdr.ehdr32;

  if (e->e_ident[EI_MAG0] != ELFMAG0
      || e->e_ident[EI_MAG1] != ELFMAG1
      || e->e_ident[EI_MAG2] != ELFMAG2
      || e->e_ident[EI_MAG3] != ELFMAG3
      || e->e_ident[EI_VERSION] != EV_CURRENT)
    return holy_error (holy_ERR_BAD_OS, N_("invalid arch-independent ELF magic"));

  if (holy_elf_is_elf32 (elf))
    {
      if (!holy_elf32_check_endianess_and_bswap_ehdr (elf)) {
	return holy_error (holy_ERR_BAD_OS, "invalid ELF endianness magic");
      }
    }
  else if (holy_elf_is_elf64 (elf))
    {
      if (!holy_elf64_check_endianess_and_bswap_ehdr (elf)) {
	return holy_error (holy_ERR_BAD_OS, "invalid ELF endianness magic");
      }
    }
  else
    return holy_error (holy_ERR_BAD_OS, "unknown ELF class");

  if (e->e_version != EV_CURRENT)
    return holy_error (holy_ERR_BAD_OS, N_("invalid arch-independent ELF magic"));

  return holy_ERR_NONE;
}

holy_err_t
holy_elf_close (holy_elf_t elf)
{
  holy_file_t file = elf->file;

  holy_free (elf->phdrs);
  holy_free (elf->filename);
  holy_free (elf);

  if (file)
    holy_file_close (file);

  return holy_errno;
}

holy_elf_t
holy_elf_file (holy_file_t file, const char *filename)
{
  holy_elf_t elf;

  elf = holy_zalloc (sizeof (*elf));
  if (! elf)
    return 0;

  elf->file = file;

  if (holy_file_seek (elf->file, 0) == (holy_off_t) -1)
    goto fail;

  if (holy_file_read (elf->file, &elf->ehdr, sizeof (elf->ehdr))
      != sizeof (elf->ehdr))
    {
      if (!holy_errno)
	holy_error (holy_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
		    filename);
      goto fail;
    }

  if (holy_elf_check_header (elf))
    goto fail;

  elf->filename = holy_strdup (filename);
  if (!elf->filename)
    goto fail;

  return elf;

fail:
  holy_free (elf->filename);
  holy_free (elf->phdrs);
  holy_free (elf);
  return 0;
}

holy_elf_t
holy_elf_open (const char *name)
{
  holy_file_t file;
  holy_elf_t elf;

  file = holy_file_open (name);
  if (! file)
    return 0;

  elf = holy_elf_file (file, name);
  if (! elf)
    holy_file_close (file);

  return elf;
}


#define holy_swap_bytes_halfXX holy_swap_bytes16
#define holy_swap_bytes_wordXX holy_swap_bytes32

/* 32-bit */
#define ehdrXX ehdr32
#define ELFCLASSXX ELFCLASS32
#define ElfXX_Addr Elf32_Addr
#define holy_elfXX_size holy_elf32_size
#define holy_elfXX_load holy_elf32_load
#define FOR_ELFXX_PHDRS FOR_ELF32_PHDRS
#define holy_elf_is_elfXX holy_elf_is_elf32
#define holy_elfXX_load_phdrs holy_elf32_load_phdrs
#define ElfXX_Phdr Elf32_Phdr
#define ElfXX_Ehdr Elf32_Ehdr
#define holy_uintXX_t holy_uint32_t
#define holy_swap_bytes_addrXX holy_swap_bytes32
#define holy_swap_bytes_offXX holy_swap_bytes32
#define holy_swap_bytes_XwordXX holy_swap_bytes32
#define holy_elfXX_check_endianess_and_bswap_ehdr holy_elf32_check_endianess_and_bswap_ehdr

#include "elfXX.c"

#undef ehdrXX
#undef ELFCLASSXX
#undef ElfXX_Addr
#undef holy_elfXX_size
#undef holy_elfXX_load
#undef FOR_ELFXX_PHDRS
#undef holy_elf_is_elfXX
#undef holy_elfXX_load_phdrs
#undef ElfXX_Phdr
#undef ElfXX_Ehdr
#undef holy_uintXX_t
#undef holy_swap_bytes_addrXX
#undef holy_swap_bytes_offXX
#undef holy_swap_bytes_XwordXX
#undef holy_elfXX_check_endianess_and_bswap_ehdr


/* 64-bit */
#define ehdrXX ehdr64
#define ELFCLASSXX ELFCLASS64
#define ElfXX_Addr Elf64_Addr
#define holy_elfXX_size holy_elf64_size
#define holy_elfXX_load holy_elf64_load
#define FOR_ELFXX_PHDRS FOR_ELF64_PHDRS
#define holy_elf_is_elfXX holy_elf_is_elf64
#define holy_elfXX_load_phdrs holy_elf64_load_phdrs
#define ElfXX_Phdr Elf64_Phdr
#define ElfXX_Ehdr Elf64_Ehdr
#define holy_uintXX_t holy_uint64_t
#define holy_swap_bytes_addrXX holy_swap_bytes64
#define holy_swap_bytes_offXX holy_swap_bytes64
#define holy_swap_bytes_XwordXX holy_swap_bytes64
#define holy_elfXX_check_endianess_and_bswap_ehdr holy_elf64_check_endianess_and_bswap_ehdr

#include "elfXX.c"
