/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/test.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

static void
strtoull_testcase (const char *input, int base, unsigned long long expected,
		   int num_digits, holy_err_t error)
{
  char *output;
  unsigned long long value;
  holy_errno = 0;
  value = holy_strtoull(input, &output, base);
  holy_test_assert (holy_errno == error,
		    "unexpected error. Expected %d, got %d. Input \"%s\"",
		    error, holy_errno, input);
  if (holy_errno)
    {
      holy_errno = 0;
      return;
    }
  holy_test_assert (input + num_digits == output,
		    "unexpected number of digits. Expected %d, got %d, input \"%s\"",
		    num_digits, (int) (output - input), input);
  holy_test_assert (value == expected,
		    "unexpected return value. Expected %llu, got %llu, input \"\%s\"",
		    expected, value, input);
}

static void
strtoull_test (void)
{
  strtoull_testcase ("9", 0, 9, 1, holy_ERR_NONE);
  strtoull_testcase ("0xaa", 0, 0xaa, 4, holy_ERR_NONE);
  strtoull_testcase ("0xff", 0, 0xff, 4, holy_ERR_NONE);
  strtoull_testcase ("0", 10, 0, 1, holy_ERR_NONE);
  strtoull_testcase ("8", 8, 0, 0, holy_ERR_BAD_NUMBER);
  strtoull_testcase ("38", 8, 3, 1, holy_ERR_NONE);
  strtoull_testcase ("7", 8, 7, 1, holy_ERR_NONE);
  strtoull_testcase ("1]", 16, 1, 1, holy_ERR_NONE);
  strtoull_testcase ("18446744073709551616", 10, 0, 0, holy_ERR_OUT_OF_RANGE);
}


holy_FUNCTIONAL_TEST (strtoull_test, strtoull_test);
