/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <stdio.h>
#include <string.h>
#include <holy/util/misc.h>
#include <holy/emu/misc.h>
#include <holy/i18n.h>

#define _GNU_SOURCE	1
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#include <argp.h>
#pragma GCC diagnostic error "-Wmissing-prototypes"
#pragma GCC diagnostic error "-Wmissing-declarations"

#include "progname.h"

struct arguments
{
  char *pathname;
};

static struct argp_option options[] = {
  { 0, 0, 0, 0, 0, 0 }
};

static error_t
argp_parser (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case ARGP_KEY_ARG:
      if (state->arg_num == 0)
	arguments->pathname = xstrdup (arg);
      else
	{
	  /* Too many arguments. */
	  fprintf (stderr, _("Unknown extra argument `%s'."), arg);
	  fprintf (stderr, "\n");
	  argp_usage (state);
	}
      break;
    case ARGP_KEY_NO_ARGS:
      fprintf (stderr, "%s", _("No path is specified.\n"));
      argp_usage (state);
      exit (1);
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options, argp_parser, N_("PATH"),
  N_("Transform a system filename into holy one."),
  NULL, NULL, NULL
};

int
main (int argc, char *argv[])
{
  char *relpath;
  struct arguments arguments;

  holy_util_host_init (&argc, &argv);

  memset (&arguments, 0, sizeof (struct arguments));

  /* Check for options.  */
  if (argp_parse (&argp, argc, argv, 0, 0, &arguments) != 0)
    {
      fprintf (stderr, "%s", _("Error in parsing command line arguments\n"));
      exit(1);
    }

  relpath = holy_make_system_path_relative_to_its_root (arguments.pathname);
  printf ("%s\n", relpath);
  free (relpath);

  return 0;
}
