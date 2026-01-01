/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/datetime.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/arc/arc.h>

holy_MOD_LICENSE ("GPLv2+");

holy_err_t
holy_get_datetime (struct holy_datetime *datetime)
{
  struct holy_arc_timeinfo *dt;
  holy_memset (datetime, 0, sizeof (*datetime));

  dt = holy_ARC_FIRMWARE_VECTOR->gettime ();

  datetime->year = dt->y;
  datetime->month = dt->m;
  datetime->day = dt->d;
  datetime->hour = dt->h;
  datetime->minute = dt->min;
  datetime->second = dt->s;

  return 0;
}

holy_err_t
holy_set_datetime (struct holy_datetime *datetime __attribute__ ((unused)))
{
  return holy_error (holy_ERR_IO, "setting time isn't supported");
}
