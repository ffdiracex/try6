/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/arc/arc.h>
#include <holy/misc.h>
#include <holy/time.h>
#include <holy/term.h>
#include <holy/i18n.h>

void
holy_reboot (void)
{
  holy_ARC_FIRMWARE_VECTOR->restart ();

  holy_millisleep (1500);

  holy_puts_ (N_("Reboot failed"));
  holy_refresh ();
  while (1);
}
