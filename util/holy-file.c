/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/util/misc.h>
#include <holy/i18n.h>
#include <holy/term.h>
#include <holy/font.h>
#include <holy/gfxmenu_view.h>
#include <holy/color.h>
#include <holy/util/install.h>
#include <holy/command.h>
#include <holy/env.h>

#define _GNU_SOURCE	1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "progname.h"

void holy_file_init (void);
void holy_host_init (void);
void holy_hostfs_init (void);

int
main (int argc, char *argv[])
{
  char **argv2;
  int i;
  int had_file = 0, had_separator = 0;
  holy_command_t cmd;
  holy_err_t err;

  holy_util_host_init (&argc, &argv);

  argv2 = xmalloc (argc * sizeof (argv2[0]));

  if (argc == 2 && strcmp (argv[1], "--version") == 0)
    {
      printf ("%s (%s) %s\n", program_name, PACKAGE_NAME, PACKAGE_VERSION);
    }

  for (i = 1; i < argc; i++)
    {
      if (argv[i][0] == '-' && argv[i][1] == '-'
	  && argv[i][2] == '\0' && !had_separator)
	{
	  had_separator = 1;
	  argv2[i - 1] = xstrdup (argv[i]);
	  continue;
	}
      if (argv[i][0] == '-' && !had_separator)
	{
	  argv2[i - 1] = xstrdup (argv[i]);
	  continue;
	}
      if (had_file)
	holy_util_error ("one argument expected");
      argv2[i - 1] = holy_canonicalize_file_name (argv[i]);
      if (!argv2[i - 1])
	{
	  holy_util_error (_("cannot open `%s': %s"), argv[i],
			   strerror (errno));
	}
      had_file = 1;
    }
  argv2[i - 1] = NULL;

  /* Initialize all modules. */
  holy_init_all ();
  holy_file_init ();
  holy_hostfs_init ();
  holy_host_init ();

  holy_env_set ("root", "host");

  cmd = holy_command_find ("file");
  if (! cmd)
    holy_util_error (_("can't find command `%s'"), "file");

  err = (cmd->func) (cmd, argc - 1, argv2);
  if (err && err != holy_ERR_TEST_FAILURE)
    holy_print_error ();
  return err;
}
