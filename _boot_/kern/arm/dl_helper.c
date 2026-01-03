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

static inline holy_uint32_t
thumb_get_instruction_word (holy_uint16_t *target)
{
  /* Extract instruction word in alignment-safe manner */
  return holy_le_to_cpu16 ((*target)) << 16 | holy_le_to_cpu16 (*(target + 1));
}

static inline void
thumb_set_instruction_word (holy_uint16_t *target, holy_uint32_t insword)
{
  *target = holy_cpu_to_le16 (insword >> 16);
  *(target + 1) = holy_cpu_to_le16 (insword & 0xffff);
}

/*
 * R_ARM_ABS32
 *
 * Simple relocation of 32-bit value (in literal pool)
 */
holy_err_t
holy_arm_reloc_abs32 (Elf32_Word *target, Elf32_Addr sym_addr)
{
  Elf32_Addr tmp;

  tmp = holy_le_to_cpu32 (*target);
  tmp += sym_addr;
  *target = holy_cpu_to_le32 (tmp);

  return holy_ERR_NONE;
}

/********************************************************************
 * Thumb (T32) relocations:                                         *
 *                                                                  *
 * 32-bit Thumb instructions can be 16-bit aligned, and are fetched *
 * little-endian, requiring some additional fiddling.               *
 ********************************************************************/

holy_int32_t
holy_arm_thm_call_get_offset (holy_uint16_t *target)
{
  holy_uint32_t sign, j1, j2;
  holy_uint32_t insword;
  holy_int32_t offset;

  insword = thumb_get_instruction_word (target);

  /* Extract bitfields from instruction words */
  sign = (insword >> 26) & 1;
  j1 = (insword >> 13) & 1;
  j2 = (insword >> 11) & 1;
  offset = (sign << 24) | ((~(j1 ^ sign) & 1) << 23) |
    ((~(j2 ^ sign) & 1) << 22) |
    ((insword & 0x03ff0000) >> 4) | ((insword & 0x000007ff) << 1);

  /* Sign adjust and calculate offset */
  if (offset & (1 << 24))
    offset -= (1 << 25);

  return offset;
}

holy_err_t
holy_arm_thm_call_set_offset (holy_uint16_t *target, holy_int32_t offset)
{
  holy_uint32_t sign, j1, j2;
  const holy_uint32_t insmask = 0xf800d000;
  holy_uint32_t insword;
  int is_blx;

  insword = thumb_get_instruction_word (target);

  if (((insword >> 12) & 0xd) == 0xc)
    is_blx = 1;
  else
    is_blx = 0;

  if (!is_blx && !(offset & 1))
    return holy_error (holy_ERR_BAD_MODULE, "bl/b.w targettting ARM");

  /* Transform blx into bl if necessarry.  */
  if (is_blx && (offset & 1))
    insword |= (1 << 12);

  /* Reassemble instruction word */
  sign = (offset >> 24) & 1;
  j1 = sign ^ (~(offset >> 23) & 1);
  j2 = sign ^ (~(offset >> 22) & 1);
  insword = (insword & insmask) |
    (sign << 26) |
    (((offset >> 12) & 0x03ff) << 16) |
    (j1 << 13) | (j2 << 11) | ((offset >> 1) & 0x07ff);

  thumb_set_instruction_word (target, insword);

  holy_dprintf ("dl", "    *insword = 0x%08x", insword);

  return holy_ERR_NONE;
}

holy_int32_t
holy_arm_thm_jump19_get_offset (holy_uint16_t *target)
{
  holy_int32_t offset;
  holy_uint32_t insword;

  insword = thumb_get_instruction_word (target);

  /* Extract and sign extend offset */
  offset = ((insword >> 26) & 1) << 19
    | ((insword >> 11) & 1) << 18
    | ((insword >> 13) & 1) << 17
    | ((insword >> 16) & 0x3f) << 11
    | (insword & 0x7ff);
  offset <<= 1;
  if (offset & (1 << 20))
    offset -= (1 << 21);

  return offset;
}

void
holy_arm_thm_jump19_set_offset (holy_uint16_t *target, holy_int32_t offset)
{
  holy_uint32_t insword;
  const holy_uint32_t insmask = 0xfbc0d000;

  offset >>= 1;
  offset &= 0xfffff;

  insword = thumb_get_instruction_word (target);

  /* Reassemble instruction word and write back */
  insword &= insmask;
  insword |= ((offset >> 19) & 1) << 26
    | ((offset >> 18) & 1) << 11
    | ((offset >> 17) & 1) << 13
    | ((offset >> 11) & 0x3f) << 16
    | (offset & 0x7ff);
  thumb_set_instruction_word (target, insword);
}

int
holy_arm_thm_jump19_check_offset (holy_int32_t offset)
{
  if ((offset > 1048574) || (offset < -1048576))
    return 0;
  return 1;
}

holy_uint16_t
holy_arm_thm_movw_movt_get_value (holy_uint16_t *target)
{
  holy_uint32_t insword;

  insword = thumb_get_instruction_word (target);

  return ((insword & 0xf0000) >> 4) | ((insword & 0x04000000) >> 15) | \
    ((insword & 0x7000) >> 4) | (insword & 0xff);
}

void
holy_arm_thm_movw_movt_set_value (holy_uint16_t *target, holy_uint16_t value)
{
  holy_uint32_t insword;
  const holy_uint32_t insmask = 0xfbf08f00;

  insword = thumb_get_instruction_word (target);
  insword &= insmask;

  insword |= ((value & 0xf000) << 4) | ((value & 0x0800) << 15) | \
    ((value & 0x0700) << 4) | (value & 0xff);

  thumb_set_instruction_word (target, insword);
}


/***********************************************************
 * ARM (A32) relocations:                                  *
 *                                                         *
 * ARM instructions are 32-bit in size and 32-bit aligned. *
 ***********************************************************/

holy_int32_t
holy_arm_jump24_get_offset (holy_uint32_t *target)
{
  holy_int32_t offset;
  holy_uint32_t insword;

  insword = holy_le_to_cpu32 (*target);

  offset = (insword & 0x00ffffff) << 2;
  if (offset & 0x02000000)
    offset -= 0x04000000;
  return offset;
}

int
holy_arm_jump24_check_offset (holy_int32_t offset)
{
  if (offset >= 0x02000000 || offset < -0x02000000)
    return 0;
  return 1;
}

void
holy_arm_jump24_set_offset (holy_uint32_t *target,
			    holy_int32_t offset)
{
  holy_uint32_t insword;

  insword = holy_le_to_cpu32 (*target);

  insword &= 0xff000000;
  insword |= (offset >> 2) & 0x00ffffff;

  *target = holy_cpu_to_le32 (insword);
}
