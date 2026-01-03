/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <holy/types.h>
#include <holy/elf.h>
#include <holy/aout.h>
#include <holy/i18n.h>
#include <holy/kernel.h>
#include <holy/disk.h>
#include <holy/emu/misc.h>
#include <holy/util/misc.h>
#include <holy/util/resolve.h>
#include <holy/misc.h>
#include <holy/offsets.h>
#include <holy/crypto.h>
#include <holy/dl.h>
#include <time.h>
#include <multiboot.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <holy/efi/pe32.h>
#include <holy/uboot/image.h>
#include <holy/arm/reloc.h>
#include <holy/ia64/reloc.h>
#include <holy/osdep/hostfile.h>
#include <holy/util/install.h>
#include <holy/emu/config.h>

#define _GNU_SOURCE	1

#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#include <argp.h>
#pragma GCC diagnostic error "-Wmissing-prototypes"
#pragma GCC diagnostic error "-Wmissing-declarations"


#include "progname.h"



static struct argp_option options[] = {
  {"directory",  'd', N_("DIR"), 0,
   /* TRANSLATORS: platform here isn't identifier. It can be translated.  */
   N_("use images and modules under DIR [default=%s/<platform>]"), 0},
  {"prefix",  'p', N_("DIR"), 0, N_("set prefix directory"), 0},
  {"memdisk",  'm', N_("FILE"), 0,
   /* TRANSLATORS: "memdisk" here isn't an identifier, it can be translated.
    "embed" is a verb (command description).  "*/
   N_("embed FILE as a memdisk image\n"
      "Implies `-p (memdisk)/boot/holy' and overrides any prefix supplied previously,"
      " but the prefix itself can be overridden by later options"), 0},
   /* TRANSLATORS: "embed" is a verb (command description).  "*/
  {"config",   'c', N_("FILE"), 0, N_("embed FILE as an early config"), 0},
   /* TRANSLATORS: "embed" is a verb (command description).  "*/
  {"pubkey",   'k', N_("FILE"), 0, N_("embed FILE as public key for signature checking"), 0},
  /* TRANSLATORS: NOTE is a name of segment.  */
  {"note",   'n', 0, 0, N_("add NOTE segment for CHRP IEEE1275"), 0},
  {"output",  'o', N_("FILE"), 0, N_("output a generated image to FILE [default=stdout]"), 0},
  {"format",  'O', N_("FORMAT"), 0, 0, 0},
  {"compression",  'C', "(xz|none|auto)", 0, N_("choose the compression to use for core image"), 0},
  {"verbose",     'v', 0,      0, N_("print verbose messages."), 0},
  { 0, 0, 0, 0, 0, 0 }
};

#pragma GCC diagnostic ignored "-Wformat-nonliteral"

static char *
help_filter (int key, const char *text, void *input __attribute__ ((unused)))
{
  switch (key)
    {
    case 'd':
      return xasprintf (text, holy_util_get_pkglibdir ());
    case 'O':
      {
	char *formats = holy_install_get_image_targets_string (), *ret;
	ret = xasprintf ("%s\n%s %s", _("generate an image in FORMAT"),
			 _("available formats:"), formats);
	free (formats);
	return ret;
      }
    default:
      return (char *) text;
    }
}

#pragma GCC diagnostic error "-Wformat-nonliteral"

struct arguments
{
  size_t nmodules;
  size_t modules_max;
  char **modules;
  char *output;
  char *dir;
  char *prefix;
  char *memdisk;
  char **pubkeys;
  size_t npubkeys;
  char *font;
  char *config;
  int note;
  const struct holy_install_image_target_desc *image_target;
  holy_compression_t comp;
};

static error_t
argp_parser (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case 'o':
      if (arguments->output)
	free (arguments->output);

      arguments->output = xstrdup (arg);
      break;

    case 'O':
      {
	arguments->image_target = holy_install_get_image_target (arg);
	if (!arguments->image_target)
	  {
	    printf (_("unknown target format %s\n"), arg);
	    argp_usage (state);
	    exit (1);
	  }
	break;
      }
    case 'd':
      if (arguments->dir)
	free (arguments->dir);

      arguments->dir = xstrdup (arg);
      break;

    case 'n':
      arguments->note = 1;
      break;

    case 'm':
      if (arguments->memdisk)
	free (arguments->memdisk);

      arguments->memdisk = xstrdup (arg);

      if (arguments->prefix)
	free (arguments->prefix);

      arguments->prefix = xstrdup ("(memdisk)/boot/holy");
      break;

    case 'k':
      arguments->pubkeys = xrealloc (arguments->pubkeys,
				     sizeof (arguments->pubkeys[0])
				     * (arguments->npubkeys + 1));
      arguments->pubkeys[arguments->npubkeys++] = xstrdup (arg);
      break;

    case 'c':
      if (arguments->config)
	free (arguments->config);

      arguments->config = xstrdup (arg);
      break;

    case 'C':
      if (holy_strcmp (arg, "xz") == 0)
	{
#ifdef HAVE_LIBLZMA
	  arguments->comp = holy_COMPRESSION_XZ;
#else
	  holy_util_error ("%s",
			   _("holy-mkimage is compiled without XZ support"));
#endif
	}
      else if (holy_strcmp (arg, "none") == 0)
	arguments->comp = holy_COMPRESSION_NONE;
      else if (holy_strcmp (arg, "auto") == 0)
	arguments->comp = holy_COMPRESSION_AUTO;
      else
	holy_util_error (_("Unknown compression format %s"), arg);
      break;

    case 'p':
      if (arguments->prefix)
	free (arguments->prefix);

      arguments->prefix = xstrdup (arg);
      break;

    case 'v':
      verbosity++;
      break;
    case ARGP_KEY_ARG:
      assert (arguments->nmodules < arguments->modules_max);
      arguments->modules[arguments->nmodules++] = xstrdup(arg);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options, argp_parser, N_("[OPTION]... [MODULES]"),
  N_("Make a bootable image of holy."),
  NULL, help_filter, NULL
};

int
main (int argc, char *argv[])
{
  FILE *fp = stdout;
  struct arguments arguments;
  unsigned i;

  holy_util_host_init (&argc, &argv);

  memset (&arguments, 0, sizeof (struct arguments));
  arguments.comp = holy_COMPRESSION_AUTO;
  arguments.modules_max = argc + 1;
  arguments.modules = xmalloc ((arguments.modules_max + 1)
			     * sizeof (arguments.modules[0]));
  memset (arguments.modules, 0, (arguments.modules_max + 1)
	  * sizeof (arguments.modules[0]));

  if (argp_parse (&argp, argc, argv, 0, 0, &arguments) != 0)
    {
      fprintf (stderr, "%s", _("Error in parsing command line arguments\n"));
      exit(1);
    }

  if (!arguments.image_target)
    {
      char *program = xstrdup(program_name);
      printf ("%s\n", _("Target format not specified (use the -O option)."));
      argp_help (&argp, stderr, ARGP_HELP_STD_USAGE, program);
      free (program);
      exit(1);
    }

  if (!arguments.prefix)
    {
      char *program = xstrdup(program_name);
      printf ("%s\n", _("Prefix not specified (use the -p option)."));
      argp_help (&argp, stderr, ARGP_HELP_STD_USAGE, program);
      free (program);
      exit(1);
    }

  if (arguments.output)
    {
      fp = holy_util_fopen (arguments.output, "wb");
      if (! fp)
	holy_util_error (_("cannot open `%s': %s"), arguments.output,
			 strerror (errno));
    }

  if (!arguments.dir)
    {
      const char *dn = holy_util_get_target_dirname (arguments.image_target);
      const char *pkglibdir = holy_util_get_pkglibdir ();
      char *ptr;
      arguments.dir = xmalloc (holy_strlen (pkglibdir) + holy_strlen (dn) + 2);
      ptr = holy_stpcpy (arguments.dir, pkglibdir);
      *ptr++ = '/';
      strcpy (ptr, dn);
    }

  holy_install_generate_image (arguments.dir, arguments.prefix, fp,
			       arguments.output, arguments.modules,
			       arguments.memdisk, arguments.pubkeys,
			       arguments.npubkeys, arguments.config,
			       arguments.image_target, arguments.note,
			       arguments.comp);

  holy_util_file_sync  (fp);
  fclose (fp);

  for (i = 0; i < arguments.nmodules; i++)
    free (arguments.modules[i]);

  free (arguments.dir);
  free (arguments.prefix);
  free (arguments.modules);

  if (arguments.output)
    free (arguments.output);

  return 0;
}
