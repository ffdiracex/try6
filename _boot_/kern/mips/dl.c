/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/elf.h>
#include <holy/misc.h>
#include <holy/err.h>
#include <holy/cpu/types.h>
#include <holy/mm.h>
#include <holy/i18n.h>

/* Dummy __gnu_local_gp. Resolved by linker.  */
static char __gnu_local_gp_dummy;
static char _gp_disp_dummy;

/* Check if EHDR is a valid ELF header.  */
holy_err_t
holy_arch_dl_check_header (void *ehdr)
{
  Elf_Ehdr *e = ehdr;

  /* Check the magic numbers.  */
#ifdef holy_CPU_WORDS_BIGENDIAN
  if (e->e_ident[EI_CLASS] != ELFCLASS32
      || e->e_ident[EI_DATA] != ELFDATA2MSB
      || e->e_machine != EM_MIPS)
#else
  if (e->e_ident[EI_CLASS] != ELFCLASS32
      || e->e_ident[EI_DATA] != ELFDATA2LSB
      || e->e_machine != EM_MIPS)
#endif
    return holy_error (holy_ERR_BAD_OS, N_("invalid arch-dependent ELF magic"));

  return holy_ERR_NONE;
}

#pragma GCC diagnostic ignored "-Wcast-align"

holy_err_t
holy_arch_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
				 holy_size_t *got)
{
  const Elf_Ehdr *e = ehdr;
  const Elf_Shdr *s;
  /* FIXME: suboptimal.  */
  holy_size_t gp_size = 0;
  unsigned i;

  *tramp = 0;
  *got = 0;

  for (i = 0, s = (const Elf_Shdr *) ((const char *) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (const Elf_Shdr *) ((const char *) s + e->e_shentsize))
    if (s->sh_type == SHT_REL)
      {
	const Elf_Rel *rel, *max;

	for (rel = (const Elf_Rel *) ((const char *) e + s->sh_offset),
	       max = rel + s->sh_size / s->sh_entsize;
	     rel < max;
	     rel++)
	  switch (ELF_R_TYPE (rel->r_info))
	    {
	    case R_MIPS_GOT16:
	    case R_MIPS_CALL16:
	    case R_MIPS_GPREL32:
	      gp_size += 4;
	      break;
	    }
      }

  if (gp_size > 0x08000)
    return holy_error (holy_ERR_OUT_OF_RANGE, "__gnu_local_gp is too big\n");

  *got = gp_size;

  return holy_ERR_NONE;
}

/* Relocate symbols.  */
holy_err_t
holy_arch_dl_relocate_symbols (holy_dl_t mod, void *ehdr,
			       Elf_Shdr *s, holy_dl_segment_t seg)
{
  holy_uint32_t gp0;
  Elf_Ehdr *e = ehdr;

  if (!mod->reginfo)
    {
      unsigned i;
      Elf_Shdr *ri;

      /* Find reginfo. */
      for (i = 0, ri = (Elf_Shdr *) ((char *) ehdr + e->e_shoff);
	   i < e->e_shnum;
	   i++, ri = (Elf_Shdr *) ((char *) ri + e->e_shentsize))
	if (ri->sh_type == SHT_MIPS_REGINFO)
	  break;
      if (i == e->e_shnum)
	return holy_error (holy_ERR_BAD_MODULE, "no reginfo found");
      mod->reginfo = (holy_uint32_t *)((char *) ehdr + ri->sh_offset);
    }

  gp0 = mod->reginfo[5];
  Elf_Rel *rel, *max;

  for (rel = (Elf_Rel *) ((char *) e + s->sh_offset),
	 max = (Elf_Rel *) ((char *) rel + s->sh_size);
       rel < max;
       rel = (Elf_Rel *) ((char *) rel + s->sh_entsize))
    {
      holy_uint8_t *addr;
      Elf_Sym *sym;
      holy_uint32_t sym_value;

      if (seg->size < rel->r_offset)
	return holy_error (holy_ERR_BAD_MODULE,
			   "reloc offset is out of the segment");

      addr = (holy_uint8_t *) ((char *) seg->addr + rel->r_offset);
      sym = (Elf_Sym *) ((char *) mod->symtab
			 + mod->symsize * ELF_R_SYM (rel->r_info));
      sym_value = sym->st_value;
      if (s->sh_type == SHT_RELA)
	{
	  sym_value += ((Elf_Rela *) rel)->r_addend;
	}
      if (sym_value == (holy_addr_t) &__gnu_local_gp_dummy)
	sym_value = (holy_addr_t) mod->got;
      else if (sym_value == (holy_addr_t) &_gp_disp_dummy)
	{
	  sym_value = (holy_addr_t) mod->got - (holy_addr_t) addr;
	  if (ELF_R_TYPE (rel->r_info) == R_MIPS_LO16)
	    /* ABI mandates +4 even if partner lui doesn't
	       immediately precede addiu.  */
	    sym_value += 4;
	}
      switch (ELF_R_TYPE (rel->r_info))
	{
	case R_MIPS_HI16:
	  {
	    holy_uint32_t value;
	    Elf_Rel *rel2;

#ifdef holy_CPU_WORDS_BIGENDIAN
	    addr += 2;
#endif

	    /* Handle partner lo16 relocation. Lower part is
	       treated as signed. Hence add 0x8000 to compensate. 
	    */
	    value = (*(holy_uint16_t *) addr << 16)
	      + sym_value + 0x8000;
	    for (rel2 = rel + 1; rel2 < max; rel2++)
	      if (ELF_R_SYM (rel2->r_info)
		  == ELF_R_SYM (rel->r_info)
		  && ELF_R_TYPE (rel2->r_info) == R_MIPS_LO16)
		{
		  value += *(holy_int16_t *)
		    ((char *) seg->addr + rel2->r_offset
#ifdef holy_CPU_WORDS_BIGENDIAN
		     + 2
#endif
		     );
		  break;
		}
	    *(holy_uint16_t *) addr = (value >> 16) & 0xffff;
	  }
	  break;
	case R_MIPS_LO16:
#ifdef holy_CPU_WORDS_BIGENDIAN
	  addr += 2;
#endif
	  *(holy_uint16_t *) addr += sym_value & 0xffff;
	  break;
	case R_MIPS_32:
	  *(holy_uint32_t *) addr += sym_value;
	  break;
	case R_MIPS_GPREL32:
	  *(holy_uint32_t *) addr = sym_value
	    + *(holy_uint32_t *) addr + gp0 - (holy_uint32_t)mod->got;
	  break;

	case R_MIPS_26:
	  {
	    holy_uint32_t value;
	    holy_uint32_t raw;
	    raw = (*(holy_uint32_t *) addr) & 0x3ffffff;
	    value = raw << 2;
	    value += sym_value;
	    raw = (value >> 2) & 0x3ffffff;
			
	    *(holy_uint32_t *) addr =
	      raw | ((*(holy_uint32_t *) addr) & 0xfc000000);
	  }
	  break;
	case R_MIPS_GOT16:
	  if (ELF_ST_BIND (sym->st_info) == STB_LOCAL)
	    {
	      Elf_Rel *rel2;
	      /* Handle partner lo16 relocation. Lower part is
		 treated as signed. Hence add 0x8000 to compensate.
	      */
	      sym_value += (*(holy_uint16_t *) addr << 16)
		+ 0x8000;
	      for (rel2 = rel + 1; rel2 < max; rel2++)
		if (ELF_R_SYM (rel2->r_info)
		    == ELF_R_SYM (rel->r_info)
		    && ELF_R_TYPE (rel2->r_info) == R_MIPS_LO16)
		  {
		    sym_value += *(holy_int16_t *)
		      ((char *) seg->addr + rel2->r_offset
#ifdef holy_CPU_WORDS_BIGENDIAN
		       + 2
#endif
		       );
		    break;
		  }
	      sym_value &= 0xffff0000;
	      *(holy_uint16_t *) addr = 0;
	    }
	  /* Fallthrough.  */
	case R_MIPS_CALL16:
	  {
	    holy_uint32_t *gpptr = mod->gotptr;
	  /* FIXME: reuse*/
#ifdef holy_CPU_WORDS_BIGENDIAN
	    addr += 2;
#endif
	    *gpptr = sym_value + *(holy_uint16_t *) addr;
	    *(holy_uint16_t *) addr
	      = sizeof (holy_uint32_t) * (gpptr - (holy_uint32_t *) mod->got);
	    mod->gotptr = gpptr + 1;
	    break;
	  }
	case R_MIPS_JALR:
	  break;
	default:
	  {
	    return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
			       N_("relocation 0x%x is not implemented yet"),
			       ELF_R_TYPE (rel->r_info));
	  }
	  break;
	}
    }

  return holy_ERR_NONE;
}

void 
holy_arch_dl_init_linker (void)
{
  holy_dl_register_symbol ("__gnu_local_gp", &__gnu_local_gp_dummy, 0, 0);
  holy_dl_register_symbol ("_gp_disp", &_gp_disp_dummy, 0, 0);
}

