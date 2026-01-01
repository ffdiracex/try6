/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/datetime.h>
#include <holy/i18n.h>

static const char *const holy_weekday_names[] =
{
  N_("Sunday"),
  N_("Monday"),
  N_("Tuesday"),
  N_("Wednesday"),
  N_("Thursday"),
  N_("Friday"),
  N_("Saturday"),
};

int
holy_get_weekday (struct holy_datetime *datetime)
{
  unsigned a, y, m;

  if (datetime->month <= 2)
    a = 1;
  else
    a = 0;
  y = datetime->year - a;
  m = datetime->month + 12 * a - 2;

  return (datetime->day + y + y / 4 - y / 100 + y / 400 + (31 * m / 12)) % 7;
}

const char *
holy_get_weekday_name (struct holy_datetime *datetime)
{
  return _ (holy_weekday_names[holy_get_weekday (datetime)]);
}

#define SECPERMIN 60
#define SECPERHOUR (60*SECPERMIN)
#define SECPERDAY (24*SECPERHOUR)
#define DAYSPERYEAR 365
#define DAYSPER4YEARS (4*DAYSPERYEAR+1)


void
holy_unixtime2datetime (holy_int32_t nix, struct holy_datetime *datetime)
{
  int i;
  holy_uint8_t months[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  /* In the period of validity of unixtime all years divisible by 4
     are bissextile*/
  /* Convenience: let's have 3 consecutive non-bissextile years
     at the beginning of the counting date. So count from 1901. */
  int days_epoch;
  /* Number of days since 1st Januar, 1901.  */
  unsigned days;
  /* Seconds into current day.  */
  unsigned secs_in_day;
  /* Transform C divisions and modulos to mathematical ones */
  if (nix < 0)
    days_epoch = -(((unsigned) (SECPERDAY-nix-1)) / SECPERDAY);
  else
    days_epoch = ((unsigned) nix) / SECPERDAY;
  secs_in_day = nix - days_epoch * SECPERDAY;
  days = days_epoch + 69 * DAYSPERYEAR + 17;

  datetime->year = 1901 + 4 * (days / DAYSPER4YEARS);
  days %= DAYSPER4YEARS;
  /* On 31st December of bissextile years 365 days from the beginning
     of the year elapsed but year isn't finished yet */
  if (days / DAYSPERYEAR == 4)
    {
      datetime->year += 3;
      days -= 3*DAYSPERYEAR;
    }
  else
    {
      datetime->year += days / DAYSPERYEAR;
      days %= DAYSPERYEAR;
    }
  for (i = 0; i < 12
	 && days >= (i==1 && datetime->year % 4 == 0
		      ? 29 : months[i]); i++)
    days -= (i==1 && datetime->year % 4 == 0
			    ? 29 : months[i]);
  datetime->month = i + 1;
  datetime->day = 1 + days;
  datetime->hour = (secs_in_day / SECPERHOUR);
  secs_in_day %= SECPERHOUR;
  datetime->minute = secs_in_day / SECPERMIN;
  datetime->second = secs_in_day % SECPERMIN;
}
