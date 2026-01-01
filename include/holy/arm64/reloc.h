/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_ARM64_RELOC_H
#define holy_ARM64_RELOC_H 1

struct holy_arm64_trampoline
{
  holy_uint32_t ldr; /* ldr	x16, 8 */
  holy_uint32_t br; /* br x16 */
  holy_uint64_t addr;
};

int holy_arm_64_check_xxxx26_offset (holy_int64_t offset);
void
holy_arm64_set_xxxx26_offset (holy_uint32_t *place, holy_int64_t offset);
int
holy_arm64_check_hi21_signed (holy_int64_t offset);
void
holy_arm64_set_hi21 (holy_uint32_t *place, holy_int64_t offset);
void
holy_arm64_set_abs_lo12 (holy_uint32_t *place, holy_int64_t target);
void
holy_arm64_set_abs_lo12_ldst64 (holy_uint32_t *place, holy_int64_t target);

#endif
