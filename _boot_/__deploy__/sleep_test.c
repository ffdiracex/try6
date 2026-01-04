/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/time.h>
#include <holy/misc.h>
#include <holy/dl.h>
#include <holy/command.h>
#include <holy/env.h>
#include <holy/test.h>
#include <holy/mm.h>
#include <holy/datetime.h>
#include <holy/time.h>

holy_MOD_LICENSE ("GPLv2+");

static void
sleep_test (void)
{
  struct holy_datetime st, en;
  holy_int32_t stu = 0, enu = 0;
  int is_delayok;
  holy_test_assert (!holy_get_datetime (&st), "Couldn't retrieve start time");
  holy_millisleep (10000);
  holy_test_assert (!holy_get_datetime (&en), "Couldn't retrieve end time");
  holy_test_assert (holy_datetime2unixtime (&st, &stu), "Invalid date");
  holy_test_assert (holy_datetime2unixtime (&en, &enu), "Invalid date");
  is_delayok = (enu - stu >= 9 && enu - stu <= 11);
#ifdef __arm__
  /* Ignore QEMU bug */
  if (enu - stu >= 15 && enu - stu <= 17)
    is_delayok = 1;
#endif
  holy_test_assert (is_delayok, "Interval out of range: %d", enu-stu);

}

holy_FUNCTIONAL_TEST (sleep_test, sleep_test);
