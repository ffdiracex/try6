/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/datetime.h>
#include <holy/cmos.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

#if !defined (__powerpc__) && !defined (__sparc__)
#define holy_get_datetime_cmos holy_get_datetime
#define holy_set_datetime_cmos holy_set_datetime
#endif

holy_err_t
holy_get_datetime_cmos (struct holy_datetime *datetime)
{
  int is_bcd, is_12hour;
  holy_uint8_t value, flag;
  holy_err_t err;

  err = holy_cmos_read (holy_CMOS_INDEX_STATUS_B, &flag);
  if (err)
    return err;

  is_bcd = ! (flag & holy_CMOS_STATUS_B_BINARY);

  err = holy_cmos_read (holy_CMOS_INDEX_YEAR, &value);
  if (err)
    return err;
  if (is_bcd)
    value = holy_bcd_to_num (value);

  datetime->year = value;
  datetime->year += (value < 80) ? 2000 : 1900;

  err = holy_cmos_read (holy_CMOS_INDEX_MONTH, &value);
  if (err)
    return err;
  if (is_bcd)
    value = holy_bcd_to_num (value);

  datetime->month = value;

  err = holy_cmos_read (holy_CMOS_INDEX_DAY_OF_MONTH, &value);
  if (err)
    return err;
  if (is_bcd)
    value = holy_bcd_to_num (value);

  datetime->day = value;

  is_12hour = ! (flag & holy_CMOS_STATUS_B_24HOUR);

  err = holy_cmos_read (holy_CMOS_INDEX_HOUR, &value);
  if (err)
    return err;
  if (is_12hour)
    {
      is_12hour = (value & 0x80);

      value &= 0x7F;
      value--;
    }

  if (is_bcd)
    value = holy_bcd_to_num (value);

  if (is_12hour)
    value += 12;

  datetime->hour = value;

  err = holy_cmos_read (holy_CMOS_INDEX_MINUTE, &value);
  if (err)
    return err;

  if (is_bcd)
    value = holy_bcd_to_num (value);

  datetime->minute = value;

  err = holy_cmos_read (holy_CMOS_INDEX_SECOND, &value);
  if (err)
    return err;
  if (is_bcd)
    value = holy_bcd_to_num (value);

  datetime->second = value;

  return 0;
}

holy_err_t
holy_set_datetime_cmos (struct holy_datetime *datetime)
{
  int is_bcd, is_12hour;
  holy_uint8_t value, flag;
  holy_err_t err;

  err = holy_cmos_read (holy_CMOS_INDEX_STATUS_B, &flag);
  if (err)
    return err;

  is_bcd = ! (flag & holy_CMOS_STATUS_B_BINARY);

  value = ((datetime->year >= 2000) ? datetime->year - 2000 :
           datetime->year - 1900);

  if (is_bcd)
    value = holy_num_to_bcd (value);

  err = holy_cmos_write (holy_CMOS_INDEX_YEAR, value);
  if (err)
    return err;

  value = datetime->month;

  if (is_bcd)
    value = holy_num_to_bcd (value);

  err = holy_cmos_write (holy_CMOS_INDEX_MONTH, value);
  if (err)
    return err;

  value = datetime->day;

  if (is_bcd)
    value = holy_num_to_bcd (value);

  err = holy_cmos_write (holy_CMOS_INDEX_DAY_OF_MONTH, value);
  if (err)
    return err;

  value = datetime->hour;

  is_12hour = (! (flag & holy_CMOS_STATUS_B_24HOUR));

  if (is_12hour)
    {
      value++;

      if (value > 12)
        value -= 12;
      else
        is_12hour = 0;
    }

  if (is_bcd)
    value = holy_num_to_bcd (value);

  if (is_12hour)
    value |= 0x80;

  err = holy_cmos_write (holy_CMOS_INDEX_HOUR, value);
  if (err)
    return err;

  value = datetime->minute;

  if (is_bcd)
    value = holy_num_to_bcd (value);

  err = holy_cmos_write (holy_CMOS_INDEX_MINUTE, value);
  if (err)
    return err;

  value = datetime->second;

  if (is_bcd)
    value = holy_num_to_bcd (value);

  err = holy_cmos_write (holy_CMOS_INDEX_SECOND, value);
  if (err)
    return err;

  return 0;
}
