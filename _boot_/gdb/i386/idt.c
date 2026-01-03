/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/machine/memory.h>
#include <holy/misc.h>
#include <holy/cpu/gdb.h>
#include <holy/gdb.h>

static struct holy_cpu_interrupt_gate holy_gdb_idt[holy_GDB_LAST_TRAP + 1]
  __attribute__ ((aligned(16)));

/* Sets up a gate descriptor in the IDT table.  */
static void
holy_idt_gate (struct holy_cpu_interrupt_gate *gate, void (*offset) (void),
	       holy_uint16_t selector, holy_uint8_t type, holy_uint8_t dpl)
{
  gate->offset_lo = (int) offset & 0xffff;
  gate->selector = selector & 0xffff;
  gate->unused = 0;
  gate->gate = (type & 0x1f) | ((dpl & 0x3) << 5) | 0x80;
  gate->offset_hi = ((int) offset >> 16) & 0xffff;
}

static struct holy_cpu_idt_descriptor holy_gdb_orig_idt_desc
  __attribute__ ((aligned(16)));
static struct holy_cpu_idt_descriptor holy_gdb_idt_desc
  __attribute__ ((aligned(16)));

/* Set up interrupt and trap handler descriptors in IDT.  */
void
holy_gdb_idtinit (void)
{
  int i;
  holy_uint16_t seg;

  asm volatile ("xorl %%eax, %%eax\n"
		"mov %%cs, %%ax\n" :"=a" (seg));

  for (i = 0; i <= holy_GDB_LAST_TRAP; i++)
    {
      holy_idt_gate (&holy_gdb_idt[i],
		     holy_gdb_trapvec[i], seg,
                     holy_CPU_TRAP_GATE, 0);
    }

  holy_gdb_idt_desc.base = (holy_addr_t) holy_gdb_idt;
  holy_gdb_idt_desc.limit = sizeof (holy_gdb_idt) - 1;
  asm volatile ("sidt %0" : : "m" (holy_gdb_orig_idt_desc));
  asm volatile ("lidt %0" : : "m" (holy_gdb_idt_desc));
}

void
holy_gdb_idtrestore (void)
{
  asm volatile ("lidt %0" : : "m" (holy_gdb_orig_idt_desc));
}

void
holy_gdb_breakpoint (void)
{
  asm volatile ("int $3");
}
