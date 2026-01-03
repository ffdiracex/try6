/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/dl.h>
#include <holy/env.h>
#include <holy/misc.h>
#include <holy/normal.h>
#include <holy/datetime.h>

holy_MOD_LICENSE ("GPLv2+");

static const char *holy_datetime_names[] =
{
  "YEAR",
  "MONTH",
  "DAY",
  "HOUR",
  "MINUTE",
  "SECOND",
  "WEEKDAY",
};

static const char *
holy_read_hook_datetime (struct holy_env_var *var,
                         const char *val __attribute__ ((unused)))
{
  struct holy_datetime datetime;
  static char buf[6];

  buf[0] = 0;
  if (! holy_get_datetime (&datetime))
    {
      int i;

      for (i = 0; i < 7; i++)
        if (holy_strcmp (var->name, holy_datetime_names[i]) == 0)
          {
            int n;

            switch (i)
              {
              case 0:
                n = datetime.year;
                break;
              case 1:
                n = datetime.month;
                break;
              case 2:
                n = datetime.day;
                break;
              case 3:
                n = datetime.hour;
                break;
              case 4:
                n = datetime.minute;
                break;
              case 5:
                n = datetime.second;
                break;
              default:
                return holy_get_weekday_name (&datetime);
              }

            holy_snprintf (buf, sizeof (buf), "%d", n);
            break;
          }
    }

  return buf;
}

holy_MOD_INIT(datehook)
{
  unsigned i;

  for (i = 0; i < ARRAY_SIZE (holy_datetime_names); i++)
    {
      holy_register_variable_hook (holy_datetime_names[i],
				   holy_read_hook_datetime, 0);
      holy_env_export (holy_datetime_names[i]);
    }
}

holy_MOD_FINI(datehook)
{
  unsigned i;

  for (i = 0; i < ARRAY_SIZE (holy_datetime_names); i++)
    {
      holy_register_variable_hook (holy_datetime_names[i], 0, 0);
      holy_env_unset (holy_datetime_names[i]);
    }
}
