/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/misc.h>

#include <holy/types.h>
#include <holy/err.h>
#include <holy/term.h>

#include <holy/i386/relocator.h>
#include <holy/relocator_private.h>
#include <holy/i386/relocator_private.h>
#include <holy/i386/pc/int.h>

extern holy_uint8_t holy_relocator16_start;
extern holy_uint8_t holy_relocator16_end;
extern holy_uint16_t holy_relocator16_cs;
extern holy_uint16_t holy_relocator16_ip;
extern holy_uint16_t holy_relocator16_ds;
extern holy_uint16_t holy_relocator16_es;
extern holy_uint16_t holy_relocator16_fs;
extern holy_uint16_t holy_relocator16_gs;
extern holy_uint16_t holy_relocator16_ss;
extern holy_uint16_t holy_relocator16_sp;
extern holy_uint32_t holy_relocator16_edx;
extern holy_uint32_t holy_relocator16_ebx;
extern holy_uint32_t holy_relocator16_esi;
extern holy_uint32_t holy_relocator16_ebp;

extern holy_uint16_t holy_relocator16_keep_a20_enabled;

extern holy_uint8_t holy_relocator32_start;
extern holy_uint8_t holy_relocator32_end;
extern holy_uint32_t holy_relocator32_eax;
extern holy_uint32_t holy_relocator32_ebx;
extern holy_uint32_t holy_relocator32_ecx;
extern holy_uint32_t holy_relocator32_edx;
extern holy_uint32_t holy_relocator32_eip;
extern holy_uint32_t holy_relocator32_esp;
extern holy_uint32_t holy_relocator32_ebp;
extern holy_uint32_t holy_relocator32_esi;
extern holy_uint32_t holy_relocator32_edi;

extern holy_uint8_t holy_relocator64_start;
extern holy_uint8_t holy_relocator64_end;
extern holy_uint64_t holy_relocator64_rax;
extern holy_uint64_t holy_relocator64_rbx;
extern holy_uint64_t holy_relocator64_rcx;
extern holy_uint64_t holy_relocator64_rdx;
extern holy_uint64_t holy_relocator64_rip;
extern holy_uint64_t holy_relocator64_rsp;
extern holy_uint64_t holy_relocator64_rsi;
extern holy_addr_t holy_relocator64_cr3;
extern struct holy_i386_idt holy_relocator16_idt;

#define RELOCATOR_SIZEOF(x)	(&holy_relocator##x##_end - &holy_relocator##x##_start)

holy_err_t
holy_relocator32_boot (struct holy_relocator *rel,
		       struct holy_relocator32_state state,
		       int avoid_efi_bootservices)
{
  holy_err_t err;
  void *relst;
  holy_relocator_chunk_t ch;

  /* Specific memory range due to Global Descriptor Table for use by payload
     that we will store in returned chunk.  The address range and preference
     are based on "THE LINUX/x86 BOOT PROTOCOL" specification.  */
  err = holy_relocator_alloc_chunk_align (rel, &ch, 0x1000,
					  0x9a000 - RELOCATOR_SIZEOF (32),
					  RELOCATOR_SIZEOF (32), 16,
					  holy_RELOCATOR_PREFERENCE_LOW,
					  avoid_efi_bootservices);
  if (err)
    return err;

  holy_relocator32_eax = state.eax;
  holy_relocator32_ebx = state.ebx;
  holy_relocator32_ecx = state.ecx;
  holy_relocator32_edx = state.edx;
  holy_relocator32_eip = state.eip;
  holy_relocator32_esp = state.esp;
  holy_relocator32_ebp = state.ebp;
  holy_relocator32_esi = state.esi;
  holy_relocator32_edi = state.edi;

  holy_memmove (get_virtual_current_address (ch), &holy_relocator32_start,
		RELOCATOR_SIZEOF (32));

  err = holy_relocator_prepare_relocs (rel, get_physical_target_address (ch),
				       &relst, NULL);
  if (err)
    return err;

  asm volatile ("cli");
  ((void (*) (void)) relst) ();

  /* Not reached.  */
  return holy_ERR_NONE;
}

holy_err_t
holy_relocator16_boot (struct holy_relocator *rel,
		       struct holy_relocator16_state state)
{
  holy_err_t err;
  void *relst;
  holy_relocator_chunk_t ch;

  /* Put it higher than the byte it checks for A20 check.  */
  err = holy_relocator_alloc_chunk_align (rel, &ch, 0x8010,
					  0xa0000 - RELOCATOR_SIZEOF (16)
					  - holy_RELOCATOR16_STACK_SIZE,
					  RELOCATOR_SIZEOF (16)
					  + holy_RELOCATOR16_STACK_SIZE, 16,
					  holy_RELOCATOR_PREFERENCE_NONE,
					  0);
  if (err)
    return err;

  holy_relocator16_cs = state.cs;
  holy_relocator16_ip = state.ip;

  holy_relocator16_ds = state.ds;
  holy_relocator16_es = state.es;
  holy_relocator16_fs = state.fs;
  holy_relocator16_gs = state.gs;

  holy_relocator16_ss = state.ss;
  holy_relocator16_sp = state.sp;

  holy_relocator16_ebp = state.ebp;
  holy_relocator16_ebx = state.ebx;
  holy_relocator16_edx = state.edx;
  holy_relocator16_esi = state.esi;
#ifdef holy_MACHINE_PCBIOS
  holy_relocator16_idt = *holy_realidt;
#else
  holy_relocator16_idt.base = 0;
  holy_relocator16_idt.limit = 0;
#endif

  holy_relocator16_keep_a20_enabled = state.a20;

  holy_memmove (get_virtual_current_address (ch), &holy_relocator16_start,
		RELOCATOR_SIZEOF (16));

  err = holy_relocator_prepare_relocs (rel, get_physical_target_address (ch),
				       &relst, NULL);
  if (err)
    return err;

  asm volatile ("cli");
  ((void (*) (void)) relst) ();

  /* Not reached.  */
  return holy_ERR_NONE;
}

holy_err_t
holy_relocator64_boot (struct holy_relocator *rel,
		       struct holy_relocator64_state state,
		       holy_addr_t min_addr, holy_addr_t max_addr)
{
  holy_err_t err;
  void *relst;
  holy_relocator_chunk_t ch;

  err = holy_relocator_alloc_chunk_align (rel, &ch, min_addr,
					  max_addr - RELOCATOR_SIZEOF (64),
					  RELOCATOR_SIZEOF (64), 16,
					  holy_RELOCATOR_PREFERENCE_NONE,
					  0);
  if (err)
    return err;

  holy_relocator64_rax = state.rax;
  holy_relocator64_rbx = state.rbx;
  holy_relocator64_rcx = state.rcx;
  holy_relocator64_rdx = state.rdx;
  holy_relocator64_rip = state.rip;
  holy_relocator64_rsp = state.rsp;
  holy_relocator64_rsi = state.rsi;
  holy_relocator64_cr3 = state.cr3;

  holy_memmove (get_virtual_current_address (ch), &holy_relocator64_start,
		RELOCATOR_SIZEOF (64));

  err = holy_relocator_prepare_relocs (rel, get_physical_target_address (ch),
				       &relst, NULL);
  if (err)
    return err;

  asm volatile ("cli");
  ((void (*) (void)) relst) ();

  /* Not reached.  */
  return holy_ERR_NONE;
}
