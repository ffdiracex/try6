/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_COMMAND_HEADER
#define holy_COMMAND_HEADER	1

#include <holy/symbol.h>
#include <holy/err.h>
#include <holy/list.h>
#include <holy/misc.h>

typedef enum holy_command_flags
  {
    /* This is an extended command.  */
    holy_COMMAND_FLAG_EXTCMD = 0x10,
    /* This is an dynamic command.  */
    holy_COMMAND_FLAG_DYNCMD = 0x20,
    /* This command accepts block arguments.  */
    holy_COMMAND_FLAG_BLOCKS = 0x40,
    /* This command accepts unknown arguments as direct parameters.  */
    holy_COMMAND_ACCEPT_DASH = 0x80,
    /* This command accepts only options preceding direct arguments.  */
    holy_COMMAND_OPTIONS_AT_START = 0x100,
    /* Can be executed in an entries extractor.  */
    holy_COMMAND_FLAG_EXTRACTOR = 0x200
  } holy_command_flags_t;

struct holy_command;

typedef holy_err_t (*holy_command_func_t) (struct holy_command *cmd,
					   int argc, char **argv);

#define holy_COMMAND_PRIO_MASK	0xff
#define holy_COMMAND_FLAG_ACTIVE	0x100

/* The command description.  */
struct holy_command
{
  /* The next element.  */
  struct holy_command *next;
  struct holy_command **prev;

  /* The name.  */
  const char *name;

    /* The priority.  */
  int prio;

  /* The callback function.  */
  holy_command_func_t func;

  /* The flags.  */
  holy_command_flags_t flags;

  /* The summary of the command usage.  */
  const char *summary;

  /* The description of the command.  */
  const char *description;

  /* Arbitrary data.  */
  void *data;
};
typedef struct holy_command *holy_command_t;

extern holy_command_t EXPORT_VAR(holy_command_list);

holy_command_t
EXPORT_FUNC(holy_register_command_prio) (const char *name,
					 holy_command_func_t func,
					 const char *summary,
					 const char *description,
					 int prio);
void EXPORT_FUNC(holy_unregister_command) (holy_command_t cmd);

static inline holy_command_t
holy_register_command (const char *name,
		       holy_command_func_t func,
		       const char *summary,
		       const char *description)
{
  return holy_register_command_prio (name, func, summary, description, 0);
}

static inline holy_command_t
holy_register_command_p1 (const char *name,
			  holy_command_func_t func,
			  const char *summary,
			  const char *description)
{
  return holy_register_command_prio (name, func, summary, description, 1);
}

static inline holy_command_t
holy_command_find (const char *name)
{
  return holy_named_list_find (holy_AS_NAMED_LIST (holy_command_list), name);
}

static inline holy_err_t
holy_command_execute (const char *name, int argc, char **argv)
{
  holy_command_t cmd;

  cmd = holy_command_find (name);
  return (cmd) ? cmd->func (cmd, argc, argv) : holy_ERR_FILE_NOT_FOUND;
}

#define FOR_COMMANDS(var) FOR_LIST_ELEMENTS((var), holy_command_list)
#define FOR_COMMANDS_SAFE(var, next) FOR_LIST_ELEMENTS_SAFE((var), (next), holy_command_list)

void holy_register_core_commands (void);

#endif /* ! holy_COMMAND_HEADER */
