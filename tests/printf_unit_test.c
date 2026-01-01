/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <stdio.h>
#include <string.h>
#include <holy/test.h>
#include <holy/misc.h>

#define MSG "printf test failed: %s, %s", real, expected

static void
printf_test (void)
{
  char real[512];
  char expected[512];
  char *null = NULL;

  holy_snprintf (real, sizeof (real), "%s", null);
  snprintf (expected, sizeof (expected), "%s", null);
  holy_test_assert (strcmp (real, expected) == 0, MSG);

  holy_snprintf (real, sizeof (real), "%10s", null);
  snprintf (expected, sizeof (expected), "%10s", null);
  holy_test_assert (strcmp (real, expected) == 0, MSG);

  holy_snprintf (real, sizeof (real), "%-10s", null);
  snprintf (expected, sizeof (expected), "%-10s", null);
  holy_test_assert (strcmp (real, expected) == 0, MSG);

  holy_snprintf (real, sizeof (real), "%d%%", 10);
  snprintf (expected, sizeof (expected), "%d%%", 10);
  holy_test_assert (strcmp (real, expected) == 0, MSG);

  holy_snprintf (real, sizeof (real), "%d %%", 10);
  snprintf (expected, sizeof (expected), "%d %%", 10);
  holy_test_assert (strcmp (real, expected) == 0, MSG);

  holy_snprintf (real, sizeof (real), "%%");
  snprintf (expected, sizeof (expected), "%%");
  holy_test_assert (strcmp (real, expected) == 0, MSG);

  holy_snprintf (real, sizeof (real), "%d %d %d", 1, 2, 3);
  snprintf (expected, sizeof (expected), "%d %d %d", 1, 2, 3);
  holy_test_assert (strcmp (real, expected) == 0, MSG);
  holy_snprintf (real, sizeof (real), "%3$d %2$d %1$d", 1, 2, 3);
  snprintf (expected, sizeof (expected), "%3$d %2$d %1$d", 1, 2, 3);
  holy_test_assert (strcmp (real, expected) == 0, MSG);
  holy_snprintf (real, sizeof (real), "%d %lld %d", 1, 2LL, 3);
  snprintf (expected, sizeof (expected), "%d %lld %d", 1, 2LL, 3);
  holy_test_assert (strcmp (real, expected) == 0, MSG);
  holy_snprintf (real, sizeof (real), "%3$d %2$lld %1$d", 1, 2LL, 3);
  snprintf (expected, sizeof (expected), "%3$d %2$lld %1$d", 1, 2LL, 3);
  holy_test_assert (strcmp (real, expected) == 0, MSG);
  holy_snprintf (real, sizeof (real), "%%0%dd ", 1);
  snprintf (expected, sizeof (expected), "%%0%dd ", 1);
  holy_test_assert (strcmp (real, expected) == 0, MSG);
}

holy_UNIT_TEST ("printf_unit_test", printf_test);
