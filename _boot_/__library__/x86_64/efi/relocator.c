/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/misc.h>

#include <holy/types.h>
#include <holy/err.h>

#include <holy/i386/relocator.h>
#include <holy/relocator_private.h>

extern holy_uint64_t holy_relocator64_rax;
extern holy_uint64_t holy_relocator64_rbx;
extern holy_uint64_t holy_relocator64_rcx;
extern holy_uint64_t holy_relocator64_rdx;
extern holy_uint64_t holy_relocator64_rip;
extern holy_uint64_t holy_relocator64_rsi;

extern holy_uint8_t holy_relocator64_efi_start;
extern holy_uint8_t holy_relocator64_efi_end;

#define RELOCATOR_SIZEOF(x)	(&holy_relocator##x##_end - &holy_relocator##x##_start)

holy_err_t
holy_relocator64_efi_boot (struct holy_relocator *rel,
			   struct holy_relocator64_efi_state state)
{
  holy_err_t err;
  void *relst;
  holy_relocator_chunk_t ch;

  /*
   * 64-bit relocator code may live above 4 GiB quite well.
   * However, I do not want ask for problems. Just in case.
   */
  err = holy_relocator_alloc_chunk_align (rel, &ch, 0,
					  0x100000000 - RELOCATOR_SIZEOF (64_efi),
					  RELOCATOR_SIZEOF (64_efi), 16,
					  holy_RELOCATOR_PREFERENCE_NONE, 1);
  if (err)
    return err;

  /* Do not touch %rsp! It points to EFI created stack. */
  holy_relocator64_rax = state.rax;
  holy_relocator64_rbx = state.rbx;
  holy_relocator64_rcx = state.rcx;
  holy_relocator64_rdx = state.rdx;
  holy_relocator64_rip = state.rip;
  holy_relocator64_rsi = state.rsi;

  holy_memmove (get_virtual_current_address (ch), &holy_relocator64_efi_start,
		RELOCATOR_SIZEOF (64_efi));

  err = holy_relocator_prepare_relocs (rel, get_physical_target_address (ch),
				       &relst, NULL);
  if (err)
    return err;

  ((void (*) (void)) relst) ();

  /* Not reached.  */
  return holy_ERR_NONE;
}
