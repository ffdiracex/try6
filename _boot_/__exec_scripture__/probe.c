/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/device.h>
#include <holy/disk.h>
#include <holy/partition.h>
#include <holy/net.h>
#include <holy/fs.h>
#include <holy/file.h>
#include <holy/misc.h>
#include <holy/env.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options[] =
  {
    {"set",             's', 0,
     N_("Set a variable to return value."), N_("VARNAME"), ARG_TYPE_STRING},
    /* TRANSLATORS: It's a driver that is currently in use to access
       the diven disk.  */
    {"driver",		'd', 0, N_("Determine driver."), 0, 0},
    {"partmap",		'p', 0, N_("Determine partition map type."), 0, 0},
    {"fs",		'f', 0, N_("Determine filesystem type."), 0, 0},
    {"fs-uuid",		'u', 0, N_("Determine filesystem UUID."), 0, 0},
    {"label",		'l', 0, N_("Determine filesystem label."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static holy_err_t
holy_cmd_probe (holy_extcmd_context_t ctxt, int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;
  holy_device_t dev;
  holy_fs_t fs;
  char *ptr;
  holy_err_t err;

  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, "device name required");

  ptr = args[0] + holy_strlen (args[0]) - 1;
  if (args[0][0] == '(' && *ptr == ')')
    {
      *ptr = 0;
      dev = holy_device_open (args[0] + 1);
      *ptr = ')';
    }
  else
    dev = holy_device_open (args[0]);
  if (! dev)
    return holy_errno;

  if (state[1].set)
    {
      const char *val = "none";
      if (dev->net)
	val = dev->net->protocol->name;
      if (dev->disk)
	val = dev->disk->dev->name;
      if (state[0].set)
	holy_env_set (state[0].arg, val);
      else
	holy_printf ("%s", val);
      holy_device_close (dev);
      return holy_ERR_NONE;
    }
  if (state[2].set)
    {
      const char *val = "none";
      if (dev->disk && dev->disk->partition)
	val = dev->disk->partition->partmap->name;
      if (state[0].set)
	holy_env_set (state[0].arg, val);
      else
	holy_printf ("%s", val);
      holy_device_close (dev);
      return holy_ERR_NONE;
    }
  fs = holy_fs_probe (dev);
  if (! fs)
    return holy_errno;
  if (state[3].set)
    {
      if (state[0].set)
	holy_env_set (state[0].arg, fs->name);
      else
	holy_printf ("%s", fs->name);
      holy_device_close (dev);
      return holy_ERR_NONE;
    }
  if (state[4].set)
    {
      char *uuid;
      if (! fs->uuid)
	return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
			   N_("%s does not support UUIDs"), fs->name);
      err = fs->uuid (dev, &uuid);
      if (err)
	return err;
      if (! uuid)
	return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
			   N_("%s does not support UUIDs"), fs->name);

      if (state[0].set)
	holy_env_set (state[0].arg, uuid);
      else
	holy_printf ("%s", uuid);
      holy_free (uuid);
      holy_device_close (dev);
      return holy_ERR_NONE;
    }
  if (state[5].set)
    {
      char *label;
      if (! fs->label)
	return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
			   N_("filesystem `%s' does not support labels"),
			   fs->name);
      err = fs->label (dev, &label);
      if (err)
	return err;
      if (! label)
	return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
			   N_("filesystem `%s' does not support labels"),
			   fs->name);

      if (state[0].set)
	holy_env_set (state[0].arg, label);
      else
	holy_printf ("%s", label);
      holy_free (label);
      holy_device_close (dev);
      return holy_ERR_NONE;
    }
  holy_device_close (dev);
  return holy_error (holy_ERR_BAD_ARGUMENT, "unrecognised target");
}

static holy_extcmd_t cmd;

holy_MOD_INIT (probe)
{
  cmd = holy_register_extcmd ("probe", holy_cmd_probe, 0, N_("DEVICE"),
			      N_("Retrieve device info."), options);
}

holy_MOD_FINI (probe)
{
  holy_unregister_extcmd (cmd);
}
