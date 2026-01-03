/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <config-util.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>

#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/setjmp.h>
#include <holy/fs.h>
#include <holy/emu/hostdisk.h>
#include <holy/time.h>
#include <holy/emu/console.h>
#include <holy/emu/misc.h>
#include <holy/kernel.h>
#include <holy/normal.h>
#include <holy/emu/getroot.h>
#include <holy/env.h>
#include <holy/partition.h>
#include <holy/i18n.h>
#include <holy/loader.h>
#include <holy/util/misc.h>

#pragma GCC diagnostic ignored "-Wmissing-prototypes"

#include "progname.h"
#include <argp.h>

#define ENABLE_RELOCATABLE 0

/* Used for going back to the main function.  */
static jmp_buf main_env;

/* Store the prefix specified by an argument.  */
static char *root_dev = NULL, *dir = NULL;

holy_addr_t holy_modbase = 0;

void
holy_reboot (void)
{
  longjmp (main_env, 1);
  holy_fatal ("longjmp failed");
}

void
holy_exit (void)
{
  holy_reboot ();
}

void
holy_machine_init (void)
{
}

void
holy_machine_get_bootlocation (char **device, char **path)
{
  *device = root_dev;
  *path = dir;
}

void
holy_machine_fini (int flags)
{
  if (flags & holy_LOADER_FLAG_NORETURN)
    holy_console_fini ();
}



#define OPT_MEMDISK 257

static struct argp_option options[] = {
  {"root",      'r', N_("DEVICE_NAME"), 0, N_("Set root device."), 2},
  {"device-map",  'm', N_("FILE"), 0,
   /* TRANSLATORS: There are many devices in device map.  */
   N_("use FILE as the device map [default=%s]"), 0},
  {"memdisk",  OPT_MEMDISK, N_("FILE"), 0,
   /* TRANSLATORS: There are many devices in device map.  */
   N_("use FILE as memdisk"), 0},
  {"directory",  'd', N_("DIR"), 0,
   N_("use holy files in the directory DIR [default=%s]"), 0},
  {"verbose",     'v', 0,      0, N_("print verbose messages."), 0},
  {"hold",     'H', N_("SECS"),      OPTION_ARG_OPTIONAL, N_("wait until a debugger will attach"), 0},
  { 0, 0, 0, 0, 0, 0 }
};

#pragma GCC diagnostic ignored "-Wformat-nonliteral"

static char *
help_filter (int key, const char *text, void *input __attribute__ ((unused)))
{
  switch (key)
    {
    case 'd':
      return xasprintf (text, DEFAULT_DIRECTORY);
    case 'm':
      return xasprintf (text, DEFAULT_DEVICE_MAP);
    default:
      return (char *) text;
    }
}

#pragma GCC diagnostic error "-Wformat-nonliteral"

struct arguments
{
  const char *dev_map;
  const char *mem_disk;
  int hold;
};

static error_t
argp_parser (int key, char *arg, struct argp_state *state)
{
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key)
    {
    case OPT_MEMDISK:
      arguments->mem_disk = arg;
      break;
    case 'r':
      free (root_dev);
      root_dev = xstrdup (arg);
      break;
    case 'd':
      free (dir);
      dir = xstrdup (arg);
      break;
    case 'm':
      arguments->dev_map = arg;
      break;
    case 'H':
      arguments->hold = (arg ? atoi (arg) : -1);
      break;
    case 'v':
      verbosity++;
      break;

    case ARGP_KEY_ARG:
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
  options, argp_parser, NULL,
  N_("holy emulator."),
  NULL, help_filter, NULL
};



#pragma GCC diagnostic ignored "-Wmissing-prototypes"

int
main (int argc, char *argv[])
{
  struct arguments arguments =
    { 
      .dev_map = DEFAULT_DEVICE_MAP,
      .hold = 0,
      .mem_disk = 0,
    };
  volatile int hold = 0;
  size_t total_module_size = sizeof (struct holy_module_info), memdisk_size = 0;
  struct holy_module_info *modinfo;
  void *mods;

  holy_util_host_init (&argc, &argv);

  dir = xstrdup (DEFAULT_DIRECTORY);

  if (argp_parse (&argp, argc, argv, 0, 0, &arguments) != 0)
    {
      fprintf (stderr, "%s", _("Error in parsing command line arguments\n"));
      exit(1);
    }

  if (arguments.mem_disk)
    {
      memdisk_size = ALIGN_UP(holy_util_get_image_size (arguments.mem_disk), 512);
      total_module_size += memdisk_size + sizeof (struct holy_module_header);
    }

  mods = xmalloc (total_module_size);
  modinfo = holy_memset (mods, 0, total_module_size);
  mods = (char *) (modinfo + 1);

  modinfo->magic = holy_MODULE_MAGIC;
  modinfo->offset = sizeof (struct holy_module_info);
  modinfo->size = total_module_size;

  if (arguments.mem_disk)
    {
      struct holy_module_header *header = (struct holy_module_header *) mods;
      header->type = OBJ_TYPE_MEMDISK;
      header->size = memdisk_size + sizeof (*header);
      mods = header + 1;

      holy_util_load_image (arguments.mem_disk, mods);
      mods = (char *) mods + memdisk_size;
    }

  holy_modbase = (holy_addr_t) modinfo;

  hold = arguments.hold;
  /* Wait until the ARGS.HOLD variable is cleared by an attached debugger. */
  if (hold && verbosity > 0)
    /* TRANSLATORS: In this case holy tells user what he has to do.  */
    printf (_("Run `gdb %s %d', and set ARGS.HOLD to zero.\n"),
            program_name, (int) getpid ());
  while (hold)
    {
      if (hold > 0)
        hold--;

      sleep (1);
    }

  signal (SIGINT, SIG_IGN);
  holy_console_init ();
  holy_host_init ();

  /* XXX: This is a bit unportable.  */
  holy_util_biosdisk_init (arguments.dev_map);

  holy_init_all ();

  holy_hostfs_init ();

  /* Make sure that there is a root device.  */
  if (! root_dev)
    root_dev = holy_strdup ("host");

  dir = xstrdup (dir);

  /* Start holy!  */
  if (setjmp (main_env) == 0)
    holy_main ();

  holy_fini_all ();
  holy_hostfs_fini ();
  holy_host_fini ();

  holy_machine_fini (holy_LOADER_FLAG_NORETURN);

  return 0;
}
