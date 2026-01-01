/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_INTERRUPT_MACHINE_HEADER
#define holy_INTERRUPT_MACHINE_HEADER	1

#include <holy/symbol.h>
#include <holy/types.h>

struct holy_bios_int_registers
{
  holy_uint32_t eax;
  holy_uint16_t es;
  holy_uint16_t ds;
  holy_uint16_t flags;
  holy_uint16_t dummy;
  holy_uint32_t ebx;
  holy_uint32_t ecx;
  holy_uint32_t edi;
  holy_uint32_t esi;
  holy_uint32_t edx;
};

#define  holy_CPU_INT_FLAGS_CARRY     0x1
#define  holy_CPU_INT_FLAGS_PARITY    0x4
#define  holy_CPU_INT_FLAGS_ADJUST    0x10
#define  holy_CPU_INT_FLAGS_ZERO      0x40
#define  holy_CPU_INT_FLAGS_SIGN      0x80
#define  holy_CPU_INT_FLAGS_TRAP      0x100
#define  holy_CPU_INT_FLAGS_INTERRUPT 0x200
#define  holy_CPU_INT_FLAGS_DIRECTION 0x400
#define  holy_CPU_INT_FLAGS_OVERFLOW  0x800
#ifdef holy_MACHINE_PCBIOS
#define  holy_CPU_INT_FLAGS_DEFAULT   holy_CPU_INT_FLAGS_INTERRUPT
#else
#define  holy_CPU_INT_FLAGS_DEFAULT   0
#endif

void EXPORT_FUNC (holy_bios_interrupt) (holy_uint8_t intno,
					struct holy_bios_int_registers *regs)
     __attribute__ ((regparm(3)));
struct holy_i386_idt
{
  holy_uint16_t limit;
  holy_uint32_t base;
} holy_PACKED;

#ifdef holy_MACHINE_PCBIOS
extern struct holy_i386_idt *EXPORT_VAR(holy_realidt);
#endif

#endif
