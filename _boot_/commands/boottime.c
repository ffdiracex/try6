/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");


static holy_err_t
holy_cmd_boottime (struct holy_command *cmd __attribute__ ((unused)),
		   int argc __attribute__ ((unused)),
		   char *argv[] __attribute__ ((unused)))
{
  struct holy_boot_time *cur;
  holy_uint64_t last_time = 0, start_time = 0;
  if (!holy_boot_time_head)
    {
      holy_puts_ (N_("No boot time statistics is available\n"));
      return 0;
    }
  start_time = last_time = holy_boot_time_head->tp;
  for (cur = holy_boot_time_head; cur; cur = cur->next)
    {
      holy_uint32_t tmabs = cur->tp - start_time;
      holy_uint32_t tmrel = cur->tp - last_time;
      last_time = cur->tp;

      holy_printf ("%3d.%03ds %2d.%03ds %s:%d %s\n",
		   tmabs / 1000, tmabs % 1000, tmrel / 1000, tmrel % 1000, cur->file, cur->line,
		   cur->msg);
    }
 return 0;
}

static holy_command_t cmd_boottime;

holy_MOD_INIT(boottime)
{
  cmd_boottime =
    holy_register_command ("boottime", holy_cmd_boottime,
			   0, N_("Show boot time statistics."));
}

holy_MOD_FINI(boottime)
{
  holy_unregister_command (cmd_boottime);
}
