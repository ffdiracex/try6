/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/util/install.h>
#include <holy/emu/config.h>
#include <holy/util/misc.h>

#include <string.h>
#include <errno.h>

#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#include <argp.h>
#pragma GCC diagnostic error "-Wmissing-prototypes"
#pragma GCC diagnostic error "-Wmissing-declarations"

static char *rootdir = NULL, *subdir = NULL;
static char *debug_image = NULL;

enum
  {
    OPTION_NET_DIRECTORY = 0x301,
    OPTION_SUBDIR,
    OPTION_DEBUG,
    OPTION_DEBUG_IMAGE
  };

static struct argp_option options[] = {
  holy_INSTALL_OPTIONS,
  {"net-directory", OPTION_NET_DIRECTORY, N_("DIR"),
   0, N_("root directory of TFTP server"), 2},
  {"subdir", OPTION_SUBDIR, N_("DIR"),
   0, N_("relative subdirectory on network server"), 2},
  {"debug", OPTION_DEBUG, 0, OPTION_HIDDEN, 0, 2},
  {"debug-image", OPTION_DEBUG_IMAGE, N_("STRING"), OPTION_HIDDEN, 0, 2},
  {0, 0, 0, 0, 0, 0}
};

static error_t 
argp_parser (int key, char *arg, struct argp_state *state)
{
  if (holy_install_parse (key, arg))
    return 0;
  switch (key)
    {
    case OPTION_NET_DIRECTORY:
      free (rootdir);
      rootdir = xstrdup (arg);
      return 0;
    case OPTION_SUBDIR:
      free (subdir);
      subdir = xstrdup (arg);
      return 0;
      /* This is an undocumented feature...  */
    case OPTION_DEBUG:
      verbosity++;
      return 0;
    case OPTION_DEBUG_IMAGE:
      free (debug_image);
      debug_image = xstrdup (arg);
      return 0;

    case ARGP_KEY_ARG:
    default:
      return ARGP_ERR_UNKNOWN;
    }
}


struct argp argp = {
  options, argp_parser, NULL,
  "\v"N_("Prepares holy network boot images at net_directory/subdir "
	 "assuming net_directory being TFTP root."), 
  NULL, holy_install_help_filter, NULL
};

static char *base;

static const struct
{
  const char *mkimage_target;
  const char *netmodule;
  const char *ext;
} targets[holy_INSTALL_PLATFORM_MAX] =
  {
    [holy_INSTALL_PLATFORM_I386_PC] = { "i386-pc-pxe", "pxe", ".0" },
    [holy_INSTALL_PLATFORM_SPARC64_IEEE1275] = { "sparc64-ieee1275-aout", "ofnet", ".img" },
    [holy_INSTALL_PLATFORM_I386_IEEE1275] = { "i386-ieee1275", "ofnet", ".elf" },
    [holy_INSTALL_PLATFORM_POWERPC_IEEE1275] = { "powerpc-ieee1275", "ofnet", ".elf" },
    [holy_INSTALL_PLATFORM_I386_EFI] = { "i386-efi", "efinet", ".efi" },
    [holy_INSTALL_PLATFORM_X86_64_EFI] = { "x86_64-efi", "efinet", ".efi" },
    [holy_INSTALL_PLATFORM_IA64_EFI] = { "ia64-efi", "efinet", ".efi" },
    [holy_INSTALL_PLATFORM_ARM_EFI] = { "arm-efi", "efinet", ".efi" },
    [holy_INSTALL_PLATFORM_ARM64_EFI] = { "arm64-efi", "efinet", ".efi" }
  };

static void
process_input_dir (const char *input_dir, enum holy_install_plat platform)
{
  char *platsub = holy_install_get_platform_name (platform);
  char *holydir = holy_util_path_concat (3, rootdir, subdir, platsub);
  char *load_cfg = holy_util_path_concat (2, holydir, "load.cfg");
  char *prefix;
  char *output;
  char *holy_cfg;
  FILE *cfg;

  holy_install_copy_files (input_dir, base, platform);
  holy_util_unlink (load_cfg);

  if (debug_image)
    {
      FILE *f = holy_util_fopen (load_cfg, "wb");
      if (!f)
	holy_util_error (_("cannot open `%s': %s"), load_cfg,
			 strerror (errno));
      fprintf (f, "set debug='%s'\n", debug_image);
      fclose (f);
    }
  else
    {
      free (load_cfg);
      load_cfg = 0;
    }

  prefix = xasprintf ("/%s", subdir);
  if (!targets[platform].mkimage_target)
    holy_util_error (_("unsupported platform %s"), platsub);

  holy_cfg = holy_util_path_concat (2, holydir, "holy.cfg");
  cfg = holy_util_fopen (holy_cfg, "wb");
  if (!cfg)
    holy_util_error (_("cannot open `%s': %s"), holy_cfg,
		     strerror (errno));
  fprintf (cfg, "source %s/holy.cfg", subdir);
  fclose (cfg);

  holy_install_push_module (targets[platform].netmodule);

  output = holy_util_path_concat_ext (2, holydir, "core", targets[platform].ext);
  holy_install_make_image_wrap (input_dir, prefix, output,
				0, load_cfg,
				targets[platform].mkimage_target, 0);
  holy_install_pop_module ();

  /* TRANSLATORS: First %s is replaced by platform name. Second one by filename.  */
  printf (_("Netboot directory for %s created. Configure your DHCP server to point to %s\n"),
	  platsub, output);

  free (platsub);
  free (output);
  free (prefix);
  free (holy_cfg);
  free (holydir);
}


int
main (int argc, char *argv[])
{
  const char *pkglibdir;

  holy_util_host_init (&argc, &argv);
  holy_util_disable_fd_syncs ();
  rootdir = xstrdup ("/srv/tftp");
  pkglibdir = holy_util_get_pkglibdir ();

  subdir = holy_util_path_concat (2, holy_BOOT_DIR_NAME, holy_DIR_NAME);

  argp_parse (&argp, argc, argv, 0, 0, 0);

  base = holy_util_path_concat (2, rootdir, subdir);
  /* Create the holy directory if it is not present.  */

  holy_install_mkdir_p (base);

  holy_install_push_module ("tftp");

  if (!holy_install_source_directory)
    {
      enum holy_install_plat plat;

      for (plat = 0; plat < holy_INSTALL_PLATFORM_MAX; plat++)
	if (targets[plat].mkimage_target)
	  {
	    char *platdir = holy_util_path_concat (2, pkglibdir,
						   holy_install_get_platform_name (plat));

	    holy_util_info ("Looking for `%s'", platdir);

	    if (!holy_util_is_directory (platdir))
	      {
		free (platdir);
		continue;
	      }
	    process_input_dir (platdir, plat);
	  }
    }
  else
    {
      enum holy_install_plat plat;
      plat = holy_install_get_target (holy_install_source_directory);
      process_input_dir (holy_install_source_directory, plat);
    }
  return 0;
}
