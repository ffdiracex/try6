/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/util/misc.h>
#include <holy/i18n.h>
#include <holy/term.h>
#include <holy/font.h>
#include <holy/emu/hostdisk.h>

#define _GNU_SOURCE	1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <holy/err.h>
#include <holy/types.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/syslinux_parse.h>

#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#include <argp.h>
#pragma GCC diagnostic error "-Wmissing-prototypes"
#pragma GCC diagnostic error "-Wmissing-declarations"

#include "progname.h"

struct arguments
{
  char *input;
  char *root;
  char *target_root;
  char *cwd;
  char *target_cwd;
  char *output;
  int verbosity;
  holy_syslinux_flavour_t flav;
};

static struct argp_option options[] = {
  {"target-root",  't', N_("DIR"), 0,
   N_("root directory as it will be seen on runtime [default=/]."), 0},
  {"root",  'r', N_("DIR"), 0,
   N_("root directory of the syslinux disk [default=/]."), 0},
  {"target-cwd",  'T', N_("DIR"), 0,
   N_(
      "current directory of syslinux as it will be seen on runtime  [default is parent directory of input file]."
), 0},
  {"cwd",  'c', N_("DIR"), 0,
   N_("current directory of syslinux [default is parent directory of input file]."), 0},

  {"output",  'o', N_("FILE"), 0, N_("write output to FILE [default=stdout]."), 0},
  {"isolinux",     'i', 0,      0, N_("assume input is an isolinux configuration file."), 0},
  {"pxelinux",     'p', 0,      0, N_("assume input is a pxelinux configuration file."), 0},
  {"syslinux",     's', 0,      0, N_("assume input is a syslinux configuration file."), 0},
  {"verbose",     'v', 0,      0, N_("print verbose messages."), 0},
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
    case 't':
      free (arguments->target_root);
      arguments->target_root = xstrdup (arg);
      break;

    case 'T':
      free (arguments->target_cwd);
      arguments->target_cwd = xstrdup (arg);
      break;

    case 'c':
      free (arguments->cwd);
      arguments->cwd = xstrdup (arg);
      break;

    case 'o':
      free (arguments->output);
      arguments->output = xstrdup (arg);
      break;

    case ARGP_KEY_ARG:
      if (!arguments->input)
	{
	  arguments->input = xstrdup (arg);
	  return 0;
	}
      return ARGP_ERR_UNKNOWN;

    case 'r':
      free (arguments->root);
      arguments->root = xstrdup (arg);
      return 0;

    case 'i':
      arguments->flav = holy_SYSLINUX_ISOLINUX;
      break;

    case 's':
      arguments->flav = holy_SYSLINUX_SYSLINUX;
      break;
    case 'p':
      arguments->flav = holy_SYSLINUX_PXELINUX;
      break;

    case 'v':
      arguments->verbosity++;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }

  return 0;
}

static struct argp argp = {
  options, argp_parser, N_("FILE"),
  N_("Transform syslinux config into holy one."),
  NULL, NULL, NULL
};

int
main (int argc, char *argv[])
{
  struct arguments arguments;

  holy_util_host_init (&argc, &argv);

  /* Check for options.  */
  memset (&arguments, 0, sizeof (struct arguments));
  if (argp_parse (&argp, argc, argv, 0, 0, &arguments) != 0)
    {
      fprintf (stderr, "%s", _("Error in parsing command line arguments\n"));
      exit(1);
    }

  if (!arguments.input)
    {
      fprintf (stderr, "%s", _("Missing arguments\n"));
      exit(1);
    }

  holy_init_all ();
  holy_hostfs_init ();
  holy_host_init ();

  char *t, *inpfull, *rootfull, *res;
  t = holy_canonicalize_file_name (arguments.input);
  if (!t)
    {
      holy_util_error (_("cannot open `%s': %s"), arguments.input,
		       strerror (errno));
    }  

  inpfull = xasprintf ("(host)/%s", t);
  free (t);

  t = holy_canonicalize_file_name (arguments.root ? : "/");
  if (!t)
    {
      holy_util_error (_("cannot open `%s': %s"), arguments.root,
		       strerror (errno));
    }  

  rootfull = xasprintf ("(host)/%s", t);
  free (t);

  char *cwd = xstrdup (arguments.input);
  char *p = strrchr (cwd, '/');
  char *cwdfull;
  if (p)
    *p = '\0';
  else
    {
      free (cwd);
      cwd = xstrdup (".");
    }

  t = holy_canonicalize_file_name (arguments.cwd ? : cwd);
  if (!t)
    {
      holy_util_error (_("cannot open `%s': %s"), arguments.root,
		       strerror (errno));
    }  

  cwdfull = xasprintf ("(host)/%s", t);
  free (t);

  res = holy_syslinux_config_file (rootfull, arguments.target_root ? : "/",
				   cwdfull, arguments.target_cwd ? : cwd,
				   inpfull, arguments.flav);
  if (!res)
    holy_util_error ("%s", holy_errmsg);
  if (arguments.output)
    {
      FILE *f = holy_util_fopen (arguments.output, "wb");
      if (!f)
	holy_util_error (_("cannot open `%s': %s"), arguments.output,
			 strerror (errno));
      fwrite (res, 1, strlen (res), f); 
      fclose (f);
    }
  else
    printf ("%s\n", res);
  free (res);
  free (rootfull);
  free (inpfull);
  free (arguments.root);
  free (arguments.output);
  free (arguments.target_root);
  free (arguments.input);
  free (cwdfull);
  free (cwd);

  return 0;
}
