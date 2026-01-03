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

/* Don't change those to use shift as shift may call to compile rt
   functions and we're not testing them now.
 */
static int
leading_bit64 (holy_uint64_t a)
{
  return !!(a & 0x8000000000000000LL);
}

static int
leading_bit32 (holy_uint32_t a)
{
  return !!(a & 0x80000000);
}

/* Computes (a < b) without involving comparison operator.  */
static int
is_less32 (holy_uint32_t a, holy_uint32_t b)
{
  if (leading_bit32(a) && !leading_bit32(b))
    return 0;
  if (!leading_bit32(a) && leading_bit32(b))
    return 1;
  return leading_bit32(a - b);
}

static void
test32 (holy_uint32_t a, holy_uint32_t b)
{
  holy_test_assert ((a < b) == is_less32(a, b), "comparison result mismatch: %lld, %lld",
		    (long long) a, (long long) b);
  holy_test_assert ((a > b) == is_less32(b, a), "comparison result mismatch: %lld, %lld",
		    (long long) a, (long long) b);
  holy_test_assert ((b < a) == is_less32(b, a), "comparison result mismatch: %lld, %lld",
		    (long long) a, (long long) b);
  holy_test_assert ((b > a) == is_less32(a, b), "comparison result mismatch: %lld, %lld",
		    (long long) a, (long long) b);
  holy_test_assert (!(is_less32(a, b) && is_less32(b, a)), "comparison inconsistent: %lld, %lld",
		    (long long) a, (long long) b);
}

/* Computes (a > b) without involving comparison operator.  */
static int
is_less32s (holy_int32_t a, holy_int32_t b)
{
  if (leading_bit32(a) && !leading_bit32(b))
    return 1; /* a < 0 && b >= 0. */
  if (!leading_bit32(a) && leading_bit32(b))
    return 0; /* b < 0 && a >= 0. */
  return leading_bit32(a - b);
}

static void
test32s (holy_int32_t a, holy_int32_t b)
{
  holy_test_assert ((a < b) == is_less32s(a, b), "comparison result mismatch: %lld, %lld",
		    (long long) a, (long long) b);
  holy_test_assert ((a > b) == is_less32s(b, a), "comparison result mismatch: %lld, %lld",
		    (long long) a, (long long) b);
  holy_test_assert ((b < a) == is_less32s(b, a), "comparison result mismatch: %lld, %lld",
		    (long long) a, (long long) b);
  holy_test_assert ((b > a) == is_less32s(a, b), "comparison result mismatch: %lld, %lld",
		    (long long) a, (long long) b);
  holy_test_assert (!(is_less32s(a, b) && is_less32s(b, a)), "comparison inconsistent: %lld, %lld",
		    (long long) a, (long long) b);
}

/* Computes (a > b) without involving comparison operator.  */
static int
is_less64 (holy_uint64_t a, holy_uint64_t b)
{
  if (leading_bit64(a) && !leading_bit64(b))
    return 0;
  if (!leading_bit64(a) && leading_bit64(b))
    return 1;
  return leading_bit64(a - b);
}

static void
test64 (holy_uint64_t a, holy_uint64_t b)
{
  holy_test_assert ((a < b) == is_less64(a, b), "comparison result mismatch: %lld, %lld",
		    (long long) a, (long long) b);
  holy_test_assert ((a > b) == is_less64(b, a), "comparison result mismatch: %lld, %lld",
		    (long long) a, (long long) b);
  holy_test_assert ((b < a) == is_less64(b, a), "comparison result mismatch: %lld, %lld",
		    (long long) a, (long long) b);
  holy_test_assert ((b > a) == is_less64(a, b), "comparison result mismatch: %lld, %lld",
		    (long long) a, (long long) b);
  holy_test_assert (!(is_less64(a, b) && is_less64(b, a)), "comparison inconsistent: %lld, %lld",
		    (long long) a, (long long) b);
}

/* Computes (a > b) without involving comparison operator.  */
static int
is_less64s (holy_int64_t a, holy_int64_t b)
{
  if (leading_bit64(a) && !leading_bit64(b))
    return 1; /* a < 0 && b >= 0. */
  if (!leading_bit64(a) && leading_bit64(b))
    return 0; /* b < 0 && a >= 0. */
  return leading_bit64(a - b);
}

static void
test64s (holy_int64_t a, holy_int64_t b)
{
  holy_test_assert ((a < b) == is_less64s(a, b), "comparison result mismatch: %lld, %lld",
		    (long long) a, (long long) b);
  holy_test_assert ((a > b) == is_less64s(b, a), "comparison result mismatch: %lld, %lld",
		    (long long) a, (long long) b);
  holy_test_assert ((b < a) == is_less64s(b, a), "comparison result mismatch: %lld, %lld",
		    (long long) a, (long long) b);
  holy_test_assert ((b > a) == is_less64s(a, b), "comparison result mismatch: %lld, %lld",
		    (long long) a, (long long) b);
  holy_test_assert (!(is_less64s(a, b) && is_less64s(b, a)), "comparison inconsistent: %lld, %lld",
		    (long long) a, (long long) b);
}

static void
test_all(holy_uint64_t a, holy_uint64_t b)
{
  test64 (a, b);
  test32 (a, b);
  test64s (a, b);
  test32s (a, b);
  test64s (a, -b);
  test32s (a, -b);
  test64s (-a, b);
  test32s (-a, b);
  test64s (-a, -b);
  test32s (-a, -b);
}

static void
cmp_test (void)
{
  holy_uint64_t a = 404, b = 7;
  holy_size_t i;

  for (i = 0; i < ARRAY_SIZE (vectors); i++)
    {
      test_all (vectors[i][0], vectors[i][1]);
    }
  for (i = 0; i < 40000; i++)
    {
      a = 17 * a + 13 * b;
      b = 23 * a + 29 * b;
      if (b == 0)
	b = 1;
      if (a == 0)
	a = 1;
      test_all (a, b);
    }
}

/* Register example_test method as a functional test.  */
holy_FUNCTIONAL_TEST (cmp_test, cmp_test);
