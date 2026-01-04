/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/test.h>
#include <holy/dl.h>
#include <holy/misc.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_uint64_t vectors[][2] = {
  { 0xffffffffffffffffULL, 1},
  { 1, 0xffffffffffffffffULL},
  { 0xffffffffffffffffULL, 0xffffffffffffffffULL},
  { 1, 1 },
  { 2, 1 }
};

static void
test64(holy_uint64_t a, holy_uint64_t b)
{
  holy_uint64_t r1 = a * b, r2 = 0, r3;
  int i;
  for (i = 0; i < 64; i++)
    if ((a & (1LL << i)))
      r2 += b << i;
  r3 = ((holy_int64_t) a) * ((holy_int64_t) b);
  holy_test_assert (r1 == r2,
		    "multiplication mismatch (u): 0x%llx x 0x%llx = 0x%llx != 0x%llx",
		    (long long) a, (long long) b, (long long) r2, (long long) r1);
  holy_test_assert (r3 == r2,
		    "multiplication mismatch (s): 0x%llx x 0x%llx = 0x%llx != 0x%llx",
		    (long long) a, (long long) b, (long long) r2, (long long) r3);
}

static void
mul_test (void)
{
  holy_uint64_t a = 404, b = 7;
  holy_size_t i;

  for (i = 0; i < ARRAY_SIZE (vectors); i++)
    {
      test64 (vectors[i][0], vectors[i][1]);
    }
  for (i = 0; i < 40000; i++)
    {
      a = 17 * a + 13 * b;
      b = 23 * a + 29 * b;
      if (b == 0)
	b = 1;
      if (a == 0)
	a = 1;
      test64 (a, b);
    }
}

/* Register example_test method as a functional test.  */
holy_FUNCTIONAL_TEST (mul_test, mul_test);
