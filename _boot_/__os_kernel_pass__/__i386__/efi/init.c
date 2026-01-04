/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/cache.h>
#include <holy/kernel.h>
#include <holy/efi/efi.h>
#include <holy/i386/tsc.h>
#include <holy/loader.h>

void
holy_machine_init (void)
{
  holy_efi_init ();
  holy_tsc_init ();
}

void
holy_machine_fini (int flags)
{
  if (flags & holy_LOADER_FLAG_NORETURN)
    holy_efi_fini ();
}
