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
#include <holy/arm/reloc.h>

struct trampoline_arm
{
#define ARM_LOAD_IP 0xe59fc000
#define ARM_BX 0xe12fff1c
#define ARM_MOV_PC 0xe1a0f00c
  holy_uint32_t load_ip;  /* ldr ip, [pc] */
  holy_uint32_t bx; /* bx ip or mov pc, ip*/
  holy_uint32_t addr;
};

static holy_uint16_t thumb_template[8] =
  {
    0x468c, /* mov	ip, r1 */
    0x4903, /* ldr	r1, [pc, #12]	; (10 <.text+0x10>) */
    /* Exchange R1 and IP in limited Thumb instruction set.
       IP gets negated but we compensate it by C code.  */
                                     /* R1   IP */
                                     /* -A   R1 */
    0x4461, /* add	r1, ip */    /* R1-A R1 */
    0x4249, /* negs	r1, r1 */    /* A-R1 R1 */
    0x448c, /* add	ip, r1 */    /* A-R1 A  */
    0x4249, /* negs	r1, r1 */    /* R1-A A  */
    0x4461, /* add	r1, ip */    /* R1   A  */
    0x4760  /* bx	ip */
  };

struct trampoline_thumb
{
  holy_uint16_t template[8];
  holy_uint32_t neg_addr;
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

  for (i = 0, s = (const Elf_Shdr *) ((holy_addr_t) e + e->e_shoff);
       i < e->e_shnum;
       i++, s = (const Elf_Shdr *) ((holy_addr_t) s + e->e_shentsize))
    if (s->sh_type == SHT_REL)
      {
	const Elf_Rel *rel, *max;

	for (rel = (const Elf_Rel *) ((holy_addr_t) e + s->sh_offset),
	       max = (const Elf_Rel *) ((holy_addr_t) rel + s->sh_size);
	     rel + 1 <= max;
	     rel = (const Elf_Rel *) ((holy_addr_t) rel + s->sh_entsize))
	  switch (ELF_R_TYPE (rel->r_info))
	    {
	    case R_ARM_CALL:
	    case R_ARM_JUMP24:
	      {
		*tramp += sizeof (struct trampoline_arm);
		break;
	      }
	    case R_ARM_THM_CALL:
	    case R_ARM_THM_JUMP24:
	    case R_ARM_THM_JUMP19:
	      {
		*tramp += sizeof (struct trampoline_thumb);
		break;
	      }
	    }
      }

  holy_dprintf ("dl", "trampoline size %x\n", *tramp);

  return holy_ERR_NONE;
}

/*************************************************
 * Runtime dynamic linker with helper functions. *
 *************************************************/
holy_err_t
holy_arch_dl_relocate_symbols (holy_dl_t mod, void *ehdr,
			       Elf_Shdr *s, holy_dl_segment_t seg)
{
  Elf_Rel *rel, *max;

  for (rel = (Elf_Rel *) ((char *) ehdr + s->sh_offset),
	 max = (Elf_Rel *) ((char *) rel + s->sh_size);
       rel < max;
       rel = (Elf_Rel *) ((char *) rel + s->sh_entsize))
    {
      Elf_Addr *target, sym_addr;
      holy_err_t retval;
      Elf_Sym *sym;

      if (seg->size < rel->r_offset)
	return holy_error (holy_ERR_BAD_MODULE,
			   "reloc offset is out of the segment");
      target = (void *) ((char *) seg->addr + rel->r_offset);
      sym = (Elf_Sym *) ((char *) mod->symtab
			 + mod->symsize * ELF_R_SYM (rel->r_info));

      sym_addr = sym->st_value;

      switch (ELF_R_TYPE (rel->r_info))
	{
	case R_ARM_ABS32:
	  {
	    /* Data will be naturally aligned */
	    retval = holy_arm_reloc_abs32 (target, sym_addr);
	    if (retval != holy_ERR_NONE)
	      return retval;
	  }
	  break;
	case R_ARM_CALL:
	case R_ARM_JUMP24:
	  {
	    holy_int32_t offset;

	    sym_addr += holy_arm_jump24_get_offset (target);
	    offset = sym_addr - (holy_uint32_t) target;

	    if ((sym_addr & 1) || !holy_arm_jump24_check_offset (offset))
	      {
		struct trampoline_arm *tp = mod->trampptr;
		mod->trampptr = tp + 1;
		tp->load_ip = ARM_LOAD_IP;
		tp->bx = (sym_addr & 1) ? ARM_BX : ARM_MOV_PC;
		tp->addr = sym_addr + 8;
		offset = (holy_uint8_t *) tp - (holy_uint8_t *) target - 8;
	      }
	    if (!holy_arm_jump24_check_offset (offset))
	      return holy_error (holy_ERR_BAD_MODULE,
				 "trampoline out of range");
	    holy_arm_jump24_set_offset (target, offset);
	  }
	  break;
	case R_ARM_THM_CALL:
	case R_ARM_THM_JUMP24:
	  {
	    /* Thumb instructions can be 16-bit aligned */
	    holy_int32_t offset;

	    sym_addr += holy_arm_thm_call_get_offset ((holy_uint16_t *) target);

	    holy_dprintf ("dl", "    sym_addr = 0x%08x\n", sym_addr);
	    if (ELF_ST_TYPE (sym->st_info) != STT_FUNC)
	      sym_addr |= 1;

	    offset = sym_addr - (holy_uint32_t) target;

	    holy_dprintf("dl", " BL*: target=%p, sym_addr=0x%08x, offset=%d\n",
			 target, sym_addr, offset);

	    if (!(sym_addr & 1) || (offset < -0x200000 || offset >= 0x200000))
	      {
		struct trampoline_thumb *tp = mod->trampptr;
		mod->trampptr = tp + 1;
		holy_memcpy (tp->template, thumb_template, sizeof (tp->template));
		tp->neg_addr = -sym_addr - 4;
		offset = ((holy_uint8_t *) tp - (holy_uint8_t *) target - 4) | 1;
	      }

	    if (offset < -0x200000 || offset >= 0x200000)
	      return holy_error (holy_ERR_BAD_MODULE,
				 "trampoline out of range");

	    holy_dprintf ("dl", "    relative destination = %p\n",
			  (char *) target + offset);

	    retval = holy_arm_thm_call_set_offset ((holy_uint16_t *) target, offset);
	    if (retval != holy_ERR_NONE)
	      return retval;
	  }
	  break;
	  /* Happens when compiled with -march=armv4.  Since currently we need
	     at least armv5, keep bx as-is.
	   */
	case R_ARM_V4BX:
	  break;
	case R_ARM_THM_MOVW_ABS_NC:
	case R_ARM_THM_MOVT_ABS:
	  {
	    holy_uint32_t offset;
	    offset = holy_arm_thm_movw_movt_get_value((holy_uint16_t *) target);
	    offset += sym_addr;

	    if (ELF_R_TYPE (rel->r_info) == R_ARM_THM_MOVT_ABS)
	      offset >>= 16;
	    else
	      offset &= 0xffff;

	    holy_arm_thm_movw_movt_set_value((holy_uint16_t *) target, offset);
	  }
	  break;
	case R_ARM_THM_JUMP19:
	  {
	    /* Thumb instructions can be 16-bit aligned */
	    holy_int32_t offset;

	    sym_addr += holy_arm_thm_jump19_get_offset ((holy_uint16_t *) target);

	    if (ELF_ST_TYPE (sym->st_info) != STT_FUNC)
	      sym_addr |= 1;

	    offset = sym_addr - (holy_uint32_t) target;

	    if (!holy_arm_thm_jump19_check_offset (offset)
		|| !(sym_addr & 1))
	      {
		struct trampoline_thumb *tp = mod->trampptr;
		mod->trampptr = tp + 1;
		holy_memcpy (tp->template, thumb_template, sizeof (tp->template));
		tp->neg_addr = -sym_addr - 4;
		offset = ((holy_uint8_t *) tp - (holy_uint8_t *) target - 4) | 1;
	      }

	    if (!holy_arm_thm_jump19_check_offset (offset))
	      return holy_error (holy_ERR_BAD_MODULE,
				 "trampoline out of range");

	    holy_arm_thm_jump19_set_offset ((holy_uint16_t *) target, offset);
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


/*
 * Check if EHDR is a valid ELF header.
 */
holy_err_t
holy_arch_dl_check_header (void *ehdr)
{
  Elf_Ehdr *e = ehdr;

  /* Check the magic numbers.  */
  if (e->e_ident[EI_CLASS] != ELFCLASS32
      || e->e_ident[EI_DATA] != ELFDATA2LSB || e->e_machine != EM_ARM)
    return holy_error (holy_ERR_BAD_OS,
		       N_("invalid arch-dependent ELF magic"));

  return holy_ERR_NONE;
}
