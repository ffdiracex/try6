/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/test.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

/* Functional test main method.  */
static void
example_test (void)
{
  /* Check if 1st argument is true and report with default error message.  */
  holy_test_assert (1 == 1, "1 equal 1 expected");

  /* Check if 1st argument is true and report with custom error message.  */
  holy_test_assert (2 == 2, "2 equal 2 expected");
  holy_test_assert (2 != 3, "2 matches %d", 3);
}

/* Register example_test method as a functional test.  */
holy_FUNCTIONAL_TEST (exfctest, example_test);
