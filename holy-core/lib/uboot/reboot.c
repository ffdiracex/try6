/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/kernel.h>
#include <holy/misc.h>
#include <holy/uboot/uboot.h>
#include <holy/loader.h>

void
holy_reboot (void)
{
  holy_machine_fini (holy_LOADER_FLAG_NORETURN);

  holy_uboot_reset ();
  while (1);
}
