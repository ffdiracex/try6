/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <stdio.h>
#include <string.h>
#include <holy/test.h>
#include <holy/misc.h>

holy_MOD_LICENSE ("GPLv2+");

#define MSG "cmp test failed"

/* Functional test main method.  */
static void
cmp_test (void)
{
  const char *s1 = "a";
  const char *s2 = "aa";
  const char *s3 = "â";

  holy_test_assert (holy_strlen (s1) == 1, MSG);
  holy_test_assert (holy_strlen (s2) == 2, MSG);
  holy_test_assert (holy_strlen (s3) == 2, MSG);

  holy_test_assert (holy_strcmp (s1, s1) == 0, MSG);
  holy_test_assert (holy_strcmp (s1, s2) < 0, MSG);
  holy_test_assert (holy_strcmp (s1, s3) < 0, MSG);

  holy_test_assert (holy_strcmp (s2, s1) > 0, MSG);
  holy_test_assert (holy_strcmp (s2, s2) == 0, MSG);
  holy_test_assert (holy_strcmp (s2, s3) < 0, MSG);

  holy_test_assert (holy_strcmp (s3, s1) > 0, MSG);
  holy_test_assert (holy_strcmp (s3, s2) > 0, MSG);
  holy_test_assert (holy_strcmp (s3, s3) == 0, MSG);

  holy_test_assert (holy_strcasecmp (s1, s1) == 0, MSG);
  holy_test_assert (holy_strcasecmp (s1, s2) < 0, MSG);
  holy_test_assert (holy_strcasecmp (s1, s3) < 0, MSG);

  holy_test_assert (holy_strcasecmp (s2, s1) > 0, MSG);
  holy_test_assert (holy_strcasecmp (s2, s2) == 0, MSG);
  holy_test_assert (holy_strcasecmp (s2, s3) < 0, MSG);

  holy_test_assert (holy_strcasecmp (s3, s1) > 0, MSG);
  holy_test_assert (holy_strcasecmp (s3, s2) > 0, MSG);
  holy_test_assert (holy_strcasecmp (s3, s3) == 0, MSG);

  holy_test_assert (holy_memcmp (s1, s1, 2) == 0, MSG);
  holy_test_assert (holy_memcmp (s1, s2, 2) < 0, MSG);
  holy_test_assert (holy_memcmp (s1, s3, 2) < 0, MSG);

  holy_test_assert (holy_memcmp (s2, s1, 2) > 0, MSG);
  holy_test_assert (holy_memcmp (s2, s2, 2) == 0, MSG);
  holy_test_assert (holy_memcmp (s2, s3, 2) < 0, MSG);

  holy_test_assert (holy_memcmp (s3, s1, 2) > 0, MSG);
  holy_test_assert (holy_memcmp (s3, s2, 2) > 0, MSG);
  holy_test_assert (holy_memcmp (s3, s3, 2) == 0, MSG);

  holy_test_assert (holy_memcmp (s1, s1, 1) == 0, MSG);
  holy_test_assert (holy_memcmp (s1, s2, 1) == 0, MSG);
  holy_test_assert (holy_memcmp (s1, s3, 1) < 0, MSG);

  holy_test_assert (holy_memcmp (s2, s1, 1) == 0, MSG);
  holy_test_assert (holy_memcmp (s2, s2, 1) == 0, MSG);
  holy_test_assert (holy_memcmp (s2, s3, 1) < 0, MSG);

  holy_test_assert (holy_memcmp (s3, s1, 1) > 0, MSG);
  holy_test_assert (holy_memcmp (s3, s2, 1) > 0, MSG);
  holy_test_assert (holy_memcmp (s3, s3, 1) == 0, MSG);

  holy_test_assert (holy_strncmp (s1, s1, 2) == 0, MSG);
  holy_test_assert (holy_strncmp (s1, s2, 2) < 0, MSG);
  holy_test_assert (holy_strncmp (s1, s3, 2) < 0, MSG);

  holy_test_assert (holy_strncmp (s2, s1, 2) > 0, MSG);
  holy_test_assert (holy_strncmp (s2, s2, 2) == 0, MSG);
  holy_test_assert (holy_strncmp (s2, s3, 2) < 0, MSG);

  holy_test_assert (holy_strncmp (s3, s1, 2) > 0, MSG);
  holy_test_assert (holy_strncmp (s3, s2, 2) > 0, MSG);
  holy_test_assert (holy_strncmp (s3, s3, 2) == 0, MSG);

  holy_test_assert (holy_strncmp (s1, s1, 1) == 0, MSG);
  holy_test_assert (holy_strncmp (s1, s2, 1) == 0, MSG);
  holy_test_assert (holy_strncmp (s1, s3, 1) < 0, MSG);

  holy_test_assert (holy_strncmp (s2, s1, 1) == 0, MSG);
  holy_test_assert (holy_strncmp (s2, s2, 1) == 0, MSG);
  holy_test_assert (holy_strncmp (s2, s3, 1) < 0, MSG);

  holy_test_assert (holy_strncmp (s3, s1, 1) > 0, MSG);
  holy_test_assert (holy_strncmp (s3, s2, 1) > 0, MSG);
  holy_test_assert (holy_strncmp (s3, s3, 1) == 0, MSG);

  holy_test_assert (holy_strncasecmp (s1, s1, 2) == 0, MSG);
  holy_test_assert (holy_strncasecmp (s1, s2, 2) < 0, MSG);
  holy_test_assert (holy_strncasecmp (s1, s3, 2) < 0, MSG);

  holy_test_assert (holy_strncasecmp (s2, s1, 2) > 0, MSG);
  holy_test_assert (holy_strncasecmp (s2, s2, 2) == 0, MSG);
  holy_test_assert (holy_strncasecmp (s2, s3, 2) < 0, MSG);

  holy_test_assert (holy_strncasecmp (s3, s1, 2) > 0, MSG);
  holy_test_assert (holy_strncasecmp (s3, s2, 2) > 0, MSG);
  holy_test_assert (holy_strncasecmp (s3, s3, 2) == 0, MSG);

  holy_test_assert (holy_strncasecmp (s1, s1, 1) == 0, MSG);
  holy_test_assert (holy_strncasecmp (s1, s2, 1) == 0, MSG);
  holy_test_assert (holy_strncasecmp (s1, s3, 1) < 0, MSG);

  holy_test_assert (holy_strncasecmp (s2, s1, 1) == 0, MSG);
  holy_test_assert (holy_strncasecmp (s2, s2, 1) == 0, MSG);
  holy_test_assert (holy_strncasecmp (s2, s3, 1) < 0, MSG);

  holy_test_assert (holy_strncasecmp (s3, s1, 1) > 0, MSG);
  holy_test_assert (holy_strncasecmp (s3, s2, 1) > 0, MSG);
  holy_test_assert (holy_strncasecmp (s3, s3, 1) == 0, MSG);

  holy_test_assert (strlen (s1) == 1, MSG);
  holy_test_assert (strlen (s2) == 2, MSG);
  holy_test_assert (strlen (s3) == 2, MSG);

  holy_test_assert (strcmp (s1, s1) == 0, MSG);
  holy_test_assert (strcmp (s1, s2) < 0, MSG);
  holy_test_assert (strcmp (s1, s3) < 0, MSG);

  holy_test_assert (strcmp (s2, s1) > 0, MSG);
  holy_test_assert (strcmp (s2, s2) == 0, MSG);
  holy_test_assert (strcmp (s2, s3) < 0, MSG);

  holy_test_assert (strcmp (s3, s1) > 0, MSG);
  holy_test_assert (strcmp (s3, s2) > 0, MSG);
  holy_test_assert (strcmp (s3, s3) == 0, MSG);

  holy_test_assert (memcmp (s1, s1, 2) == 0, MSG);
  holy_test_assert (memcmp (s1, s2, 2) < 0, MSG);
  holy_test_assert (memcmp (s1, s3, 2) < 0, MSG);

  holy_test_assert (memcmp (s2, s1, 2) > 0, MSG);
  holy_test_assert (memcmp (s2, s2, 2) == 0, MSG);
  holy_test_assert (memcmp (s2, s3, 2) < 0, MSG);

  holy_test_assert (memcmp (s3, s1, 2) > 0, MSG);
  holy_test_assert (memcmp (s3, s2, 2) > 0, MSG);
  holy_test_assert (memcmp (s3, s3, 2) == 0, MSG);

  holy_test_assert (memcmp (s1, s1, 1) == 0, MSG);
  holy_test_assert (memcmp (s1, s2, 1) == 0, MSG);
  holy_test_assert (memcmp (s1, s3, 1) < 0, MSG);

  holy_test_assert (memcmp (s2, s1, 1) == 0, MSG);
  holy_test_assert (memcmp (s2, s2, 1) == 0, MSG);
  holy_test_assert (memcmp (s2, s3, 1) < 0, MSG);

  holy_test_assert (memcmp (s3, s1, 1) > 0, MSG);
  holy_test_assert (memcmp (s3, s2, 1) > 0, MSG);
  holy_test_assert (memcmp (s3, s3, 1) == 0, MSG);

  holy_test_assert (strncmp (s1, s1, 2) == 0, MSG);
  holy_test_assert (strncmp (s1, s2, 2) < 0, MSG);
  holy_test_assert (strncmp (s1, s3, 2) < 0, MSG);

  holy_test_assert (strncmp (s2, s1, 2) > 0, MSG);
  holy_test_assert (strncmp (s2, s2, 2) == 0, MSG);
  holy_test_assert (strncmp (s2, s3, 2) < 0, MSG);

  holy_test_assert (strncmp (s3, s1, 2) > 0, MSG);
  holy_test_assert (strncmp (s3, s2, 2) > 0, MSG);
  holy_test_assert (strncmp (s3, s3, 2) == 0, MSG);

  holy_test_assert (strncmp (s1, s1, 1) == 0, MSG);
  holy_test_assert (strncmp (s1, s2, 1) == 0, MSG);
  holy_test_assert (strncmp (s1, s3, 1) < 0, MSG);

  holy_test_assert (strncmp (s2, s1, 1) == 0, MSG);
  holy_test_assert (strncmp (s2, s2, 1) == 0, MSG);
  holy_test_assert (strncmp (s2, s3, 1) < 0, MSG);

  holy_test_assert (strncmp (s3, s1, 1) > 0, MSG);
  holy_test_assert (strncmp (s3, s2, 1) > 0, MSG);
  holy_test_assert (strncmp (s3, s3, 1) == 0, MSG);

  holy_test_assert (strncasecmp (s1, s1, 2) == 0, MSG);
  holy_test_assert (strncasecmp (s1, s2, 2) < 0, MSG);
  holy_test_assert (strncasecmp (s1, s3, 2) < 0, MSG);

  holy_test_assert (strncasecmp (s2, s1, 2) > 0, MSG);
  holy_test_assert (strncasecmp (s2, s2, 2) == 0, MSG);
  holy_test_assert (strncasecmp (s2, s3, 2) < 0, MSG);

  holy_test_assert (strncasecmp (s3, s1, 2) > 0, MSG);
  holy_test_assert (strncasecmp (s3, s2, 2) > 0, MSG);
  holy_test_assert (strncasecmp (s3, s3, 2) == 0, MSG);

  holy_test_assert (strncasecmp (s1, s1, 1) == 0, MSG);
  holy_test_assert (strncasecmp (s1, s2, 1) == 0, MSG);
  holy_test_assert (strncasecmp (s1, s3, 1) < 0, MSG);

  holy_test_assert (strncasecmp (s2, s1, 1) == 0, MSG);
  holy_test_assert (strncasecmp (s2, s2, 1) == 0, MSG);
  holy_test_assert (strncasecmp (s2, s3, 1) < 0, MSG);

  holy_test_assert (strncasecmp (s3, s1, 1) > 0, MSG);
  holy_test_assert (strncasecmp (s3, s2, 1) > 0, MSG);
  holy_test_assert (strncasecmp (s3, s3, 1) == 0, MSG);
}

/* Register example_test method as a functional test.  */
holy_UNIT_TEST ("cmp_test", cmp_test);
