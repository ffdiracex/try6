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
#include <holy/cpu/reloc.h>

#define LDR 0x58000050
#define BR 0xd61f0200


/*
 * Check if EHDR is a valid ELF header.
 */
holy_err_t
holy_arch_dl_check_header (void *ehdr)
{
  Elf_Ehdr *e = ehdr;

  /* Check the magic numbers.  */
  if (e->e_ident[EI_CLASS] != ELFCLASS64
      || e->e_ident[EI_DATA] != ELFDATA2LSB || e->e_machine != EM_AARCH64)
    return holy_error (holy_ERR_BAD_OS,
		       N_("invalid arch-dependent ELF magic"));

  return holy_ERR_NONE;
}

#pragma GCC diagnostic ignored "-Wcast-align"

/*
 * Unified function for both REL and RELA 
 */
holy_err_t
holy_arch_dl_relocate_symbols (holy_dl_t mod, void *ehdr,
			       Elf_Shdr *s, holy_dl_segment_t seg)
{
  Elf_Rel *rel, *max;
  unsigned unmatched_adr_got_page = 0;

  for (rel = (Elf_Rel *) ((char *) ehdr + s->sh_offset),
	 max = (Elf_Rel *) ((char *) rel + s->sh_size);
       rel < max;
       rel = (Elf_Rel *) ((char *) rel + s->sh_entsize))
    {
      Elf_Sym *sym;
      void *place;
      holy_uint64_t sym_addr;

      if (rel->r_offset >= seg->size)
	return holy_error (holy_ERR_BAD_MODULE,
			   "reloc offset is out of the segment");

      sym = (Elf_Sym *) ((char *) mod->symtab
			 + mod->symsize * ELF_R_SYM (rel->r_info));

      sym_addr = sym->st_value;
      if (s->sh_type == SHT_RELA)
	sym_addr += ((Elf_Rela *) rel)->r_addend;

      place = (void *) ((holy_addr_t) seg->addr + rel->r_offset);

      switch (ELF_R_TYPE (rel->r_info))
	{
	case R_AARCH64_ABS64:
	  {
	    holy_uint64_t *abs_place = place;

	    holy_dprintf ("dl", "  reloc_abs64 %p => 0x%016llx\n",
			  place, (unsigned long long) sym_addr);

	    *abs_place = (holy_uint64_t) sym_addr;
	  }
	  break;
	case R_AARCH64_ADD_ABS_LO12_NC:
	  holy_arm64_set_abs_lo12 (place, sym_addr);
	  break;
	case R_AARCH64_LDST64_ABS_LO12_NC:
	  holy_arm64_set_abs_lo12_ldst64 (place, sym_addr);
	  break;
	case R_AARCH64_CALL26:
	case R_AARCH64_JUMP26:
	  {
	    holy_int64_t offset = sym_addr - (holy_uint64_t) place;

	    if (!holy_arm_64_check_xxxx26_offset (offset))
	      {
		struct holy_arm64_trampoline *tp = mod->trampptr;
		mod->trampptr = tp + 1;
		tp->ldr = LDR;
		tp->br = BR;
		tp->addr = sym_addr;
		offset = (holy_uint8_t *) tp - (holy_uint8_t *) place;
	      }

	    if (!holy_arm_64_check_xxxx26_offset (offset))
		return holy_error (holy_ERR_BAD_MODULE,
				   "trampoline out of range");

	    holy_arm64_set_xxxx26_offset (place, offset);
	  }
	  break;
	case R_AARCH64_PREL32:
	  {
	    holy_int64_t value;
	    Elf64_Word *addr32 = place;
	    value = ((holy_int32_t) *addr32) + sym_addr -
	      (Elf64_Xword) (holy_addr_t) seg->addr - rel->r_offset;
	    if (value != (holy_int32_t) value)
	      return holy_error (holy_ERR_BAD_MODULE, "relocation out of range");
	    holy_dprintf("dl", "  reloc_prel32 %p => 0x%016llx\n",
			  place, (unsigned long long) sym_addr);
	    *addr32 = value;
	  }
	  break;
	case R_AARCH64_ADR_GOT_PAGE:
	  {
	    holy_uint64_t *gp = mod->gotptr;
	    Elf_Rela *rel2;
	    holy_int64_t gpoffset = ((holy_uint64_t) gp & ~0xfffULL) - (((holy_uint64_t) place) & ~0xfffULL);
	    *gp = (holy_uint64_t) sym_addr;
	    mod->gotptr = gp + 1;
	    unmatched_adr_got_page++;
	    holy_dprintf("dl", "  reloc_got %p => 0x%016llx (0x%016llx)\n",
			 place, (unsigned long long) sym_addr, (unsigned long long) gp);
	    if (!holy_arm64_check_hi21_signed (gpoffset))
		return holy_error (holy_ERR_BAD_MODULE,
				   "HI21 out of range");
	    holy_arm64_set_hi21(place, gpoffset);
	    for (rel2 = (Elf_Rela *) ((char *) rel + s->sh_entsize);
		 rel2 < (Elf_Rela *) max;
		 rel2 = (Elf_Rela *) ((char *) rel2 + s->sh_entsize))
	      if (ELF_R_SYM (rel2->r_info)
		  == ELF_R_SYM (rel->r_info)
		  && ((Elf_Rela *) rel)->r_addend == rel2->r_addend
		  && ELF_R_TYPE (rel2->r_info) == R_AARCH64_LD64_GOT_LO12_NC)
		{
		  holy_arm64_set_abs_lo12_ldst64 ((void *) ((holy_addr_t) seg->addr + rel2->r_offset),
						  (holy_uint64_t)gp);
		  break;
		}
	    if (rel2 >= (Elf_Rela *) max)
	      return holy_error (holy_ERR_BAD_MODULE,
				 "ADR_GOT_PAGE without matching LD64_GOT_LO12_NC");
	  }
	  break;
	case R_AARCH64_LD64_GOT_LO12_NC:
	  if (unmatched_adr_got_page == 0)
	    return holy_error (holy_ERR_BAD_MODULE,
			       "LD64_GOT_LO12_NC without matching ADR_GOT_PAGE");
	  unmatched_adr_got_page--;
	  break;
	case R_AARCH64_ADR_PREL_PG_HI21:
	  {
	    holy_int64_t offset = (sym_addr & ~0xfffULL) - (((holy_uint64_t) place) & ~0xfffULL);

	    if (!holy_arm64_check_hi21_signed (offset))
		return holy_error (holy_ERR_BAD_MODULE,
				   "HI21 out of range");

	    holy_arm64_set_hi21 (place, offset);
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
