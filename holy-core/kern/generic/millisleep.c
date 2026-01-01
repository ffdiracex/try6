/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/time.h>

void
holy_millisleep (holy_uint32_t ms)
{
  holy_uint64_t start;

  start = holy_get_time_ms ();

  /* Instead of setting an end time and looping while the current time is
     less than that, comparing the elapsed sleep time with the desired sleep
     time handles the (unlikely!) case that the timer would wrap around
     during the sleep. */

  while (holy_get_time_ms () - start < ms)
    holy_cpu_idle ();
}
