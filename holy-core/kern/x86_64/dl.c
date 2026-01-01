/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/elf.h>
#include <holy/misc.h>
#include <holy/err.h>
#include <holy/i18n.h>

/* Check if EHDR is a valid ELF header.  */
holy_err_t
holy_arch_dl_check_header (void *ehdr)
{
  Elf64_Ehdr *e = ehdr;

  /* Check the magic numbers.  */
  if (e->e_ident[EI_CLASS] != ELFCLASS64
      || e->e_ident[EI_DATA] != ELFDATA2LSB
      || e->e_machine != EM_X86_64)
    return holy_error (holy_ERR_BAD_OS, N_("invalid arch-dependent ELF magic"));

  return holy_ERR_NONE;
}

/* Relocate symbols.  */
holy_err_t
holy_arch_dl_relocate_symbols (holy_dl_t mod, void *ehdr,
			       Elf_Shdr *s, holy_dl_segment_t seg)
{
  Elf64_Rela *rel, *max;

  for (rel = (Elf64_Rela *) ((char *) ehdr + s->sh_offset),
	 max = (Elf64_Rela *) ((char *) rel + s->sh_size);
       rel < max;
       rel = (Elf64_Rela *) ((char *) rel + s->sh_entsize))
    {
      Elf64_Word *addr32;
      Elf64_Xword *addr64;
      Elf64_Sym *sym;

      if (seg->size < rel->r_offset)
	return holy_error (holy_ERR_BAD_MODULE,
			   "reloc offset is out of the segment");

      addr32 = (Elf64_Word *) ((char *) seg->addr + rel->r_offset);
      addr64 = (Elf64_Xword *) addr32;
      sym = (Elf64_Sym *) ((char *) mod->symtab
			   + mod->symsize * ELF_R_SYM (rel->r_info));

      switch (ELF_R_TYPE (rel->r_info))
	{
	case R_X86_64_64:
	  *addr64 += rel->r_addend + sym->st_value;
	  break;

	case R_X86_64_PC32:
	case R_X86_64_PLT32:
	  {
	    holy_int64_t value;
	    value = ((holy_int32_t) *addr32) + rel->r_addend + sym->st_value -
	      (Elf64_Xword) (holy_addr_t) seg->addr - rel->r_offset;
	    if (value != (holy_int32_t) value)
	      return holy_error (holy_ERR_BAD_MODULE, "relocation out of range");
	    *addr32 = value;
	  }
	  break;

	case R_X86_64_PC64:
	  {
	    *addr64 += rel->r_addend + sym->st_value -
	      (Elf64_Xword) (holy_addr_t) seg->addr - rel->r_offset;
	  }
	  break;

	case R_X86_64_32:
	  {
	    holy_uint64_t value = *addr32 + rel->r_addend + sym->st_value;
	    if (value != (holy_uint32_t) value)
	      return holy_error (holy_ERR_BAD_MODULE, "relocation out of range");
	    *addr32 = value;
	  }
	  break;
	case R_X86_64_32S:
	  {
	    holy_int64_t value = ((holy_int32_t) *addr32) + rel->r_addend + sym->st_value;
	    if (value != (holy_int32_t) value)
	      return holy_error (holy_ERR_BAD_MODULE, "relocation out of range");
	    *addr32 = value;
	  }
	  break;

	default:
	  return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
			     N_("relocation 0x%x is not implemented yet"),
			     ELF_R_TYPE (rel->r_info));
	}
    }

  return holy_ERR_NONE;
}
