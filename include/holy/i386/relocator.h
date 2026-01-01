/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_RELOCATOR_CPU_HEADER
#define holy_RELOCATOR_CPU_HEADER	1

#include <holy/types.h>
#include <holy/err.h>
#include <holy/relocator.h>

struct holy_relocator32_state
{
  holy_uint32_t esp;
  holy_uint32_t ebp;
  holy_uint32_t eax;
  holy_uint32_t ebx;
  holy_uint32_t ecx;
  holy_uint32_t edx;
  holy_uint32_t eip;
  holy_uint32_t esi;
  holy_uint32_t edi;
};

struct holy_relocator16_state
{
  holy_uint16_t cs;
  holy_uint16_t ds;
  holy_uint16_t es;
  holy_uint16_t fs;
  holy_uint16_t gs;
  holy_uint16_t ss;
  holy_uint16_t sp;
  holy_uint16_t ip;
  holy_uint32_t ebx;
  holy_uint32_t edx;
  holy_uint32_t esi;
  holy_uint32_t ebp;
  int a20;
};

struct holy_relocator64_state
{
  holy_uint64_t rsp;
  holy_uint64_t rax;
  holy_uint64_t rbx;
  holy_uint64_t rcx;
  holy_uint64_t rdx;
  holy_uint64_t rip;
  holy_uint64_t rsi;
  holy_addr_t cr3;
};

#ifdef holy_MACHINE_EFI
#ifdef __x86_64__
struct holy_relocator64_efi_state
{
  holy_uint64_t rax;
  holy_uint64_t rbx;
  holy_uint64_t rcx;
  holy_uint64_t rdx;
  holy_uint64_t rip;
  holy_uint64_t rsi;
};
#endif
#endif

holy_err_t holy_relocator16_boot (struct holy_relocator *rel,
				  struct holy_relocator16_state state);

holy_err_t holy_relocator32_boot (struct holy_relocator *rel,
				  struct holy_relocator32_state state,
				  int avoid_efi_bootservices);

holy_err_t holy_relocator64_boot (struct holy_relocator *rel,
				  struct holy_relocator64_state state,
				  holy_addr_t min_addr, holy_addr_t max_addr);

#ifdef holy_MACHINE_EFI
#ifdef __x86_64__
holy_err_t holy_relocator64_efi_boot (struct holy_relocator *rel,
				      struct holy_relocator64_efi_state state);
#endif
#endif

#endif /* ! holy_RELOCATOR_CPU_HEADER */
