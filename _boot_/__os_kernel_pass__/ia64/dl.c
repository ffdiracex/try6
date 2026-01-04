/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/elf.h>
#include <holy/misc.h>
#include <holy/err.h>
#include <holy/mm.h>
#include <holy/i18n.h>
#include <holy/ia64/reloc.h>

#define MASK19 ((1 << 19) - 1)
#define MASK20 ((1 << 20) - 1)

/* Check if EHDR is a valid ELF header.  */
holy_err_t
holy_arch_dl_check_header (void *ehdr)
{
  Elf_Ehdr *e = ehdr;

  /* Check the magic numbers.  */
  if (e->e_ident[EI_CLASS] != ELFCLASS64
      || e->e_ident[EI_DATA] != ELFDATA2LSB
      || e->e_machine != EM_IA_64)
    return holy_error (holy_ERR_BAD_OS, N_("invalid arch-dependent ELF magic"));

  return holy_ERR_NONE;
}

#pragma GCC diagnostic ignored "-Wcast-align"

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
      holy_addr_t addr;
      Elf_Sym *sym;
      holy_uint64_t value;

      if (seg->size < (rel->r_offset & ~3))
	return holy_error (holy_ERR_BAD_MODULE,
			   "reloc offset is out of the segment");

      addr = (holy_addr_t) seg->addr + rel->r_offset;
      sym = (Elf_Sym *) ((char *) mod->symtab
			 + mod->symsize * ELF_R_SYM (rel->r_info));

      /* On the PPC the value does not have an explicit
	 addend, add it.  */
      value = sym->st_value + rel->r_addend;

      switch (ELF_R_TYPE (rel->r_info))
	{
	case R_IA64_PCREL21B:
	  {
	    holy_int64_t noff;
	    if (ELF_ST_TYPE (sym->st_info) == STT_FUNC)
	      {
		struct holy_ia64_trampoline *tr = mod->trampptr;
		holy_ia64_make_trampoline (tr, value);
		noff = ((char *) tr - (char *) (addr & ~3)) >> 4;
		mod->trampptr = tr + 1;
	      }
	    else
	      noff = ((char *) value - (char *) (addr & ~3)) >> 4;

	    if ((noff & ~MASK19) && ((-noff) & ~MASK19))
	      return holy_error (holy_ERR_BAD_MODULE,
				 "jump offset too big (%lx)", noff);
	    holy_ia64_add_value_to_slot_20b (addr, noff);
	  }
	  break;
	case R_IA64_SEGREL64LSB:
	  *(holy_uint64_t *) addr += value - (holy_addr_t) seg->addr;
	  break;
	case R_IA64_FPTR64LSB:
	case R_IA64_DIR64LSB:
	  *(holy_uint64_t *) addr += value;
	  break;
	case R_IA64_PCREL64LSB:
	  *(holy_uint64_t *) addr += value - addr;
	  break;
	case R_IA64_GPREL64I:
	  holy_ia64_set_immu64 (addr, value - (holy_addr_t) mod->base);
	  break;
	case R_IA64_GPREL22:
	  if ((value - (holy_addr_t) mod->base) & ~MASK20)
	    return holy_error (holy_ERR_BAD_MODULE,
			       "gprel offset too big (%lx)",
			       value - (holy_addr_t) mod->base);
	  holy_ia64_add_value_to_slot_21 (addr, value - (holy_addr_t) mod->base);
	  break;

	case R_IA64_LTOFF22X:
	case R_IA64_LTOFF22:
	  if (ELF_ST_TYPE (sym->st_info) == STT_FUNC)
	    value = *(holy_uint64_t *) sym->st_value + rel->r_addend;
	  /* Fallthrough.  */
	case R_IA64_LTOFF_FPTR22:
	  {
	    holy_uint64_t *gpptr = mod->gotptr;
	    *gpptr = value;
	    if (((holy_addr_t) gpptr - (holy_addr_t) mod->base) & ~MASK20)
	      return holy_error (holy_ERR_BAD_MODULE,
				 "gprel offset too big (%lx)",
				 (holy_addr_t) gpptr - (holy_addr_t) mod->base);
	    holy_ia64_add_value_to_slot_21 (addr, (holy_addr_t) gpptr - (holy_addr_t) mod->base);
	    mod->gotptr = gpptr + 1;
	    break;
	  }
	  /* We treat LTOFF22X as LTOFF22, so we can ignore LDXMOV.  */
	case R_IA64_LDXMOV:
	  break;
	default:
	  return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
			     N_("relocation 0x%x is not implemented yet"),
			     ELF_R_TYPE (rel->r_info));
	}
    }
  return holy_ERR_NONE;
}
