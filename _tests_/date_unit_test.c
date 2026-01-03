/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#include <holy/misc.h>
#include <holy/datetime.h>
#include <holy/test.h>

static void
date_test (holy_int32_t v)
{
  struct holy_datetime dt;
  time_t t = v;
  struct tm *g;
  int w;

  g = gmtime (&t);

  holy_unixtime2datetime (v, &dt);

  w = holy_get_weekday (&dt);

  holy_test_assert (g->tm_sec == dt.second, "time %d bad second: %d vs %d", v,
		    g->tm_sec, dt.second);
  holy_test_assert (g->tm_min == dt.minute, "time %d bad minute: %d vs %d", v,
		    g->tm_min, dt.minute);
  holy_test_assert (g->tm_hour == dt.hour, "time %d bad hour: %d vs %d", v,
		    g->tm_hour, dt.hour);
  holy_test_assert (g->tm_mday == dt.day, "time %d bad day: %d vs %d", v,
		    g->tm_mday, dt.day);
  holy_test_assert (g->tm_mon + 1 == dt.month, "time %d bad month: %d vs %d", v,
		    g->tm_mon + 1, dt.month);
  holy_test_assert (g->tm_year + 1900 == dt.year,
		    "time %d bad year: %d vs %d", v,
		    g->tm_year + 1900, dt.year);
  holy_test_assert (g->tm_wday == w, "time %d bad week day: %d vs %d", v,
		    g->tm_wday, w);
}

static void
date_test_iter (void)
{
  holy_int32_t tests[] = { -1, 0, +1, -2133156255, holy_INT32_MIN,
			   holy_INT32_MAX };
  unsigned i;

  for (i = 0; i < ARRAY_SIZE (tests); i++)
    date_test (tests[i]);
  srand (42);
  for (i = 0; i < 1000000; i++)
    {
      holy_int32_t x = rand ();
      date_test (x);
      date_test (-x);
    }
}

holy_UNIT_TEST ("date_unit_test", date_test_iter);
