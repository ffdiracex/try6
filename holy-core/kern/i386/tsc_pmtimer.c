/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/time.h>
#include <holy/misc.h>
#include <holy/i386/tsc.h>
#include <holy/i386/pmtimer.h>
#include <holy/acpi.h>
#include <holy/cpu/io.h>

holy_uint64_t
holy_pmtimer_wait_count_tsc (holy_port_t pmtimer,
			     holy_uint16_t num_pm_ticks)
{
  holy_uint32_t start;
  holy_uint32_t last;
  holy_uint32_t cur, end;
  holy_uint64_t start_tsc;
  holy_uint64_t end_tsc;
  int num_iter = 0;

  start = holy_inl (pmtimer) & 0xffffff;
  last = start;
  end = start + num_pm_ticks;
  start_tsc = holy_get_tsc ();
  while (1)
    {
      cur = holy_inl (pmtimer) & 0xffffff;
      if (cur < last)
	cur |= 0x1000000;
      num_iter++;
      if (cur >= end)
	{
	  end_tsc = holy_get_tsc ();
	  return end_tsc - start_tsc;
	}
      /* Check for broken PM timer.
	 50000000 TSCs is between 5 ms (10GHz) and 200 ms (250 MHz)
	 if after this time we still don't have 1 ms on pmtimer, then
	 pmtimer is broken.
       */
      if ((num_iter & 0xffffff) == 0 && holy_get_tsc () - start_tsc > 5000000) {
	return 0;
      }
    }
}

int
holy_tsc_calibrate_from_pmtimer (void)
{
  struct holy_acpi_fadt *fadt;
  holy_port_t pmtimer;
  holy_uint64_t tsc_diff;

  fadt = holy_acpi_find_fadt ();
  if (!fadt)
    return 0;
  pmtimer = fadt->pmtimer;
  if (!pmtimer)
    return 0;

  /* It's 3.579545 MHz clock. Wait 1 ms.  */
  tsc_diff = holy_pmtimer_wait_count_tsc (pmtimer, 3580);
  if (tsc_diff == 0)
    return 0;
  holy_tsc_rate = holy_divmod64 ((1ULL << 32), tsc_diff, 0);
  return 1;
}
