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
#include <holy/arm64/reloc.h>

/*
 * holy_arm64_reloc_xxxx26():
 *
 * JUMP26/CALL26 relocations for B and BL instructions.
 */

int
holy_arm_64_check_xxxx26_offset (holy_int64_t offset)
{
  const holy_ssize_t offset_low = -(1 << 27), offset_high = (1 << 27) - 1;

  if ((offset < offset_low) || (offset > offset_high))
    return 0;
  return 1;
}

void
holy_arm64_set_xxxx26_offset (holy_uint32_t *place, holy_int64_t offset)
{
  const holy_uint32_t insmask = holy_cpu_to_le32_compile_time (0xfc000000);

  holy_dprintf ("dl", "  reloc_xxxx64 %p %c= 0x%llx\n",
		place, offset > 0 ? '+' : '-',
		offset < 0 ? (long long) -(unsigned long long) offset : offset);

  *place &= insmask;
  *place |= holy_cpu_to_le32 (offset >> 2) & ~insmask;
}

int
holy_arm64_check_hi21_signed (holy_int64_t offset)
{
  if (offset != (holy_int64_t)(holy_int32_t)offset)
    return 0;
  return 1;
}

void
holy_arm64_set_hi21 (holy_uint32_t *place, holy_int64_t offset)
{
  const holy_uint32_t insmask = holy_cpu_to_le32_compile_time (0x9f00001f);
  holy_uint32_t val;

  offset >>= 12;
  
  val = ((offset & 3) << 29) | (((offset >> 2) & 0x7ffff) << 5);
  
  *place &= insmask;
  *place |= holy_cpu_to_le32 (val) & ~insmask;
}

void
holy_arm64_set_abs_lo12 (holy_uint32_t *place, holy_int64_t target)
{
  const holy_uint32_t insmask = holy_cpu_to_le32_compile_time (0xffc003ff);

  *place &= insmask;
  *place |= holy_cpu_to_le32 (target << 10) & ~insmask;
}

void
holy_arm64_set_abs_lo12_ldst64 (holy_uint32_t *place, holy_int64_t target)
{
  const holy_uint32_t insmask = holy_cpu_to_le32_compile_time (0xfff803ff);

  *place &= insmask;
  *place |= holy_cpu_to_le32 (target << 7) & ~insmask;
}

#pragma GCC diagnostic ignored "-Wcast-align"

holy_err_t
holy_arm64_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
				  holy_size_t *got)
{
  const Elf64_Ehdr *e = ehdr;
  const Elf64_Shdr *s;
  unsigned i;

  *tramp = 0;
  *got = 0;

  for (i = 0, s = (Elf64_Shdr *) ((char *) e + holy_le_to_cpu64 (e->e_shoff));
       i < holy_le_to_cpu16 (e->e_shnum);
       i++, s = (Elf64_Shdr *) ((char *) s + holy_le_to_cpu16 (e->e_shentsize)))
    if (s->sh_type == holy_cpu_to_le32_compile_time (SHT_REL)
	|| s->sh_type == holy_cpu_to_le32_compile_time (SHT_RELA))
      {
	const Elf64_Rela *rel, *max;

	for (rel = (Elf64_Rela *) ((char *) e + holy_le_to_cpu64 (s->sh_offset)),
	       max = (const Elf64_Rela *) ((char *) rel + holy_le_to_cpu64 (s->sh_size));
	     rel < max; rel = (const Elf64_Rela *) ((char *) rel + holy_le_to_cpu64 (s->sh_entsize)))
	  switch (ELF64_R_TYPE (rel->r_info))
	    {
	    case R_AARCH64_CALL26:
	    case R_AARCH64_JUMP26:
	      *tramp += sizeof (struct holy_arm64_trampoline);
	      break;
	    case R_AARCH64_ADR_GOT_PAGE:
	      *got += 8;
	      break;
	    }
      }

  return holy_ERR_NONE;
}
