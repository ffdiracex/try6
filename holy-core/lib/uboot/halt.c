/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/kernel.h>
#include <holy/loader.h>

void
holy_halt (void)
{
  holy_machine_fini (holy_LOADER_FLAG_NORETURN);

  /* Just stop here */

  while (1);
}
