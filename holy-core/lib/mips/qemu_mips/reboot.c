/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/cpu/io.h>

void
holy_reboot (void)
{
  holy_outl (42, 0xbfbf0000);
  while (1);
}
