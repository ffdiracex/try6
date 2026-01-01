/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/datetime.h>
#include <time.h>

holy_err_t
holy_get_datetime (struct holy_datetime *datetime)
{
  struct tm *mytm;
  time_t mytime;

  mytime = time (&mytime);
  mytm = gmtime (&mytime);

  datetime->year = mytm->tm_year + 1900;
  datetime->month = mytm->tm_mon + 1;
  datetime->day = mytm->tm_mday;
  datetime->hour = mytm->tm_hour;
  datetime->minute = mytm->tm_min;
  datetime->second = mytm->tm_sec;

  return holy_ERR_NONE;
}

holy_err_t
holy_set_datetime (struct holy_datetime *datetime __attribute__ ((unused)))
{
  return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		     "no clock setting routine available");
}
