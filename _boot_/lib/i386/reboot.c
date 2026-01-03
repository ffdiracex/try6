/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/relocator.h>
#include <holy/cpu/relocator.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/cpu/reboot.h>
#include <holy/i386/floppy.h>

void
holy_reboot (void)
{
  struct holy_relocator *relocator = NULL;
  holy_relocator_chunk_t ch;
  holy_err_t err;
  void *buf;
  struct holy_relocator16_state state;
  holy_uint16_t segment;

  relocator = holy_relocator_new ();
  if (!relocator)
    while (1);
  err = holy_relocator_alloc_chunk_align (relocator, &ch, 0x1000, 0x1000,
					  holy_reboot_end - holy_reboot_start,
					  16, holy_RELOCATOR_PREFERENCE_NONE,
					  0);
  if (err)
    while (1);
  buf = get_virtual_current_address (ch);
  holy_memcpy (buf, holy_reboot_start, holy_reboot_end - holy_reboot_start);

  segment = ((holy_addr_t) get_physical_target_address (ch)) >> 4;
  state.gs = state.fs = state.es = state.ds = state.ss = segment;
  state.sp = 0;
  state.cs = segment;
  state.ip = 0;
  state.a20 = 0;

  holy_stop_floppy ();
  
  err = holy_relocator16_boot (relocator, state);

  while (1);
}
