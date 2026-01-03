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
#include <holy/emu/hostdisk.h>

#define _GNU_SOURCE	1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#include <argp.h>
#pragma GCC diagnostic error "-Wmissing-prototypes"
#pragma GCC diagnostic error "-Wmissing-declarations"

#include "progname.h"

struct arguments
{
  char *input;
  char *text;
  char *output;
  char *font;
  char *fgcolor;
  char *bgcolor;
  int verbosity;
};

static struct argp_option options[] = {
  {"input",  'i', N_("FILE"), 0,
   N_("read text from FILE."), 0},
  {"color",  'c', N_("COLOR"), 0,
   N_("use COLOR for text"), 0},
  {"bgcolor",  'b', N_("COLOR"), 0,
   N_("use COLOR for background"), 0},
  {"text",  't', N_("STRING"), 0,
  /* TRANSLATORS: The result is always stored to file and
     never shown directly, so don't use "show" as synonym for render. Use "create" if
     "render" doesn't translate directly.  */
   N_("set the label to render"), 0},
  {"output",  'o', N_("FILE"), 0,
   N_("set output filename. Default is STDOUT"), 0},
  {"font",  'f', N_("FILE"), 0,
   N_("use FILE as font (PF2)."), 0},
  {"verbose",     'v', 0,      0, N_("print verbose messages."), 0},
  { 0, 0, 0, 0, 0, 0 }
};

#include <holy/err.h>
#include <holy/types.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/video.h>
#include <holy/video_fb.h>

static error_t
argp_parser (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'i':
      arguments->input = xstrdup (arg);
      break;

    case 'b':
      arguments->bgcolor = xstrdup (arg);
      break;

    case 'c':
      arguments->fgcolor = xstrdup (arg);
      break;

    case 'f':
      arguments->font = xstrdup (arg);
      break;

    case 't':
      arguments->text = xstrdup (arg);
      break;

    case 'o':
      arguments->output = xstrdup (arg);
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
  options, argp_parser, N_("[OPTIONS]"),
  /* TRANSLATORS: This file takes a text and creates a graphical representation of it,
     putting the result into .disk_label file. The result is always stored to file and
     never shown directly, so don't use "show" as synonym for render. Use "create" if
     "render" doesn't translate directly.  */
  N_("Render Apple .disk_label."),
  NULL, NULL, NULL
};

int
main (int argc, char *argv[])
{
  char *text;
  struct arguments arguments;

  holy_util_host_init (&argc, &argv);

  /* Check for options.  */
  memset (&arguments, 0, sizeof (struct arguments));
  if (argp_parse (&argp, argc, argv, 0, 0, &arguments) != 0)
    {
      fprintf (stderr, "%s", _("Error in parsing command line arguments\n"));
      exit(1);
    }

  if ((!arguments.input && !arguments.text) || !arguments.font)
    {
      fprintf (stderr, "%s", _("Missing arguments\n"));
      exit(1);
    }

  if (arguments.text)
    text = arguments.text;
  else
    {
      FILE *in = holy_util_fopen (arguments.input, "r");
      size_t s;
      if (!in)
	holy_util_error (_("cannot open `%s': %s"), arguments.input,
			 strerror (errno));
      fseek (in, 0, SEEK_END);
      s = ftell (in);
      fseek (in, 0, SEEK_SET);
      text = xmalloc (s + 1);
      if (fread (text, 1, s, in) != s)
	holy_util_error (_("cannot read `%s': %s"), arguments.input,
			 strerror (errno));
      text[s] = 0;
      fclose (in);
    }

  holy_init_all ();
  holy_hostfs_init ();
  holy_host_init ();

  holy_util_render_label (arguments.font,
			  arguments.bgcolor,
			  arguments.fgcolor,
			  text,
			  arguments.output);

  return 0;
}
