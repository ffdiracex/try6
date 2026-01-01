/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <holy/types.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/emu/misc.h>
#include <holy/util/misc.h>
#include <holy/i18n.h>
#include <holy/parser.h>
#include <holy/script_sh.h>

#define _GNU_SOURCE	1

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#include <argp.h>
#pragma GCC diagnostic error "-Wmissing-prototypes"
#pragma GCC diagnostic error "-Wmissing-declarations"

#include "progname.h"

struct arguments
{
  int verbose;
  char *filename;
};

static struct argp_option options[] = {
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
    case 'v':
      arguments->verbose = 1;
      break;

    case ARGP_KEY_ARG:
      if (state->arg_num == 0)
	arguments->filename = xstrdup (arg);
      else
	{
	  /* Too many arguments. */
	  fprintf (stderr, _("Unknown extra argument `%s'."), arg);
	  fprintf (stderr, "\n");
	  argp_usage (state);
	}
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options, argp_parser, N_("[PATH]"),
  N_("Checks holy script configuration file for syntax errors."),
  NULL, NULL, NULL
};

/* Context for main.  */
struct main_ctx
{
  int lineno;
  FILE *file;
  struct arguments arguments;
};

/* Helper for main.  */
static holy_err_t
get_config_line (char **line, int cont __attribute__ ((unused)), void *data)
{
  struct main_ctx *ctx = data;
  int i;
  char *cmdline = 0;
  size_t len = 0;
  ssize_t curread;

  curread = getline (&cmdline, &len, (ctx->file ?: stdin));
  if (curread == -1)
    {
      *line = 0;
      holy_errno = holy_ERR_READ_ERROR;

      if (cmdline)
	free (cmdline);
      return holy_errno;
    }

  if (ctx->arguments.verbose)
    holy_printf ("%s", cmdline);

  for (i = 0; cmdline[i] != '\0'; i++)
    {
      /* Replace tabs and carriage returns with spaces.  */
      if (cmdline[i] == '\t' || cmdline[i] == '\r')
	cmdline[i] = ' ';

      /* Replace '\n' with '\0'.  */
      if (cmdline[i] == '\n')
	cmdline[i] = '\0';
    }

  ctx->lineno++;
  *line = holy_strdup (cmdline);

  free (cmdline);
  return 0;
}

int
main (int argc, char *argv[])
{
  struct main_ctx ctx = {
    .lineno = 0,
    .file = 0
  };
  char *input;
  int found_input = 0, found_cmd = 0;
  struct holy_script *script = NULL;

  holy_util_host_init (&argc, &argv);

  memset (&ctx.arguments, 0, sizeof (struct arguments));

  /* Check for options.  */
  if (argp_parse (&argp, argc, argv, 0, 0, &ctx.arguments) != 0)
    {
      fprintf (stderr, "%s", _("Error in parsing command line arguments\n"));
      exit(1);
    }

  /* Obtain ARGUMENT.  */
  if (!ctx.arguments.filename)
    {
      ctx.file = 0; /* read from stdin */
    }
  else
    {
      ctx.file = holy_util_fopen (ctx.arguments.filename, "r");
      if (! ctx.file)
	{
          char *program = xstrdup(program_name);
	  fprintf (stderr, _("cannot open `%s': %s"),
		   ctx.arguments.filename, strerror (errno));
          argp_help (&argp, stderr, ARGP_HELP_STD_USAGE, program);
          free(program);
          exit(1);
	}
    }

  do
    {
      input = 0;
      get_config_line (&input, 0, &ctx);
      if (! input) 
	break;
      found_input = 1;

      script = holy_script_parse (input, get_config_line, &ctx);
      if (script)
	{
	  if (script->cmd)
	    found_cmd = 1;
	  holy_script_execute (script);
	  holy_script_free (script);
	}

      holy_free (input);
    } while (script != 0);

  if (ctx.file) fclose (ctx.file);

  if (found_input && script == 0)
    {
      fprintf (stderr, _("Syntax error at line %u\n"), ctx.lineno);
      return 1;
    }
  if (! found_cmd)
    {
      fprintf (stderr, _("Script `%s' contains no commands and will do nothing\n"),
	       ctx.arguments.filename);
      return 1;
    }

  return 0;
}
