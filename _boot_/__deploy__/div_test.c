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
test32 (holy_uint32_t a, holy_uint32_t b)
{
  holy_uint64_t q, r;
  if (b == 0)
    return;
  q = holy_divmod64 (a, b, &r);
  holy_test_assert (r < b, "remainder is larger than dividend: 0x%llx %% 0x%llx = 0x%llx",
		    (long long) a, (long long) b, (long long) r);
  holy_test_assert (q * b + r == a, "division doesn't satisfy base property: 0x%llx * 0x%llx + 0x%llx != 0x%llx", (long long) q, (long long) b, (long long) r,
		    (long long) a);
  /* Overflow check.  */
  holy_test_assert ((q >> 32) == 0,
		    "division overflow in 0x%llx, 0x%llx", (long long) a, (long long) b);
  holy_test_assert ((r >> 32) == 0,
		    "division overflow in 0x%llx, 0x%llx", (long long) a, (long long) b);
  /* q * b + r is at most:
     0xffffffff * 0xffffffff + 0xffffffff = 0xffffffff00000000
     so no overflow
   */
  holy_test_assert (q == (a / b),
		    "C compiler division failure in 0x%llx, 0x%llx", (long long) a, (long long) b);
  holy_test_assert (r == (a % b),
		    "C compiler modulo failure in 0x%llx, 0x%llx", (long long) a, (long long) b);
}

static void
test64 (holy_uint64_t a, holy_uint64_t b)
{
  holy_uint64_t q, r;
  holy_uint64_t x1, x2;
  q = holy_divmod64 (a, b, &r);
  holy_test_assert (r < b, "remainder is larger than dividend: 0x%llx %% 0x%llx = 0x%llx",
		    (long long) a, (long long) b, (long long) r);
  holy_test_assert (q * b + r == a, "division doesn't satisfy base property: 0x%llx * 0x%llx + 0x%llx != 0x%llx", (long long) q, (long long) b, (long long) r,
		    (long long) a);
  /* Overflow checks.  */
  holy_test_assert ((q >> 32) * (b >> 32) == 0,
		    "division overflow in 0x%llx, 0x%llx", (long long) a, (long long) b);
  x1 = (q >> 32) * (b & 0xffffffff);
  holy_test_assert (x1 < (1LL << 32),
		    "division overflow in 0x%llx, 0x%llx", (long long) a, (long long) b);
  x1 <<= 32;
  x2 = (b >> 32) * (q & 0xffffffff);
  holy_test_assert (x2 < (1LL << 32),
		    "division overflow in 0x%llx, 0x%llx", (long long) a, (long long) b);
  x2 <<= 32;
  holy_test_assert (x1 <= ~x2,
		    "division overflow in 0x%llx, 0x%llx", (long long) a, (long long) b);
  x1 += x2;
  x2 = (q & 0xffffffff) * (b & 0xffffffff);
  holy_test_assert (x1 <= ~x2,
		    "division overflow in 0x%llx, 0x%llx", (long long) a, (long long) b);
  x1 += x2;
  holy_test_assert (x1 <= ~r,
		    "division overflow in 0x%llx, 0x%llx", (long long) a, (long long) b);
  x1 += r;
  holy_test_assert (a == x1,
		    "division overflow test failure in 0x%llx, 0x%llx", (long long) a, (long long) b);
#if holy_TARGET_SIZEOF_VOID_P == 8
  holy_test_assert (q == (a / b),
		    "C compiler division failure in 0x%llx, 0x%llx", (long long) a, (long long) b);
  holy_test_assert (r == (a % b),
		    "C compiler modulo failure in 0x%llx, 0x%llx", (long long) a, (long long) b);
#endif
}

static holy_int64_t
abs64(holy_int64_t a)
{
  return a > 0 ? a : -a;
}

static void
test32s (holy_int32_t a, holy_int32_t b)
{
  holy_int64_t q, r;
  if (b == 0)
    return;

  q = holy_divmod64s (a, b, &r);
  holy_test_assert (a > 0 ? r >= 0 : r <= 0, "remainder sign mismatch: %lld %% %lld = %lld",
		    (long long) a, (long long) b, (long long) r);
  holy_test_assert (((a > 0) == (b > 0)) ? q >= 0 : q <= 0, "quotient sign mismatch: %lld / %lld = %lld",
		    (long long) a, (long long) b, (long long) q);
  holy_test_assert (abs64(r) < abs64(b), "remainder is larger than dividend: %lld %% %lld = %lld",
		    (long long) a, (long long) b, (long long) r);
  holy_test_assert (q * b + r == a, "division doesn't satisfy base property: %lld * %lld + %lld != %lld", (long long) q, (long long) b, (long long) r,
		    (long long) a);
  if (0) {  holy_test_assert (q == (a / b),
		    "C compiler division failure in 0x%llx, 0x%llx", (long long) a, (long long) b);
  holy_test_assert (r == (a % b),
		    "C compiler modulo failure in 0x%llx, 0x%llx", (long long) a, (long long) b);
  }
}

static void
test64s (holy_int64_t a, holy_int64_t b)
{
  holy_int64_t q, r;
  q = holy_divmod64s (a, b, &r);

  holy_test_assert (a > 0 ? r >= 0 : r <= 0, "remainder sign mismatch: %lld %% %lld = %lld",
		    (long long) a, (long long) b, (long long) r);
  holy_test_assert (((a > 0) == (b > 0)) ? q >= 0 : q <= 0, "quotient sign mismatch: %lld / %lld = %lld",
		    (long long) a, (long long) b, (long long) q);
  holy_test_assert (abs64(r) < abs64(b), "remainder is larger than dividend: %lld %% %lld = %lld",
		    (long long) a, (long long) b, (long long) r);
  holy_test_assert (q * b + r == a, "division doesn't satisfy base property: 0x%llx * 0x%llx + 0x%llx != 0x%llx", (long long) q, (long long) b, (long long) r,
		    (long long) a);
#if holy_TARGET_SIZEOF_VOID_P == 8
  holy_test_assert (q == (a / b),
		    "C compiler division failure in 0x%llx, 0x%llx", (long long) a, (long long) b);
  holy_test_assert (r == (a % b),
		    "C compiler modulo failure in 0x%llx, 0x%llx", (long long) a, (long long) b);
#endif
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
div_test (void)
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
holy_FUNCTIONAL_TEST (div_test, div_test);
