/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/env.h>
#include <holy/term.h>
#include <holy/types.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static char *
holy_getline (void)
{
  int i;
  char *line;
  char *tmp;
  char c;

  i = 0;
  line = holy_malloc (1 + i + sizeof('\0'));
  if (! line)
    return NULL;

  while (1)
    {
      c = holy_getkey ();
      if ((c == '\n') || (c == '\r'))
	break;

      line[i] = c;
      if (holy_isprint (c))
	holy_printf ("%c", c);
      i++;
      tmp = holy_realloc (line, 1 + i + sizeof('\0'));
      if (! tmp)
	{
	  holy_free (line);
	  return NULL;
	}
      line = tmp;
    }
  line[i] = '\0';

  return line;
}

static holy_err_t
holy_cmd_read (holy_command_t cmd __attribute__ ((unused)), int argc, char **args)
{
  char *line = holy_getline ();
  if (! line)
    return holy_errno;
  if (argc > 0)
    holy_env_set (args[0], line);

  holy_free (line);
  return 0;
}

static holy_command_t cmd;

holy_MOD_INIT(read)
{
  cmd = holy_register_command ("read", holy_cmd_read,
			       N_("[ENVVAR]"),
			       N_("Set variable with user input."));
}

holy_MOD_FINI(read)
{
  holy_unregister_command (cmd);
}
