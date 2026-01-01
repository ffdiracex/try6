/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/dl.h>
#include <holy/err.h>
#include <holy/env.h>
#include <holy/misc.h>
#include <holy/term.h>
#include <holy/file.h>
#include <holy/device.h>
#include <holy/command.h>
#include <holy/i18n.h>

/* set ENVVAR=VALUE */
static holy_err_t
holy_core_cmd_set (struct holy_command *cmd __attribute__ ((unused)),
		   int argc, char *argv[])
{
  char *var;
  char *val;

  if (argc < 1)
    {
      struct holy_env_var *env;
      FOR_SORTED_ENV (env)
	holy_printf ("%s=%s\n", env->name, holy_env_get (env->name));
      return 0;
    }

  var = argv[0];
  val = holy_strchr (var, '=');
  if (! val)
    return holy_error (holy_ERR_BAD_ARGUMENT, "not an assignment");

  val[0] = 0;
  holy_env_set (var, val + 1);
  val[0] = '=';

  return 0;
}

static holy_err_t
holy_core_cmd_unset (struct holy_command *cmd __attribute__ ((unused)),
		     int argc, char *argv[])
{
  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("one argument expected"));

  holy_env_unset (argv[0]);
  return 0;
}

/* insmod MODULE */
static holy_err_t
holy_core_cmd_insmod (struct holy_command *cmd __attribute__ ((unused)),
		      int argc, char *argv[])
{
  holy_dl_t mod;

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));

  if (argv[0][0] == '/' || argv[0][0] == '(' || argv[0][0] == '+')
    mod = holy_dl_load_file (argv[0]);
  else
    mod = holy_dl_load (argv[0]);

  if (mod)
    holy_dl_ref (mod);

  return 0;
}

static int
holy_mini_print_devices (const char *name, void *data __attribute__ ((unused)))
{
  holy_printf ("(%s) ", name);

  return 0;
}

static int
holy_mini_print_files (const char *filename,
		       const struct holy_dirhook_info *info,
		       void *data __attribute__ ((unused)))
{
  holy_printf ("%s%s ", filename, info->dir ? "/" : "");

  return 0;
}

/* ls [ARG] */
static holy_err_t
holy_core_cmd_ls (struct holy_command *cmd __attribute__ ((unused)),
		  int argc, char *argv[])
{
  if (argc < 1)
    {
      holy_device_iterate (holy_mini_print_devices, NULL);
      holy_xputs ("\n");
      holy_refresh ();
    }
  else
    {
      char *device_name;
      holy_device_t dev = 0;
      holy_fs_t fs;
      char *path;

      device_name = holy_file_get_device_name (argv[0]);
      if (holy_errno)
	goto fail;
      dev = holy_device_open (device_name);
      if (! dev)
	goto fail;

      fs = holy_fs_probe (dev);
      path = holy_strchr (argv[0], ')');
      if (! path)
	path = argv[0];
      else
	path++;

      if (! *path && ! device_name)
	{
	  holy_error (holy_ERR_BAD_ARGUMENT, "invalid argument");
	  goto fail;
	}

      if (! *path)
	{
	  if (holy_errno == holy_ERR_UNKNOWN_FS)
	    holy_errno = holy_ERR_NONE;

	  holy_printf ("(%s): Filesystem is %s.\n",
		       device_name, fs ? fs->name : "unknown");
	}
      else if (fs)
	{
	  (fs->dir) (dev, path, holy_mini_print_files, NULL);
	  holy_xputs ("\n");
	  holy_refresh ();
	}

    fail:
      if (dev)
	holy_device_close (dev);

      holy_free (device_name);
    }

  return holy_errno;
}

void
holy_register_core_commands (void)
{
  holy_command_t cmd;
  cmd = holy_register_command ("set", holy_core_cmd_set,
			       N_("[ENVVAR=VALUE]"),
			       N_("Set an environment variable."));
  if (cmd)
    cmd->flags |= holy_COMMAND_FLAG_EXTRACTOR;
  holy_register_command ("unset", holy_core_cmd_unset,
			 N_("ENVVAR"),
			 N_("Remove an environment variable."));
  holy_register_command ("ls", holy_core_cmd_ls,
			 N_("[ARG]"), N_("List devices or files."));
  holy_register_command ("insmod", holy_core_cmd_insmod,
			 N_("MODULE"), N_("Insert a module."));
}
