/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/ieee1275/ieee1275.h>
#include <holy/misc.h>

void
holy_halt (void)
{
  /* Not standardized.  We try three known commands.  */

  holy_ieee1275_interpret ("power-off", 0);
  holy_ieee1275_interpret ("shut-down", 0);
  holy_ieee1275_interpret ("poweroff", 0);

  while (1);
}
