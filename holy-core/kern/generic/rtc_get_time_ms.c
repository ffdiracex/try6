/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/time.h>
#include <holy/misc.h>
#include <holy/machine/time.h>

/* Calculate the time in milliseconds since the epoch based on the RTC. */
holy_uint64_t
holy_rtc_get_time_ms (void)
{
  /* By dimensional analysis:

      1000 ms   N rtc ticks       1 s
      ------- * ----------- * ----------- = 1000*N/T ms
        1 s          1        T rtc ticks
   */
  holy_uint64_t ticks_ms_per_sec = ((holy_uint64_t) 1000) * holy_get_rtc ();
  return holy_divmod64 (ticks_ms_per_sec, holy_TICKS_PER_SECOND ? : 1000, 0);
}
