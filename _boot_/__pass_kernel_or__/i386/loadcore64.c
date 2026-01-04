/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/efiemu/efiemu.h>
#include <holy/cpu/efiemu.h>
#include <holy/elf.h>
#include <holy/i18n.h>

/* Check if EHDR is a valid ELF header.  */
int
holy_arch_efiemu_check_header64 (void *ehdr)
{
  Elf64_Ehdr *e = ehdr;

  return (e->e_ident[EI_CLASS] == ELFCLASS64
	  && e->e_ident[EI_DATA] == ELFDATA2LSB
	  && e->e_machine == EM_X86_64);
}

/* Relocate symbols.  */
holy_err_t
holy_arch_efiemu_relocate_symbols64 (holy_efiemu_segment_t segs,
				     struct holy_efiemu_elf_sym *elfsyms,
				     void *ehdr)
{
  unsigned i;
  Elf64_Ehdr *e = ehdr;
  Elf64_Shdr *s;
  holy_err_t err;

  for (i = 0, s = (Elf64_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf64_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_RELA)
      {
	holy_efiemu_segment_t seg;
	holy_dprintf ("efiemu", "shtrel\n");

	/* Find the target segment.  */
	for (seg = segs; seg; seg = seg->next)
	  if (seg->section == s->sh_info)
	    break;

	if (seg)
	  {
	    Elf64_Rela *rel, *max;

	    for (rel = (Elf64_Rela *) ((char *) e + s->sh_offset),
		   max = rel + (unsigned long) s->sh_size
		   / (unsigned long)s->sh_entsize;
		 rel < max;
		 rel++)
	      {
		void *addr;
		holy_uint32_t *addr32;
		holy_uint64_t *addr64;
		struct holy_efiemu_elf_sym sym;
		if (seg->size < rel->r_offset)
		  return holy_error (holy_ERR_BAD_MODULE,
				     "reloc offset is out of the segment");

		addr =
		  ((char *) holy_efiemu_mm_obtain_request (seg->handle)
		   + seg->off + rel->r_offset);
		addr32 = (holy_uint32_t *) addr;
		addr64 = (holy_uint64_t *) addr;
		sym = elfsyms[ELF64_R_SYM (rel->r_info)];

		switch (ELF64_R_TYPE (rel->r_info))
		  {
		  case R_X86_64_64:
		    err = holy_efiemu_write_value (addr,
						   *addr64 + rel->r_addend
						   + sym.off, sym.handle,
						   0, seg->ptv_rel_needed,
						   sizeof (holy_uint64_t));
		    if (err)
		      return err;
		    break;

		  case R_X86_64_PC32:
		  case R_X86_64_PLT32:
		    err = holy_efiemu_write_value (addr,
						   *addr32 + rel->r_addend
						   + sym.off
						   - rel->r_offset - seg->off,
						   sym.handle, seg->handle,
						   seg->ptv_rel_needed,
						   sizeof (holy_uint32_t));
		    if (err)
		      return err;
		    break;

                  case R_X86_64_32:
                  case R_X86_64_32S:
		    err = holy_efiemu_write_value (addr,
						   *addr32 + rel->r_addend
						   + sym.off, sym.handle,
						   0, seg->ptv_rel_needed,
						   sizeof (holy_uint32_t));
		    if (err)
		      return err;
                    break;
		  default:
		    return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
				       N_("relocation 0x%x is not implemented yet"),
				       ELF_R_TYPE (rel->r_info));
		  }
	      }
	  }
      }

  return holy_ERR_NONE;
}
