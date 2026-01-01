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
  Elf_Ehdr *e = ehdr;

  /* Check the magic numbers.  */
  if (e->e_ident[EI_CLASS] != ELFCLASS32
      || e->e_ident[EI_DATA] != ELFDATA2MSB
      || e->e_machine != EM_PPC)
    return holy_error (holy_ERR_BAD_OS, N_("invalid arch-dependent ELF magic"));

  return holy_ERR_NONE;
}

/* For low-endian reverse lis and addr_high as well as ori and addr_low. */
struct trampoline
{
  holy_uint32_t lis;
  holy_uint32_t ori;
  holy_uint32_t mtctr;
  holy_uint32_t bctr;
};

static const struct trampoline trampoline_template = 
  {
    0x3d800000,
    0x618c0000,
    0x7d8903a6,
    0x4e800420,
  };

#pragma GCC diagnostic ignored "-Wcast-align"

holy_err_t
holy_arch_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
				 holy_size_t *got)
{
  const Elf_Ehdr *e = ehdr;
  const Elf_Shdr *s;
  unsigned i;

  *tramp = 0;
  *got = 0;

  for (i = 0, s = (const Elf_Shdr *) ((const char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (const Elf_Shdr *) ((const char *) s + e->e_shentsize))
    if (s->sh_type == SHT_RELA)
      {
	const Elf_Rela *rel, *max;
	
	for (rel = (const Elf_Rela *) ((const char *) e + s->sh_offset),
	       max = rel + s->sh_size / s->sh_entsize;
	     rel < max;
	     rel++)
	  if (ELF_R_TYPE (rel->r_info) == holy_ELF_R_PPC_REL24)
	    (*tramp)++;
	
      }

  *tramp *= sizeof (struct trampoline);

  return holy_ERR_NONE;
}

/* Relocate symbols.  */
holy_err_t
holy_arch_dl_relocate_symbols (holy_dl_t mod, void *ehdr,
			       Elf_Shdr *s, holy_dl_segment_t seg)
{
  Elf_Rela *rel, *max;

  for (rel = (Elf_Rela *) ((char *) ehdr + s->sh_offset),
	 max = (Elf_Rela *) ((char *) rel + s->sh_size);
       rel < max;
       rel = (Elf_Rela *) ((char *) rel + s->sh_entsize))
    {
      Elf_Word *addr;
      Elf_Sym *sym;
      holy_uint32_t value;

      if (seg->size < rel->r_offset)
	return holy_error (holy_ERR_BAD_MODULE,
			   "reloc offset is out of the segment");

      addr = (Elf_Word *) ((char *) seg->addr + rel->r_offset);
      sym = (Elf_Sym *) ((char *) mod->symtab
			 + mod->symsize * ELF_R_SYM (rel->r_info));

      /* On the PPC the value does not have an explicit
	 addend, add it.  */
      value = sym->st_value + rel->r_addend;
      switch (ELF_R_TYPE (rel->r_info))
	{
	case holy_ELF_R_PPC_ADDR16_LO:
	  *(Elf_Half *) addr = value;
	  break;

	case holy_ELF_R_PPC_REL24:
	  {
	    Elf_Sword delta = value - (Elf_Word) addr;

	    if (delta << 6 >> 6 != delta)
	      {
		struct trampoline *tptr = mod->trampptr;
		holy_memcpy (tptr, &trampoline_template,
			     sizeof (*tptr));
		delta = (holy_uint8_t *) tptr - (holy_uint8_t *) addr;
		tptr->lis |= (((value) >> 16) & 0xffff);
		tptr->ori |= ((value) & 0xffff);
		mod->trampptr = tptr + 1;
	      }
			
	    if (delta << 6 >> 6 != delta)
	      return holy_error (holy_ERR_BAD_MODULE,
				 "relocation overflow");
	    *addr = (*addr & 0xfc000003) | (delta & 0x3fffffc);
	    break;
	  }

	case holy_ELF_R_PPC_ADDR16_HA:
	  *(Elf_Half *) addr = (value + 0x8000) >> 16;
	  break;

	case holy_ELF_R_PPC_ADDR32:
	  *addr = value;
	  break;

	case holy_ELF_R_PPC_REL32:
	  *addr = value - (Elf_Word) addr;
	  break;

	default:
	  return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
			     N_("relocation 0x%x is not implemented yet"),
			     ELF_R_TYPE (rel->r_info));
	}
    }

  return holy_ERR_NONE;
}
