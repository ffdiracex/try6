/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/ieee1275/ieee1275.h>
#include <holy/misc.h>

void
holy_reboot (void)
{
  holy_ieee1275_interpret ("reset-all", 0);
  for (;;) ;
}
