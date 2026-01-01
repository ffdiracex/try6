/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/disk.h>
#include <holy/device.h>
#include <holy/term.h>
#include <holy/partition.h>
#include <holy/file.h>
#include <holy/normal.h>
#include <holy/extcmd.h>
#include <holy/datetime.h>
#include <holy/i18n.h>
#include <holy/net.h>

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options[] =
  {
    {"long", 'l', 0, N_("Show a long list with more detailed information."), 0, 0},
    {"human-readable", 'h', 0, N_("Print sizes in a human readable format."), 0, 0},
    {"all", 'a', 0, N_("List all files."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

/* Helper for holy_ls_list_devices.  */
static int
holy_ls_print_devices (const char *name, void *data)
{
  int *longlist = data;

  if (*longlist)
    holy_normal_print_device_info (name);
  else
    holy_printf ("(%s) ", name);

  return 0;
}

static holy_err_t
holy_ls_list_devices (int longlist)
{
  holy_device_iterate (holy_ls_print_devices, &longlist);
  holy_xputs ("\n");

#if 0
  {
    holy_net_app_level_t proto;
    int first = 1;
    FOR_NET_APP_LEVEL (proto)
    {
      if (first)
	holy_puts_ (N_ ("Network protocols:"));
      first = 0;
      holy_printf ("%s ", proto->name);
    }
    holy_xputs ("\n");
  }
#endif

  holy_refresh ();

  return 0;
}

/* Context for holy_ls_list_files.  */
struct holy_ls_list_files_ctx
{
  char *dirname;
  int all;
  int human;
};

/* Helper for holy_ls_list_files.  */
static int
print_files (const char *filename, const struct holy_dirhook_info *info,
	     void *data)
{
  struct holy_ls_list_files_ctx *ctx = data;

  if (ctx->all || filename[0] != '.')
    holy_printf ("%s%s ", filename, info->dir ? "/" : "");

  return 0;
}

/* Helper for holy_ls_list_files.  */
static int
print_files_long (const char *filename, const struct holy_dirhook_info *info,
		  void *data)
{
  struct holy_ls_list_files_ctx *ctx = data;

  if ((! ctx->all) && (filename[0] == '.'))
    return 0;

  if (! info->dir)
    {
      holy_file_t file;
      char *pathname;

      if (ctx->dirname[holy_strlen (ctx->dirname) - 1] == '/')
	pathname = holy_xasprintf ("%s%s", ctx->dirname, filename);
      else
	pathname = holy_xasprintf ("%s/%s", ctx->dirname, filename);

      if (!pathname)
	return 1;

      /* XXX: For ext2fs symlinks are detected as files while they
	 should be reported as directories.  */
      holy_file_filter_disable_compression ();
      file = holy_file_open (pathname);
      if (! file)
	{
	  holy_errno = 0;
	  holy_free (pathname);
	  return 0;
	}

      if (! ctx->human)
	holy_printf ("%-12llu", (unsigned long long) file->size);
      else
	holy_printf ("%-12s", holy_get_human_size (file->size,
						   holy_HUMAN_SIZE_SHORT));
      holy_file_close (file);
      holy_free (pathname);
    }
  else
    holy_printf ("%-12s", _("DIR"));

  if (info->mtimeset)
    {
      struct holy_datetime datetime;
      holy_unixtime2datetime (info->mtime, &datetime);
      if (ctx->human)
	holy_printf (" %d-%02d-%02d %02d:%02d:%02d %-11s ",
		     datetime.year, datetime.month, datetime.day,
		     datetime.hour, datetime.minute,
		     datetime.second,
		     holy_get_weekday_name (&datetime));
      else
	holy_printf (" %04d%02d%02d%02d%02d%02d ",
		     datetime.year, datetime.month,
		     datetime.day, datetime.hour,
		     datetime.minute, datetime.second);
    }
  holy_printf ("%s%s\n", filename, info->dir ? "/" : "");

  return 0;
}

static holy_err_t
holy_ls_list_files (char *dirname, int longlist, int all, int human)
{
  char *device_name;
  holy_fs_t fs;
  const char *path;
  holy_device_t dev;

  device_name = holy_file_get_device_name (dirname);
  dev = holy_device_open (device_name);
  if (! dev)
    goto fail;

  fs = holy_fs_probe (dev);
  path = holy_strchr (dirname, ')');
  if (! path)
    path = dirname;
  else
    path++;

  if (! path && ! device_name)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, "invalid argument");
      goto fail;
    }

  if (! *path)
    {
      if (holy_errno == holy_ERR_UNKNOWN_FS)
	holy_errno = holy_ERR_NONE;

      holy_normal_print_device_info (device_name);
    }
  else if (fs)
    {
      struct holy_ls_list_files_ctx ctx = {
	.dirname = dirname,
	.all = all,
	.human = human
      };

      if (longlist)
	(fs->dir) (dev, path, print_files_long, &ctx);
      else
	(fs->dir) (dev, path, print_files, &ctx);

      if (holy_errno == holy_ERR_BAD_FILE_TYPE
	  && path[holy_strlen (path) - 1] != '/')
	{
	  /* PATH might be a regular file.  */
	  char *p;
	  holy_file_t file;
	  struct holy_dirhook_info info;
	  holy_errno = 0;

	  holy_file_filter_disable_compression ();
	  file = holy_file_open (dirname);
	  if (! file)
	    goto fail;

	  holy_file_close (file);

	  p = holy_strrchr (dirname, '/') + 1;
	  dirname = holy_strndup (dirname, p - dirname);
	  if (! dirname)
	    goto fail;

	  all = 1;
	  holy_memset (&info, 0, sizeof (info));
	  if (longlist)
	    print_files_long (p, &info, &ctx);
	  else
	    print_files (p, &info, &ctx);

	  holy_free (dirname);
	}

      if (holy_errno == holy_ERR_NONE)
	holy_xputs ("\n");

      holy_refresh ();
    }

 fail:
  if (dev)
    holy_device_close (dev);

  holy_free (device_name);

  return 0;
}

static holy_err_t
holy_cmd_ls (holy_extcmd_context_t ctxt, int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;
  int i;

  if (argc == 0)
    holy_ls_list_devices (state[0].set);
  else
    for (i = 0; i < argc; i++)
      holy_ls_list_files (args[i], state[0].set, state[2].set,
			  state[1].set);

  return 0;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(ls)
{
  cmd = holy_register_extcmd ("ls", holy_cmd_ls, 0,
			      N_("[-l|-h|-a] [FILE ...]"),
			      N_("List devices and files."), options);
}

holy_MOD_FINI(ls)
{
  holy_unregister_extcmd (cmd);
}
