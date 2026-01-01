/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/test.h>
#include <holy/dl.h>
#include <holy/misc.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_uint64_t vectors[] = {
  0xffffffffffffffffULL, 1, 2, 0, 0x0102030405060708ULL
};

static void
test16 (holy_uint16_t a)
{
  holy_uint16_t b, c;
  holy_uint8_t *ap, *bp;
  int i;
  b = holy_swap_bytes16 (a);
  c = holy_swap_bytes16 (b);
  holy_test_assert (a == c, "bswap not idempotent: 0x%llx, 0x%llx, 0x%llx",
		    (long long) a, (long long) b, (long long) c);
  ap = (holy_uint8_t *) &a;
  bp = (holy_uint8_t *) &b;
  for (i = 0; i < 2; i++)
    {
      holy_test_assert (ap[i] == bp[1 - i],
			"bswap bytes wrong: 0x%llx, 0x%llx",
			(long long) a, (long long) b);
    }
}

static void
test32 (holy_uint32_t a)
{
  holy_uint32_t b, c;
  holy_uint8_t *ap, *bp;
  int i;
  b = holy_swap_bytes32 (a);
  c = holy_swap_bytes32 (b);
  holy_test_assert (a == c, "bswap not idempotent: 0x%llx, 0x%llx, 0x%llx",
		    (long long) a, (long long) b, (long long) c);
  ap = (holy_uint8_t *) &a;
  bp = (holy_uint8_t *) &b;
  for (i = 0; i < 4; i++)
    {
      holy_test_assert (ap[i] == bp[3 - i],
			"bswap bytes wrong: 0x%llx, 0x%llx",
			(long long) a, (long long) b);
    }
}

static void
test64 (holy_uint64_t a)
{
  holy_uint64_t b, c;
  holy_uint8_t *ap, *bp;
  int i;
  b = holy_swap_bytes64 (a);
  c = holy_swap_bytes64 (b);
  holy_test_assert (a == c, "bswap not idempotent: 0x%llx, 0x%llx, 0x%llx",
		    (long long) a, (long long) b, (long long) c);
  ap = (holy_uint8_t *) &a;
  bp = (holy_uint8_t *) &b;
  for (i = 0; i < 4; i++)
    {
      holy_test_assert (ap[i] == bp[7 - i],
			"bswap bytes wrong: 0x%llx, 0x%llx",
			(long long) a, (long long) b);
    }
}

static void
test_all(holy_uint64_t a)
{
  test64 (a);
  test32 (a);
  test16 (a);
}

static void
bswap_test (void)
{
  holy_uint64_t a = 404, b = 7;
  holy_size_t i;

  for (i = 0; i < ARRAY_SIZE (vectors); i++)
    {
      test_all (vectors[i]);
    }
  for (i = 0; i < 40000; i++)
    {
      a = 17 * a + 13 * b;
      b = 23 * a + 29 * b;
      if (b == 0)
	b = 1;
      if (a == 0)
	a = 1;
      test_all (a);
      test_all (b);
    }
}

/* Register example_test method as a functional test.  */
holy_FUNCTIONAL_TEST (bswap_test, bswap_test);
