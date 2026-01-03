/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <string.h>

/* All tests need to include test.h for holy testing framework.  */
#include <holy/test.h>

/* Unit test main method.  */
static void
example_test (void)
{
  /* Check if 1st argument is true and report with default error message.  */
  holy_test_assert (1 == 1, "1 equal 1 expected");

  /* Check if 1st argument is true and report with custom error message.  */
  holy_test_assert (2 == 2, "2 equal 2 expected");
  holy_test_assert (2 != 3, "2 matches %d", 3);
}

/* Register example_test method as a unit test.  */
holy_UNIT_TEST ("example_unit_test", example_test);
