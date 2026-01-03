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
holy_arch_efiemu_check_header32 (void *ehdr)
{
  Elf32_Ehdr *e = ehdr;

  /* Check the magic numbers.  */
  return (e->e_ident[EI_CLASS] == ELFCLASS32
	  && e->e_ident[EI_DATA] == ELFDATA2LSB
	  && e->e_machine == EM_386);
}

/* Relocate symbols.  */
holy_err_t
holy_arch_efiemu_relocate_symbols32 (holy_efiemu_segment_t segs,
				     struct holy_efiemu_elf_sym *elfsyms,
				     void *ehdr)
{
  unsigned i;
  Elf32_Ehdr *e = ehdr;
  Elf32_Shdr *s;
  holy_err_t err;

  holy_dprintf ("efiemu", "relocating symbols %d %d\n",
		e->e_shoff, e->e_shnum);

  for (i = 0, s = (Elf32_Shdr *) ((char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (Elf32_Shdr *) ((char *) s + e->e_shentsize))
    if (s->sh_type == SHT_REL)
      {
	holy_efiemu_segment_t seg;
	holy_dprintf ("efiemu", "shtrel\n");

	/* Find the target segment.  */
	for (seg = segs; seg; seg = seg->next)
	  if (seg->section == s->sh_info)
	    break;

	if (seg)
	  {
	    Elf32_Rel *rel, *max;

	    for (rel = (Elf32_Rel *) ((char *) e + s->sh_offset),
		   max = rel + s->sh_size / s->sh_entsize;
		 rel < max;
		 rel++)
	      {
		Elf32_Word *addr;
		struct holy_efiemu_elf_sym sym;
		if (seg->size < rel->r_offset)
		  return holy_error (holy_ERR_BAD_MODULE,
				     "reloc offset is out of the segment");

		addr = (Elf32_Word *)
		  ((char *) holy_efiemu_mm_obtain_request (seg->handle)
		   + seg->off + rel->r_offset);
		sym = elfsyms[ELF32_R_SYM (rel->r_info)];

		switch (ELF32_R_TYPE (rel->r_info))
		  {
		  case R_386_32:
		    err = holy_efiemu_write_value (addr, sym.off + *addr,
						   sym.handle, 0,
						   seg->ptv_rel_needed,
						   sizeof (holy_uint32_t));
		    if (err)
		      return err;

		    break;

		  case R_386_PC32:
		    err = holy_efiemu_write_value (addr, sym.off + *addr
						   - rel->r_offset
						   - seg->off, sym.handle,
						   seg->handle,
						   seg->ptv_rel_needed,
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


