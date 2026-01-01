/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/kernel.h>
#include <holy/xen.h>

void
holy_halt (void)
{
  struct sched_shutdown arg;

  arg.reason = SHUTDOWN_poweroff;
  holy_xen_sched_op (SCHEDOP_shutdown, &arg);
  for (;;);
}
