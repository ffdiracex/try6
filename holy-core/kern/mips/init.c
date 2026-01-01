/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/kernel.h>
#include <holy/env.h>
#include <holy/cpu/time.h>
#include <holy/cpu/mips.h>

/* FIXME: use interrupt to count high.  */
holy_uint64_t
holy_get_rtc (void)
{
  static holy_uint32_t high = 0;
  static holy_uint32_t last = 0;
  holy_uint32_t low;

  asm volatile ("mfc0 %0, " holy_CPU_MIPS_COP0_TIMER_COUNT : "=r" (low));
  if (low < last)
    high++;
  last = low;

  return (((holy_uint64_t) high) << 32) | low;
}
