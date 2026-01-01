/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/mm.h>
#include <holy/env.h>
#include <holy/parser.h>
#include <holy/misc.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_err_t
holy_rescue_parse_line (char *line,
			holy_reader_getline_t getline, void *getline_data)
{
  char *name;
  int n;
  holy_command_t cmd;
  char **args;

  if (holy_parser_split_cmdline (line, getline, getline_data, &n, &args)
      || n < 0)
    return holy_errno;

  if (n == 0)
    return holy_ERR_NONE;

  /* In case of an assignment set the environment accordingly
     instead of calling a function.  */
  if (n == 1)
    {
      char *val = holy_strchr (args[0], '=');

      if (val)
	{
	  val[0] = 0;
	  holy_env_set (args[0], val + 1);
	  val[0] = '=';
	  goto quit;
	}
    }

  /* Get the command name.  */
  name = args[0];

  /* If nothing is specified, restart.  */
  if (*name == '\0')
    goto quit;

  cmd = holy_command_find (name);
  if (cmd)
    {
      (cmd->func) (cmd, n - 1, &args[1]);
    }
  else
    {
      holy_printf_ (N_("Unknown command `%s'.\n"), name);
      if (holy_command_find ("help"))
	holy_printf ("Try `help' for usage\n");
    }

 quit:
  /* Arguments are returned in single memory chunk separated by zeroes */
  holy_free (args[0]);
  holy_free (args);

  return holy_errno;
}
