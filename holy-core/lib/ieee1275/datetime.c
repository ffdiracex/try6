/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/datetime.h>
#include <holy/ieee1275/ieee1275.h>
#include <holy/misc.h>
#include <holy/dl.h>
#if defined (__powerpc__) || defined (__sparc__)
#include <holy/cmos.h>
#endif

holy_MOD_LICENSE ("GPLv2+");

static char *rtc = 0;
static int no_ieee1275_rtc = 0;

/* Helper for find_rtc.  */
static int
find_rtc_iter (struct holy_ieee1275_devalias *alias)
{
  if (holy_strcmp (alias->type, "rtc") == 0)
    {
      holy_dprintf ("datetime", "Found RTC %s\n", alias->path);
      rtc = holy_strdup (alias->path);
      return 1;
    }
  return 0;
}

static void
find_rtc (void)
{
  holy_ieee1275_devices_iterate (find_rtc_iter);
  if (!rtc)
    no_ieee1275_rtc = 1;
}

holy_err_t
holy_get_datetime (struct holy_datetime *datetime)
{
  struct get_time_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t method;
    holy_ieee1275_cell_t device;
    holy_ieee1275_cell_t catch_result;
    holy_ieee1275_cell_t year;
    holy_ieee1275_cell_t month;
    holy_ieee1275_cell_t day;
    holy_ieee1275_cell_t hour;
    holy_ieee1275_cell_t minute;
    holy_ieee1275_cell_t second;
  }
  args;
  int status;
  holy_ieee1275_ihandle_t ihandle;

  if (no_ieee1275_rtc)
    return holy_get_datetime_cmos (datetime);
  if (!rtc)
    find_rtc ();
  if (!rtc)
    return holy_get_datetime_cmos (datetime);

  status = holy_ieee1275_open (rtc, &ihandle);
  if (status == -1)
    return holy_error (holy_ERR_IO, "couldn't open RTC");

  INIT_IEEE1275_COMMON (&args.common, "call-method", 2, 7);
  args.device = (holy_ieee1275_cell_t) ihandle;
  args.method = (holy_ieee1275_cell_t) "get-time";

  status = IEEE1275_CALL_ENTRY_FN (&args);

  holy_ieee1275_close (ihandle);

  if (status == -1 || args.catch_result)
    return holy_error (holy_ERR_IO, "get-time failed");

  datetime->year = args.year;
  datetime->month = args.month;
  datetime->day = args.day + 1;
  datetime->hour = args.hour;
  datetime->minute = args.minute;
  datetime->second = args.second;

  return holy_ERR_NONE;
}

holy_err_t
holy_set_datetime (struct holy_datetime *datetime)
{
  struct set_time_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t method;
    holy_ieee1275_cell_t device;
    holy_ieee1275_cell_t year;
    holy_ieee1275_cell_t month;
    holy_ieee1275_cell_t day;
    holy_ieee1275_cell_t hour;
    holy_ieee1275_cell_t minute;
    holy_ieee1275_cell_t second;
    holy_ieee1275_cell_t catch_result;
  }
  args;
  int status;
  holy_ieee1275_ihandle_t ihandle;

  if (no_ieee1275_rtc)
    return holy_set_datetime_cmos (datetime);
  if (!rtc)
    find_rtc ();
  if (!rtc)
    return holy_set_datetime_cmos (datetime);

  status = holy_ieee1275_open (rtc, &ihandle);
  if (status == -1)
    return holy_error (holy_ERR_IO, "couldn't open RTC");

  INIT_IEEE1275_COMMON (&args.common, "call-method", 8, 1);
  args.device = (holy_ieee1275_cell_t) ihandle;
  args.method = (holy_ieee1275_cell_t) "set-time";

  args.year = datetime->year;
  args.month = datetime->month;
  args.day = datetime->day - 1;
  args.hour = datetime->hour;
  args.minute = datetime->minute;
  args.second = datetime->second;

  status = IEEE1275_CALL_ENTRY_FN (&args);

  holy_ieee1275_close (ihandle);

  if (status == -1 || args.catch_result)
    return holy_error (holy_ERR_IO, "set-time failed");

  return holy_ERR_NONE;
}
