/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_uint64_t
abs64(holy_int64_t a)
{
  return a > 0 ? a : -a;
}

holy_int64_t
holy_divmod64s (holy_int64_t n,
		holy_int64_t d,
		holy_int64_t *ro)
{
  holy_uint64_t ru;
  holy_int64_t q, r;
  q = holy_divmod64 (abs64(n), abs64(d), &ru);
  r = ru;
  /* Now: |n| = |d| * q + r  */
  if (n < 0)
    {
      /* -|n| = |d| * (-q) + (-r)  */
      q = -q;
      r = -r;
    }
  /* Now: n = |d| * q + r  */
  if (d < 0)
    {
      /* n = (-|d|) * (-q) + r  */
      q = -q;
    }
  /* Now: n = d * q + r  */
  if (ro)
    *ro = r;
  return q;
}

holy_uint32_t
holy_divmod32 (holy_uint32_t n, holy_uint32_t d, holy_uint32_t *ro)
{
  holy_uint64_t q, r;
  q = holy_divmod64 (n, d, &r);
  *ro = r;
  return q;
}

holy_int32_t
holy_divmod32s (holy_int32_t n, holy_int32_t d, holy_int32_t *ro)
{
  holy_int64_t q, r;
  q = holy_divmod64s (n, d, &r);
  *ro = r;
  return q;
}
