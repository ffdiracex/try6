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

/* We're testing shifts, don't replace access to this with a shift.  */
static const holy_uint8_t bitmask[] =
  { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80 };

typedef union {
  holy_uint64_t v64;
  holy_uint8_t v8[8];
} holy_raw_u64_t;

static int
get_bit64 (holy_uint64_t v, int b)
{
  holy_raw_u64_t vr = { .v64 = v };
  holy_uint8_t *p = vr.v8;
  if (b >= 64)
    return 0;
#ifdef holy_CPU_WORDS_BIGENDIAN
  p += 7 - b / 8;
#else
  p += b / 8;
#endif
  return !!(*p & bitmask[b % 8]);
}

static holy_uint64_t
set_bit64 (holy_uint64_t v, int b)
{
  holy_raw_u64_t vr = { .v64 = v };
  holy_uint8_t *p = vr.v8;
  if (b >= 64)
    return v;
#ifdef holy_CPU_WORDS_BIGENDIAN
  p += 7 - b / 8;
#else
  p += b / 8;
#endif
  *p |= bitmask[b % 8];
  return vr.v64;
}

static holy_uint64_t
left_shift64 (holy_uint64_t v, int s)
{
  holy_uint64_t r = 0;
  int i;
  for (i = 0; i + s < 64; i++)
    if (get_bit64 (v, i))
      r = set_bit64 (r, i + s);
  return r;
}

static holy_uint64_t
right_shift64 (holy_uint64_t v, int s)
{
  holy_uint64_t r = 0;
  int i;
  for (i = s; i < 64; i++)
    if (get_bit64 (v, i))
      r = set_bit64 (r, i - s);
  return r;
}

static holy_uint64_t
arithmetic_right_shift64 (holy_uint64_t v, int s)
{
  holy_uint64_t r = 0;
  int i;
  for (i = s; i < 64; i++)
    if (get_bit64 (v, i))
      r = set_bit64 (r, i - s);
  if (get_bit64 (v, 63))
    for (i -= s; i < 64; i++)
	r = set_bit64 (r, i);
    
  return r;
}

static void
test64 (holy_uint64_t v)
{
  int i;
  for (i = 0; i < 64; i++)
    {
      holy_test_assert ((v << i) == left_shift64 (v, i),
			"lshift wrong: 0x%llx << %d: 0x%llx, 0x%llx",
			(long long) v, i,
			(long long) (v << i), (long long) left_shift64 (v, i));
      holy_test_assert ((v >> i) == right_shift64 (v, i),
			"rshift wrong: 0x%llx >> %d: 0x%llx, 0x%llx",
			(long long) v, i,
			(long long) (v >> i), (long long) right_shift64 (v, i));
      holy_test_assert ((((holy_int64_t) v) >> i) == (holy_int64_t) arithmetic_right_shift64 (v, i),
			"arithmetic rshift wrong: ((holy_int64_t) 0x%llx) >> %d: 0x%llx, 0x%llx",
			(long long) v, i,
			(long long) (((holy_int64_t) v) >> i), (long long) arithmetic_right_shift64 (v, i));
    }
}

static void
test_all(holy_uint64_t a)
{
  test64 (a);
}

static void
shift_test (void)
{
  holy_uint64_t a = 404, b = 7;
  holy_size_t i;

  for (i = 0; i < ARRAY_SIZE (vectors); i++)
    {
      test_all (vectors[i]);
    }
  for (i = 0; i < 4000; i++)
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
holy_FUNCTIONAL_TEST (shift_test, shift_test);
