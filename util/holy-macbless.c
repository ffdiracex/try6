/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <holy/types.h>
#include <holy/emu/misc.h>
#include <holy/util/misc.h>
#include <holy/device.h>
#include <holy/disk.h>
#include <holy/file.h>
#include <holy/fs.h>
#include <holy/partition.h>
#include <holy/msdos_partition.h>
#include <holy/emu/hostdisk.h>
#include <holy/emu/getroot.h>
#include <holy/term.h>
#include <holy/env.h>
#include <holy/diskfilter.h>
#include <holy/i18n.h>
#include <holy/crypto.h>
#include <holy/cryptodisk.h>
#include <holy/hfsplus.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>

#define _GNU_SOURCE	1
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#include <argp.h>
#pragma GCC diagnostic error "-Wmissing-prototypes"
#pragma GCC diagnostic error "-Wmissing-declarations"

#include "progname.h"

static void
bless (const char *path, int x86)
{
  char *drive_name = NULL;
  char **devices;
  char *holy_path = NULL;
  char *filebuf_via_holy = NULL, *filebuf_via_sys = NULL;
  holy_device_t dev = NULL;
  holy_err_t err;
  struct stat st;

  holy_path = holy_canonicalize_file_name (path);

  if (stat (holy_path, &st) < 0)
    holy_util_error (N_("cannot stat `%s': %s"),
		     holy_path, strerror (errno));

  devices = holy_guess_root_devices (holy_path);

  if (! devices || !devices[0])
    holy_util_error (_("cannot find a device for %s (is /dev mounted?)"), path);

  drive_name = holy_util_get_holy_dev (devices[0]);
  if (! drive_name)
    holy_util_error (_("cannot find a holy drive for %s.  Check your device.map"),
		     devices[0]);

  holy_util_info ("opening %s", drive_name);
  dev = holy_device_open (drive_name);
  if (! dev)
    holy_util_error ("%s", holy_errmsg);

  err = holy_mac_bless_inode (dev, st.st_ino, S_ISDIR (st.st_mode), x86);
  if (err)
    holy_util_error ("%s", holy_errmsg);
  free (holy_path);
  free (filebuf_via_holy);
  free (filebuf_via_sys);
  free (drive_name);
  free (devices);
  holy_device_close (dev);
}

static struct argp_option options[] = {
  {"x86",  'x', 0, 0,
   N_("bless for x86-based macs"), 0},
  {"ppc",  'p', 0, 0,
   N_("bless for ppc-based macs"), 0},
  {"verbose",     'v', 0,      0, N_("print verbose messages."), 0},
  { 0, 0, 0, 0, 0, 0 }
};

struct arguments
{
  char *arg;
  int ppc;
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
      verbosity++;
      break;

    case 'x':
      arguments->ppc = 0;
      break;

    case 'p':
      arguments->ppc = 1;
      break;

    case ARGP_KEY_NO_ARGS:
      fprintf (stderr, "%s", _("No path or device is specified.\n"));
      argp_usage (state);
      break;

    case ARGP_KEY_ARG:
      if (arguments->arg)
	{
	  fprintf (stderr, _("Unknown extra argument `%s'."), arg);
	  fprintf (stderr, "\n");
	  return ARGP_ERR_UNKNOWN;
	}
      arguments->arg = xstrdup (arg);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {
  options, argp_parser, N_("--ppc PATH|--x86 FILE"),
  N_("Mac-style bless on HFS or HFS+"),
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

  if (verbosity > 1)
    holy_env_set ("debug", "all");

  /* Initialize the emulated biosdisk driver.  */
  holy_util_biosdisk_init (NULL);

  /* Initialize all modules. */
  holy_init_all ();
  holy_gcry_init_all ();

  holy_lvm_fini ();
  holy_mdraid09_fini ();
  holy_mdraid1x_fini ();
  holy_diskfilter_fini ();
  holy_diskfilter_init ();
  holy_mdraid09_init ();
  holy_mdraid1x_init ();
  holy_lvm_init ();

  /* Do it.  */
  bless (arguments.arg, !arguments.ppc);

  /* Free resources.  */
  holy_gcry_fini_all ();
  holy_fini_all ();
  holy_util_biosdisk_fini ();

  return 0;
}
