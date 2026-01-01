/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_ARM_RELOC_H
#define holy_ARM_RELOC_H 1

holy_err_t holy_arm_reloc_abs32 (holy_uint32_t *addr, Elf32_Addr sym_addr);

int
holy_arm_thm_call_check_offset (holy_int32_t offset, int is_thumb);
holy_int32_t
holy_arm_thm_call_get_offset (holy_uint16_t *target);
holy_err_t
holy_arm_thm_call_set_offset (holy_uint16_t *target, holy_int32_t offset);

holy_int32_t
holy_arm_thm_jump19_get_offset (holy_uint16_t *target);
void
holy_arm_thm_jump19_set_offset (holy_uint16_t *target, holy_int32_t offset);
int
holy_arm_thm_jump19_check_offset (holy_int32_t offset);

holy_int32_t
holy_arm_jump24_get_offset (holy_uint32_t *target);
int
holy_arm_jump24_check_offset (holy_int32_t offset);
void
holy_arm_jump24_set_offset (holy_uint32_t *target,
			    holy_int32_t offset);

holy_uint16_t
holy_arm_thm_movw_movt_get_value (holy_uint16_t *target);
void
holy_arm_thm_movw_movt_set_value (holy_uint16_t *target, holy_uint16_t value);

#endif
