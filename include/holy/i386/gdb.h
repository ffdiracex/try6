/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_GDB_CPU_HEADER
#define holy_GDB_CPU_HEADER	1

#define holy_GDB_LAST_TRAP	31
/* You may have to edit the bottom of machdep.S when adjusting
   holy_GDB_LAST_TRAP.	*/
#define holy_MACHINE_NR_REGS	16

#define	EAX	0
#define	ECX	1
#define	EDX	2
#define	EBX	3
#define	ESP	4
#define	EBP	5
#define	ESI	6
#define	EDI	7
#define	EIP	8
#define	EFLAGS	9
#define	CS	10
#define	SS	11
#define	DS	12
#define	ES	13
#define	FS	14
#define	GS	15

#define PC	EIP
#define FP	EBP
#define SP	ESP
#define PS	EFLAGS

#ifndef ASM_FILE

#include <holy/gdb.h>

#define holy_CPU_TRAP_GATE	15

struct holy_cpu_interrupt_gate
{
  holy_uint16_t offset_lo;
  holy_uint16_t selector;
  holy_uint8_t unused;
  holy_uint8_t gate;
  holy_uint16_t offset_hi;
} holy_PACKED;

struct holy_cpu_idt_descriptor
{
  holy_uint16_t limit;
  holy_uint32_t base;
} holy_PACKED;

extern void (*holy_gdb_trapvec[]) (void);
void holy_gdb_idtinit (void);
void holy_gdb_idtrestore (void);
void holy_gdb_trap (int trap_no) __attribute__ ((regparm(3)));

#endif /* ! ASM */
#endif /* ! holy_GDB_CPU_HEADER */

