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
  if (e->e_ident[EI_CLASS] != ELFCLASS64
      || e->e_ident[EI_DATA] != ELFDATA2MSB
      || e->e_machine != EM_SPARCV9)
    return holy_error (holy_ERR_BAD_OS, N_("invalid arch-dependent ELF magic"));

  return holy_ERR_NONE;
}

#pragma GCC diagnostic ignored "-Wcast-align"

struct trampoline
{
  holy_uint8_t code[0x28];
  holy_uint64_t addr;
};

static const holy_uint8_t trampoline_code[0x28] =
{
  /* 0:  */ 0x82, 0x10, 0x00, 0x0f,	/* mov  %o7, %g1 */
  /* 4:  */ 0x40, 0x00, 0x00, 0x02, 	/* call  0xc */
  /* 8:  */ 0x01, 0x00, 0x00, 0x00, 	/* nop */
  /* c:  */ 0x9e, 0x1b, 0xc0, 0x01, 	/* xor  %o7, %g1, %o7 */
  /* 10: */ 0x82, 0x18, 0x40, 0x0f, 	/* xor  %g1, %o7, %g1 */
  /* 14: */ 0x9e, 0x1b, 0xc0, 0x01, 	/* xor  %o7, %g1, %o7 */
  /* 18: */ 0xc2, 0x58, 0x60, 0x24, 	/* ldx  [ %g1 + 0x24 ], %g1 */
  /* 1c: */ 0x81, 0xc0, 0x40, 0x00, 	/* jmp  %g1 */
  /* 20: */ 0x01, 0x00, 0x00, 0x00, 	/* nop */
  /* 24: */ 0x01, 0x00, 0x00, 0x00, 	/* nop */
};

holy_err_t
holy_arch_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
				 holy_size_t *got)
{
  const Elf_Ehdr *e = ehdr;
  const Elf_Shdr *s;
  unsigned i;

  *tramp = 0;
  *got = 0;

  for (i = 0, s = (const Elf_Shdr *) ((holy_addr_t) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (const Elf_Shdr *) ((holy_addr_t) s + e->e_shentsize))
    if (s->sh_type == SHT_REL || s->sh_type == SHT_RELA)
      {
	const Elf_Rel *rel, *max;

	for (rel = (const Elf_Rel *) ((holy_addr_t) e + s->sh_offset),
	       max = rel + s->sh_size / s->sh_entsize;
	     rel < max;
	     rel = (const Elf_Rel *) ((holy_addr_t) rel + s->sh_entsize))
	  switch (ELF_R_TYPE (rel->r_info))
	    {
	    case R_SPARC_WDISP30:
	      {
		*tramp += sizeof (struct trampoline);
		break;
	      }
	    }
      }

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
      Elf_Addr value;

      if (seg->size < rel->r_offset)
	return holy_error (holy_ERR_BAD_MODULE,
			   "reloc offset is out of the segment");

      addr = (Elf_Word *) ((char *) seg->addr + rel->r_offset);
      sym = (Elf_Sym *) ((char *) mod->symtab
			 + mod->symsize * ELF_R_SYM (rel->r_info));

      value = sym->st_value + rel->r_addend;
      switch (ELF_R_TYPE (rel->r_info) & 0xff)
	{
	case R_SPARC_32: /* 3 V-word32 */
	  if (value & 0xFFFFFFFF00000000)
	    return holy_error (holy_ERR_BAD_MODULE,
			       "address out of 32 bits range");
	  *addr = value;
	  break;
	case R_SPARC_WDISP30: /* 7 V-disp30 */
	  if (((value - (Elf_Addr) addr) & 0xFFFFFFFF00000000) &&
	       (((value - (Elf_Addr) addr) & 0xFFFFFFFF00000000)
		!= 0xFFFFFFFF00000000))
	    {
	      struct trampoline *tp = mod->trampptr;
	      mod->trampptr = tp + 1;
	      holy_memcpy (tp->code, trampoline_code, sizeof (tp->code));
	      tp->addr = value;
	      value = (Elf_Addr) tp;
	    }

	  if (((value - (Elf_Addr) addr) & 0xFFFFFFFF00000000) &&
	      (((value - (Elf_Addr) addr) & 0xFFFFFFFF00000000)
	       != 0xFFFFFFFF00000000))
	    return holy_error (holy_ERR_BAD_MODULE,
			       "displacement out of 30 bits range");
	  *addr = (*addr & 0xC0000000) |
	    (((holy_int32_t) ((value - (Elf_Addr) addr) >> 2)) &
	     0x3FFFFFFF);
	  break;
	case R_SPARC_HH22: /* 9 V-imm22 */
	  *addr = (*addr & 0xFFC00000) | ((value >> 42) & 0x3FFFFF);
	  break;
	case R_SPARC_HM10: /* 12 T-simm13 */
	  *addr = (*addr & 0xFFFFFC00) | ((value >> 32) & 0x3FF);
	  break;
	case R_SPARC_HI22: /* 9 V-imm22 */
	  if (value >> 32)
	    return holy_error (holy_ERR_BAD_MODULE,
			       "address out of 32 bits range");
	  /* Fallthrough.  */
	case R_SPARC_LM22:
	  *addr = (*addr & 0xFFC00000) | ((value >> 10) & 0x3FFFFF);
	  break;
	case R_SPARC_LO10: /* 12 T-simm13 */
	  *addr = (*addr & 0xFFFFFC00) | (value & 0x3FF);
	  break;
	case R_SPARC_64: /* 32 V-xwords64 */
	  *(Elf_Xword *) addr = value;
	  break;
	case R_SPARC_OLO10:
	  *addr = (*addr & ~0x1fff)
	    | (((value & 0x3ff) +
		(ELF_R_TYPE (rel->r_info) >> 8))
	       & 0x1fff);
	  break;
	default:
	  return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
			     N_("relocation 0x%x is not implemented yet"),
			     ELF_R_TYPE (rel->r_info));
	}
    }

  return holy_ERR_NONE;
}
