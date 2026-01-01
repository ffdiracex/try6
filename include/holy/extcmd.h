/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EXTCMD_HEADER
#define holy_EXTCMD_HEADER	1

#include <holy/lib/arg.h>
#include <holy/command.h>
#include <holy/script_sh.h>

struct holy_extcmd;
struct holy_extcmd_context;

typedef holy_err_t (*holy_extcmd_func_t) (struct holy_extcmd_context *ctxt,
					  int argc, char **args);

/* The argcmd description.  */
struct holy_extcmd
{
  holy_command_t cmd;

  holy_extcmd_func_t func;

  /* The argument parser optionlist.  */
  const struct holy_arg_option *options;

  void *data;
};
typedef struct holy_extcmd *holy_extcmd_t;

/* Command context for each instance of execution.  */
struct holy_extcmd_context
{
  struct holy_extcmd *extcmd;

  struct holy_arg_list *state;

  /* Script parameter, if any.  */
  struct holy_script *script;
};
typedef struct holy_extcmd_context *holy_extcmd_context_t;

holy_extcmd_t EXPORT_FUNC(holy_register_extcmd) (const char *name,
						 holy_extcmd_func_t func,
						 holy_command_flags_t flags,
						 const char *summary,
						 const char *description,
						 const struct holy_arg_option *parser);

holy_extcmd_t EXPORT_FUNC(holy_register_extcmd_prio) (const char *name,
						      holy_extcmd_func_t func,
						      holy_command_flags_t flags,
						      const char *summary,
						      const char *description,
						      const struct holy_arg_option *parser,
						      int prio);

void EXPORT_FUNC(holy_unregister_extcmd) (holy_extcmd_t cmd);

holy_err_t EXPORT_FUNC(holy_extcmd_dispatcher) (struct holy_command *cmd,
						int argc, char **args,
						struct holy_script *script);

#endif /* ! holy_EXTCMD_HEADER */
