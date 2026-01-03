/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/device.h>
#include <holy/mm.h>
#include <holy/fs.h>
#include <holy/env.h>
#include <holy/file.h>

holy_MOD_LICENSE ("GPLv2+");

static const char *modnames_def[] = { 
  /* FIXME: autogenerate this.  */
#if defined (__i386__) || defined (__x86_64__) || defined (holy_MACHINE_MIPS_LOONGSON)
  "pata", "ahci", "usbms", "ohci", "uhci", "ehci"
#elif defined (holy_MACHINE_MIPS_QEMU_MIPS)
  "pata"
#else
#error "Fill this"
#endif
 };

static holy_err_t
get_uuid (const char *name, char **uuid, int getnative)
{
  holy_device_t dev;
  holy_fs_t fs = 0;

  *uuid = 0;

  dev = holy_device_open (name);
  if (!dev)
    return holy_errno;

  if (!dev->disk)
    {
      holy_dprintf ("nativedisk", "Skipping non-disk\n");
      holy_device_close (dev);
      return 0;
    }

  switch (dev->disk->dev->id)
    {
      /* Firmware disks.  */
    case holy_DISK_DEVICE_BIOSDISK_ID:
    case holy_DISK_DEVICE_OFDISK_ID:
    case holy_DISK_DEVICE_EFIDISK_ID:
    case holy_DISK_DEVICE_NAND_ID:
    case holy_DISK_DEVICE_ARCDISK_ID:
    case holy_DISK_DEVICE_HOSTDISK_ID:
    case holy_DISK_DEVICE_UBOOTDISK_ID:
      break;

      /* Native disks.  */
    case holy_DISK_DEVICE_ATA_ID:
    case holy_DISK_DEVICE_SCSI_ID:
    case holy_DISK_DEVICE_XEN:
      if (getnative)
	break;
      /* FALLTHROUGH */

      /* Virtual disks.  */
      /* holy dynamically generated files.  */
    case holy_DISK_DEVICE_PROCFS_ID:
      /* To access through host OS routines (holy-emu only).  */
    case holy_DISK_DEVICE_HOST_ID:
      /* To access coreboot roms.  */
    case holy_DISK_DEVICE_CBFSDISK_ID:
      /* holy-only memdisk. Can't match any of firmware devices.  */
    case holy_DISK_DEVICE_MEMDISK_ID:
      holy_dprintf ("nativedisk", "Skipping native disk %s\n",
		    dev->disk->name);
      holy_device_close (dev);
      return 0;

      /* FIXME: those probably need special handling.  */
    case holy_DISK_DEVICE_LOOPBACK_ID:
    case holy_DISK_DEVICE_DISKFILTER_ID:
    case holy_DISK_DEVICE_CRYPTODISK_ID:
      break;
    }
  if (dev)
    fs = holy_fs_probe (dev);
  if (!fs)
    {
      holy_device_close (dev);
      return holy_errno;
    }
  if (!fs->uuid || fs->uuid (dev, uuid) || !*uuid)
    {
      holy_device_close (dev);

      if (!holy_errno)
	holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		    N_("%s does not support UUIDs"), fs->name);

      return holy_errno;
    }
  holy_device_close (dev);
  return holy_ERR_NONE;
}

struct search_ctx
{
  char *root_uuid;
  char *prefix_uuid;
  const char *prefix_path;
  int prefix_found, root_found;
};

static int
iterate_device (const char *name, void *data)
{
  struct search_ctx *ctx = data;
  char *cur_uuid;

  if (get_uuid (name, &cur_uuid, 1))
    {
      if (holy_errno == holy_ERR_UNKNOWN_FS)
	holy_errno = 0;
      holy_print_error ();
      return 0;
    }

  holy_dprintf ("nativedisk", "checking %s: %s\n", name,
		cur_uuid);
  if (ctx->prefix_uuid && holy_strcasecmp (cur_uuid, ctx->prefix_uuid) == 0)
    {
      char *prefix;
      prefix = holy_xasprintf ("(%s)/%s", name, ctx->prefix_path);
      holy_env_set ("prefix", prefix);
      holy_free (prefix);
      ctx->prefix_found = 1;
    }
  if (ctx->root_uuid && holy_strcasecmp (cur_uuid, ctx->root_uuid) == 0)
    {
      holy_env_set ("root", name);
      ctx->root_found = 1;
    }
  return ctx->prefix_found && ctx->root_found;
}

static holy_err_t
holy_cmd_nativedisk (holy_command_t cmd __attribute__ ((unused)),
		     int argc, char **args_in)
{
  char *uuid_root = 0, *uuid_prefix, *prefdev = 0;
  const char *prefix = 0;
  const char *path_prefix = 0;
  int mods_loaded = 0;
  holy_dl_t *mods;
  const char **args;
  int i;

  if (argc == 0)
    {
      argc = ARRAY_SIZE (modnames_def);
      args = modnames_def;
    }
  else
    args = (const char **) args_in;

  prefix = holy_env_get ("prefix");

  if (! prefix)
    return holy_error (holy_ERR_FILE_NOT_FOUND, N_("variable `%s' isn't set"), "prefix");

  if (prefix)
    path_prefix = (prefix[0] == '(') ? holy_strchr (prefix, ')') : NULL;
  if (path_prefix)
    path_prefix++;
  else
    path_prefix = prefix;

  mods = holy_malloc (argc * sizeof (mods[0]));
  if (!mods)
    return holy_errno;

  if (get_uuid (NULL, &uuid_root, 0))
    {
      holy_free (mods);
      return holy_errno;
    }

  prefdev = holy_file_get_device_name (prefix);
  if (holy_errno)
    {
      holy_print_error ();
      prefdev = 0;
    }

  if (get_uuid (prefdev, &uuid_prefix, 0))
    {
      holy_free (uuid_root);
      holy_free (prefdev);
      holy_free (mods);
      return holy_errno;
    }

  holy_dprintf ("nativedisk", "uuid_prefix = %s, uuid_root = %s\n",
		uuid_prefix, uuid_root);

  for (mods_loaded = 0; mods_loaded < argc; mods_loaded++)
    {
      char *filename;
      holy_dl_t mod;
      holy_file_t file = NULL;
      holy_ssize_t size;
      void *core = 0;

      mod = holy_dl_get (args[mods_loaded]);
      if (mod)
	{
	  mods[mods_loaded] = 0;
	  continue;
	}

      filename = holy_xasprintf ("%s/" holy_TARGET_CPU "-" holy_PLATFORM "/%s.mod",
				 prefix, args[mods_loaded]);
      if (! filename)
	goto fail;

      file = holy_file_open (filename);
      holy_free (filename);
      if (! file)
	goto fail;

      size = holy_file_size (file);
      core = holy_malloc (size);
      if (! core)
	{
	  holy_file_close (file);
	  goto fail;
	}

      if (holy_file_read (file, core, size) != (holy_ssize_t) size)
	{
	  holy_file_close (file);
	  holy_free (core);
	  goto fail;
	}

      holy_file_close (file);

      mods[mods_loaded] = holy_dl_load_core_noinit (core, size);
      if (! mods[mods_loaded])
	goto fail;
    }

  for (i = 0; i < argc; i++)
    if (mods[i])
      holy_dl_init (mods[i]);

  if (uuid_prefix || uuid_root)
    {
      struct search_ctx ctx;
      holy_fs_autoload_hook_t saved_autoload;

      /* No need to autoload FS since obviously we already have the necessary fs modules.  */
      saved_autoload = holy_fs_autoload_hook;
      holy_fs_autoload_hook = 0;

      ctx.root_uuid = uuid_root;
      ctx.prefix_uuid = uuid_prefix;
      ctx.prefix_path = path_prefix;
      ctx.prefix_found = !uuid_prefix;
      ctx.root_found = !uuid_root;

      /* FIXME: try to guess the correct values.  */
      holy_device_iterate (iterate_device, &ctx);

      holy_fs_autoload_hook = saved_autoload;
    }
  holy_free (uuid_root);
  holy_free (uuid_prefix);
  holy_free (prefdev);
  holy_free (mods);

  return holy_ERR_NONE;

 fail:
  holy_free (uuid_root);
  holy_free (uuid_prefix);
  holy_free (prefdev);

  for (i = 0; i < mods_loaded; i++)
    if (mods[i])
      {
	mods[i]->fini = 0;
	holy_dl_unload (mods[i]);
      }
  holy_free (mods);

  return holy_errno;
}

static holy_command_t cmd;

holy_MOD_INIT(nativedisk)
{
  cmd = holy_register_command ("nativedisk", holy_cmd_nativedisk, N_("[MODULE1 MODULE2 ...]"),
			       N_("Switch to native disk drivers. If no modules are specified default set (pata,ahci,usbms,ohci,uhci,ehci) is used"));
}

holy_MOD_FINI(nativedisk)
{
  holy_unregister_command (cmd);
}
