/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/term.h>
#include <holy/env.h>
#include <holy/normal.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t
holy_cmd_source (holy_command_t cmd, int argc, char **args)
{
  int new_env, extractor;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  extractor = (cmd->name[0] == 'e');
  new_env = (cmd->name[extractor ? sizeof ("extract_entries_") - 1 : 0] == 'c');

  if (new_env)
    holy_cls ();

  if (new_env && !extractor)
    holy_env_context_open ();
  if (extractor)
    holy_env_extractor_open (!new_env);

  holy_normal_execute (args[0], 1, ! new_env);

  if (new_env && !extractor)
    holy_env_context_close ();
  if (extractor)
    holy_env_extractor_close (!new_env);

  return 0;
}

static holy_command_t cmd_configfile, cmd_source, cmd_dot;
static holy_command_t cmd_extractor_source, cmd_extractor_configfile;

holy_MOD_INIT(configfile)
{
  cmd_configfile =
    holy_register_command ("configfile", holy_cmd_source,
			   N_("FILE"), N_("Load another config file."));
  cmd_source =
    holy_register_command ("source", holy_cmd_source,
			   N_("FILE"),
			   N_("Load another config file without changing context.")
			   );

  cmd_extractor_source =
    holy_register_command ("extract_entries_source", holy_cmd_source,
			   N_("FILE"),
			   N_("Load another config file without changing context but take only menu entries.")
			   );

  cmd_extractor_configfile =
    holy_register_command ("extract_entries_configfile", holy_cmd_source,
			   N_("FILE"),
			   N_("Load another config file but take only menu entries.")
			   );

  cmd_dot =
    holy_register_command (".", holy_cmd_source,
			   N_("FILE"),
			   N_("Load another config file without changing context.")
			   );
}

holy_MOD_FINI(configfile)
{
  holy_unregister_command (cmd_configfile);
  holy_unregister_command (cmd_source);
  holy_unregister_command (cmd_extractor_configfile);
  holy_unregister_command (cmd_extractor_source);
  holy_unregister_command (cmd_dot);
}
