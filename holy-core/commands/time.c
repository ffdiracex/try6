/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/time.h>
#include <holy/misc.h>
#include <holy/dl.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");


static holy_err_t
holy_cmd_time (holy_command_t ctxt __attribute__ ((unused)),
	       int argc, char **args)
{
  holy_command_t cmd;
  holy_uint32_t start;
  holy_uint32_t end;

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("no command is specified"));

  cmd = holy_command_find (args[0]);

  if (!cmd)
    return holy_error (holy_ERR_UNKNOWN_COMMAND, N_("can't find command `%s'"),
		       args[0]);

  start = holy_get_time_ms ();
  (cmd->func) (cmd, argc - 1, &args[1]);
  end = holy_get_time_ms ();

  holy_printf_ (N_("Elapsed time: %d.%03d seconds \n"), (end - start) / 1000,
		(end - start) % 1000);

  return holy_errno;
}

static holy_command_t cmd;

holy_MOD_INIT(time)
{
  cmd = holy_register_command ("time", holy_cmd_time,
			      N_("COMMAND [ARGS]"),
			       N_("Measure time used by COMMAND"));
}

holy_MOD_FINI(time)
{
  holy_unregister_command (cmd);
}
