/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <config-util.h>

#include <holy/emu/misc.h>
#include <windows.h>

holy_uint64_t
holy_util_get_cpu_time_ms (void)
{
  FILETIME cr, ex, ke, us;
  ULARGE_INTEGER us_ul;

  GetProcessTimes (GetCurrentProcess (), &cr, &ex, &ke, &us);
  us_ul.LowPart = us.dwLowDateTime;
  us_ul.HighPart = us.dwHighDateTime;

  return us_ul.QuadPart / 10000;
}

