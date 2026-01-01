/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef KERNEL_DATETIME_HEADER
#define KERNEL_DATETIME_HEADER	1

#include <holy/types.h>
#include <holy/err.h>

struct holy_datetime
{
  holy_uint16_t year;
  holy_uint8_t month;
  holy_uint8_t day;
  holy_uint8_t hour;
  holy_uint8_t minute;
  holy_uint8_t second;
};

/* Return date and time.  */
#ifdef holy_MACHINE_EMU
holy_err_t EXPORT_FUNC(holy_get_datetime) (struct holy_datetime *datetime);

/* Set date and time.  */
holy_err_t EXPORT_FUNC(holy_set_datetime) (struct holy_datetime *datetime);
#else
holy_err_t holy_get_datetime (struct holy_datetime *datetime);

/* Set date and time.  */
holy_err_t holy_set_datetime (struct holy_datetime *datetime);
#endif

int holy_get_weekday (struct holy_datetime *datetime);
const char *holy_get_weekday_name (struct holy_datetime *datetime);

void holy_unixtime2datetime (holy_int32_t nix,
			     struct holy_datetime *datetime);

static inline int
holy_datetime2unixtime (const struct holy_datetime *datetime, holy_int32_t *nix)
{
  holy_int32_t ret;
  int y4, ay;
  const holy_uint16_t monthssum[12]
    = { 0,
	31, 
	31 + 28,
	31 + 28 + 31,
	31 + 28 + 31 + 30,
	31 + 28 + 31 + 30 + 31, 
	31 + 28 + 31 + 30 + 31 + 30,
	31 + 28 + 31 + 30 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31,
	31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30};
  const holy_uint8_t months[12] = {31, 28, 31, 30, 31, 30,
				   31, 31, 30, 31, 30, 31};
  const int SECPERMIN = 60;
  const int SECPERHOUR = 60 * SECPERMIN;
  const int SECPERDAY = 24 * SECPERHOUR;
  const int SECPERYEAR = 365 * SECPERDAY;
  const int SECPER4YEARS = 4 * SECPERYEAR + SECPERDAY;

  if (datetime->year > 2038 || datetime->year < 1901)
    return 0;
  if (datetime->month > 12 || datetime->month < 1)
    return 0;

  /* In the period of validity of unixtime all years divisible by 4
     are bissextile*/
  /* Convenience: let's have 3 consecutive non-bissextile years
     at the beginning of the epoch. So count from 1973 instead of 1970 */
  ret = 3 * SECPERYEAR + SECPERDAY;

  /* Transform C divisions and modulos to mathematical ones */
  y4 = ((datetime->year - 1) >> 2) - (1973 / 4);
  ay = datetime->year - 1973 - 4 * y4;
  ret += y4 * SECPER4YEARS;
  ret += ay * SECPERYEAR;

  ret += monthssum[datetime->month - 1] * SECPERDAY;
  if (ay == 3 && datetime->month >= 3)
    ret += SECPERDAY;

  ret += (datetime->day - 1) * SECPERDAY;
  if ((datetime->day > months[datetime->month - 1]
       && (!ay || datetime->month != 2 || datetime->day != 29))
      || datetime->day < 1)
    return 0;

  ret += datetime->hour * SECPERHOUR;
  if (datetime->hour > 23)
    return 0;
  ret += datetime->minute * 60;
  if (datetime->minute > 59)
    return 0;

  ret += datetime->second;
  /* Accept leap seconds.  */
  if (datetime->second > 60)
    return 0;

  if ((datetime->year > 1980 && ret < 0)
      || (datetime->year < 1960 && ret > 0))
    return 0;
  *nix = ret;
  return 1;
}

#if (defined (__powerpc__) || defined (__sparc__)) && !defined (holy_UTIL)
holy_err_t
holy_get_datetime_cmos (struct holy_datetime *datetime);
holy_err_t
holy_set_datetime_cmos (struct holy_datetime *datetime);
#endif

#endif /* ! KERNEL_DATETIME_HEADER */
