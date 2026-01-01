/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_IA64_RELOC_H
#define holy_IA64_RELOC_H 1

struct holy_ia64_trampoline;

void
holy_ia64_add_value_to_slot_20b (holy_addr_t addr, holy_uint32_t value);
void
holy_ia64_add_value_to_slot_21 (holy_addr_t addr, holy_uint32_t value);
void
holy_ia64_set_immu64 (holy_addr_t addr, holy_uint64_t value);
void
holy_ia64_make_trampoline (struct holy_ia64_trampoline *tr, holy_uint64_t addr);

struct holy_ia64_trampoline
{
  /* nop.m */
  holy_uint8_t nop[5];
  /* movl r15 = addr*/
  holy_uint8_t addr_hi[6];
  holy_uint8_t e0;
  holy_uint8_t addr_lo[4];
  holy_uint8_t jump[0x20];
};

#endif
