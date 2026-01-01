/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/extcmd.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/file.h>
#include <holy/normal.h>
#include <holy/script_sh.h>
#include <holy/i18n.h>
#include <holy/term.h>
#include <holy/syslinux_parse.h>
#include <holy/crypto.h>
#include <holy/auth.h>
#include <holy/disk.h>
#include <holy/partition.h>

holy_MOD_LICENSE ("GPLv2+");

/* Helper for syslinux_file.  */
static holy_err_t
syslinux_file_getline (char **line, int cont __attribute__ ((unused)),
		     void *data __attribute__ ((unused)))
{
  *line = 0;
  return holy_ERR_NONE;
}

static const struct holy_arg_option options[] =
  {
    {"root",  'r', 0,
     N_("root directory of the syslinux disk [default=/]."),
     N_("DIR"), ARG_TYPE_STRING},
    {"cwd",  'c', 0,
     N_("current directory of syslinux [default is parent directory of input file]."),
     N_("DIR"), ARG_TYPE_STRING},
    {"isolinux",     'i',  0, N_("assume input is an isolinux configuration file."), 0, 0},
    {"pxelinux",     'p',  0, N_("assume input is a pxelinux configuration file."), 0, 0},
    {"syslinux",     's',  0, N_("assume input is a syslinux configuration file."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

enum
  {
    OPTION_ROOT,
    OPTION_CWD,
    OPTION_ISOLINUX,
    OPTION_PXELINUX,
    OPTION_SYSLINUX
  };

static holy_err_t
syslinux_file (holy_extcmd_context_t ctxt, const char *filename)
{
  char *result;
  const char *root = ctxt->state[OPTION_ROOT].set ? ctxt->state[OPTION_ROOT].arg : "/";
  const char *cwd = ctxt->state[OPTION_CWD].set ? ctxt->state[OPTION_CWD].arg : NULL;
  holy_syslinux_flavour_t flav = holy_SYSLINUX_UNKNOWN;
  char *cwdf = NULL;
  holy_menu_t menu;

  if (ctxt->state[OPTION_ISOLINUX].set)
    flav = holy_SYSLINUX_ISOLINUX;
  if (ctxt->state[OPTION_PXELINUX].set)
    flav = holy_SYSLINUX_PXELINUX;
  if (ctxt->state[OPTION_SYSLINUX].set)
    flav = holy_SYSLINUX_SYSLINUX;

  if (!cwd)
    {
      char *p;
      cwdf = holy_strdup (filename);
      if (!cwdf)
	return holy_errno;
      p = holy_strrchr (cwdf, '/');
      if (!p)
	{
	  holy_free (cwdf);
	  cwd = "/";
	  cwdf = NULL;
	}
      else
	{
	  *p = '\0';
	  cwd = cwdf;
	}
    }

  holy_dprintf ("syslinux",
		"transforming syslinux config %s, root = %s, cwd = %s\n",
		filename, root, cwd);

  result = holy_syslinux_config_file (root, root, cwd, cwd, filename, flav);
  if (!result)
    return holy_errno;

  holy_dprintf ("syslinux", "syslinux config transformed\n");

  menu = holy_env_get_menu ();
  if (! menu)
    {
      menu = holy_zalloc (sizeof (*menu));
      if (! menu)
	{
	  holy_free (result);
	  return holy_errno;
	}

      holy_env_set_menu (menu);
    }

  holy_normal_parse_line (result, syslinux_file_getline, NULL);
  holy_print_error ();
  holy_free (result);
  holy_free (cwdf);

  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_syslinux_source (holy_extcmd_context_t ctxt,
			  int argc, char **args)
{
  int new_env, extractor;
  holy_err_t ret;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  extractor = (ctxt->extcmd->cmd->name[0] == 'e');
  new_env = (ctxt->extcmd->cmd->name[extractor ? (sizeof ("extract_syslinux_entries_") - 1)
			     : (sizeof ("syslinux_") - 1)] == 'c');

  if (new_env)
    holy_cls ();

  if (new_env && !extractor)
    holy_env_context_open ();
  if (extractor)
    holy_env_extractor_open (!new_env);

  ret = syslinux_file (ctxt, args[0]);

  if (new_env)
    {
      holy_menu_t menu;
      menu = holy_env_get_menu ();
      if (menu && menu->size)
	holy_show_menu (menu, 1, 0);
      if (!extractor)
	holy_env_context_close ();
    }
  if (extractor)
    holy_env_extractor_close (!new_env);

  return ret;
}


static holy_extcmd_t cmd_source, cmd_configfile;
static holy_extcmd_t cmd_source_extract, cmd_configfile_extract;

holy_MOD_INIT(syslinuxcfg)
{
  cmd_source
    = holy_register_extcmd ("syslinux_source",
			    holy_cmd_syslinux_source, 0,
			    N_("FILE"),
			    /* TRANSLATORS: "syslinux config" means
			       "config as used by syslinux".  */
			    N_("Execute syslinux config in same context"),
			    options);
  cmd_configfile
    = holy_register_extcmd ("syslinux_configfile",
			    holy_cmd_syslinux_source, 0,
			    N_("FILE"),
			    N_("Execute syslinux config in new context"),
			    options);
  cmd_source_extract
    = holy_register_extcmd ("extract_syslinux_entries_source",
			    holy_cmd_syslinux_source, 0,
			    N_("FILE"),
			    N_("Execute syslinux config in same context taking only menu entries"),
			    options);
  cmd_configfile_extract
    = holy_register_extcmd ("extract_syslinux_entries_configfile",
			    holy_cmd_syslinux_source, 0,
			    N_("FILE"),
			    N_("Execute syslinux config in new context taking only menu entries"),
			    options);
}

holy_MOD_FINI(syslinuxcfg)
{
  holy_unregister_extcmd (cmd_source);
  holy_unregister_extcmd (cmd_configfile);
  holy_unregister_extcmd (cmd_source_extract);
  holy_unregister_extcmd (cmd_configfile_extract);
}
