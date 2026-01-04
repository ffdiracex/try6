/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/err.h>
#include <holy/misc.h>
#include <holy/datetime.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

#define holy_DATETIME_SET_YEAR		1
#define holy_DATETIME_SET_MONTH		2
#define holy_DATETIME_SET_DAY		4
#define holy_DATETIME_SET_HOUR		8
#define holy_DATETIME_SET_MINUTE	16
#define holy_DATETIME_SET_SECOND	32

static holy_err_t
holy_cmd_date (holy_command_t cmd __attribute__ ((unused)),
               int argc, char **args)
{
  struct holy_datetime datetime;
  int limit[6][2] = {{1980, 2079}, {1, 12}, {1, 31}, {0, 23}, {0, 59}, {0, 59}};
  int value[6], mask;

  if (argc == 0)
    {
      if (holy_get_datetime (&datetime))
        return holy_errno;

      holy_printf ("%d-%02d-%02d %02d:%02d:%02d %s\n",
                   datetime.year, datetime.month, datetime.day,
                   datetime.hour, datetime.minute, datetime.second,
                   holy_get_weekday_name (&datetime));

      return 0;
    }

  holy_memset (&value, 0, sizeof (value));
  mask = 0;

  for (; argc; argc--, args++)
    {
      char *p, c;
      int m1, ofs, n, cur_mask;

      p = args[0];
      m1 = holy_strtoul (p, &p, 10);

      c = *p;
      if (c == '-')
        ofs = 0;
      else if (c == ':')
        ofs = 3;
      else
        goto fail;

      value[ofs] = m1;
      cur_mask = (1 << ofs);
      mask &= ~(cur_mask * (1 + 2 + 4));

      for (n = 1; (n < 3) && (*p); n++)
        {
          if (*p != c)
            goto fail;

          value[ofs + n] = holy_strtoul (p + 1, &p, 10);
          cur_mask |= (1 << (ofs + n));
        }

      if (*p)
        goto fail;

      if ((ofs == 0) && (n == 2))
        {
          value[ofs + 2] = value[ofs + 1];
          value[ofs + 1] = value[ofs];
          ofs++;
          cur_mask <<= 1;
        }

      for (; n; n--, ofs++)
        if ((value [ofs] < limit[ofs][0]) ||
            (value [ofs] > limit[ofs][1]))
          goto fail;

      mask |= cur_mask;
    }

  if (holy_get_datetime (&datetime))
    return holy_errno;

  if (mask & holy_DATETIME_SET_YEAR)
    datetime.year = value[0];

  if (mask & holy_DATETIME_SET_MONTH)
    datetime.month = value[1];

  if (mask & holy_DATETIME_SET_DAY)
    datetime.day = value[2];

  if (mask & holy_DATETIME_SET_HOUR)
    datetime.hour = value[3];

  if (mask & holy_DATETIME_SET_MINUTE)
    datetime.minute = value[4];

  if (mask & holy_DATETIME_SET_SECOND)
    datetime.second = value[5];

  return holy_set_datetime (&datetime);

fail:
  return holy_error (holy_ERR_BAD_ARGUMENT, "invalid datetime");
}

static holy_command_t cmd;

holy_MOD_INIT(date)
{
  cmd =
    holy_register_command ("date", holy_cmd_date,
			   N_("[[year-]month-day] [hour:minute[:second]]"),
			   N_("Display/set current datetime."));
}

holy_MOD_FINI(date)
{
  holy_unregister_command (cmd);
}
