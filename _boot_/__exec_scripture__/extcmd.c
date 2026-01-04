/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/list.h>
#include <holy/misc.h>
#include <holy/extcmd.h>
#include <holy/script_sh.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

holy_err_t
holy_extcmd_dispatcher (struct holy_command *cmd, int argc, char **args,
			struct holy_script *script)
{
  int new_argc;
  char **new_args;
  struct holy_arg_list *state;
  struct holy_extcmd_context context;
  holy_err_t ret;
  holy_extcmd_t ext = cmd->data;

  context.state = 0;
  context.extcmd = ext;
  context.script = script;

  if (! ext->options)
    {
      ret = (ext->func) (&context, argc, args);
      return ret;
    }

  state = holy_arg_list_alloc (ext, argc, args);
  if (holy_arg_parse (ext, argc, args, state, &new_args, &new_argc))
    {
      context.state = state;
      ret = (ext->func) (&context, new_argc, new_args);
      holy_free (new_args);
      holy_free (state);
      return ret;
    }

  holy_free (state);
  return holy_errno;
}

static holy_err_t
holy_extcmd_dispatch (struct holy_command *cmd, int argc, char **args)
{
  return holy_extcmd_dispatcher (cmd, argc, args, 0);
}

holy_extcmd_t
holy_register_extcmd_prio (const char *name, holy_extcmd_func_t func,
			   holy_command_flags_t flags, const char *summary,
			   const char *description,
			   const struct holy_arg_option *parser,
			   int prio)
{
  holy_extcmd_t ext;
  holy_command_t cmd;

  ext = (holy_extcmd_t) holy_malloc (sizeof (*ext));
  if (! ext)
    return 0;

  cmd = holy_register_command_prio (name, holy_extcmd_dispatch,
				    summary, description, prio);
  if (! cmd)
    {
      holy_free (ext);
      return 0;
    }

  cmd->flags = (flags | holy_COMMAND_FLAG_EXTCMD);
  cmd->data = ext;

  ext->cmd = cmd;
  ext->func = func;
  ext->options = parser;
  ext->data = 0;

  return ext;
}

holy_extcmd_t
holy_register_extcmd (const char *name, holy_extcmd_func_t func,
		      holy_command_flags_t flags, const char *summary,
		      const char *description,
		      const struct holy_arg_option *parser)
{
  return holy_register_extcmd_prio (name, func, flags,
				    summary, description, parser, 1);
}

void
holy_unregister_extcmd (holy_extcmd_t ext)
{
  holy_unregister_command (ext->cmd);
  holy_free (ext);
}
