/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <config-util.h>

#include <sys/times.h>
#include <unistd.h>
#include <holy/emu/misc.h>

holy_uint64_t
holy_util_get_cpu_time_ms (void)
{
  struct tms tm;
  static long sc_clk_tck;
  if (!sc_clk_tck)
    {
      sc_clk_tck = sysconf(_SC_CLK_TCK);
      if (sc_clk_tck <= 0)
	sc_clk_tck = 1000;
    }

  times (&tm); 
  return (tm.tms_utime * 1000ULL) / sc_clk_tck;
}
