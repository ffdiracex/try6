/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/script_sh.h>
#include <holy/parser.h>
#include <holy/mm.h>

/* It is not possible to deallocate the memory when a syntax error was
   found.  Because of that it is required to keep track of all memory
   allocations.  The memory is freed in case of an error, or assigned
   to the parsed script when parsing was successful.

   In case of the normal malloc, some additional bytes are allocated
   for this datastructure.  All reserved memory is stored in a linked
   list so it can be easily freed.  The original memory can be found
   from &mem.  */
struct holy_script_mem
{
  struct holy_script_mem *next;
  char mem;
};

/* Return malloc'ed memory and keep track of the allocation.  */
void *
holy_script_malloc (struct holy_parser_param *state, holy_size_t size)
{
  struct holy_script_mem *mem;
  mem = (struct holy_script_mem *) holy_malloc (size + sizeof (*mem)
						- sizeof (char));
  if (!mem)
    return 0;

  holy_dprintf ("scripting", "malloc %p\n", mem);
  mem->next = state->memused;
  state->memused = mem;
  return (void *) &mem->mem;
}

/* Free all memory described by MEM.  */
void
holy_script_mem_free (struct holy_script_mem *mem)
{
  struct holy_script_mem *memfree;

  while (mem)
    {
      memfree = mem->next;
      holy_dprintf ("scripting", "free %p\n", mem);
      holy_free (mem);
      mem = memfree;
    }
}

/* Start recording memory usage.  Returns the memory that should be
   restored when calling stop.  */
struct holy_script_mem *
holy_script_mem_record (struct holy_parser_param *state)
{
  struct holy_script_mem *mem = state->memused;
  state->memused = 0;

  return mem;
}

/* Stop recording memory usage.  Restore previous recordings using
   RESTORE.  Return the recorded memory.  */
struct holy_script_mem *
holy_script_mem_record_stop (struct holy_parser_param *state,
			     struct holy_script_mem *restore)
{
  struct holy_script_mem *mem = state->memused;
  state->memused = restore;
  return mem;
}

/* Free the memory reserved for CMD and all of it's children.  */
void
holy_script_free (struct holy_script *script)
{
  struct holy_script *s;
  struct holy_script *t;

  if (! script)
    return;

  if (script->mem)
    holy_script_mem_free (script->mem);

  s = script->children;
  while (s) {
    t = s->next_siblings;
    holy_script_unref (s);
    s = t;
  }
  holy_free (script);
}



/* Extend the argument arg with a variable or string of text.  If ARG
   is zero a new list is created.  */
struct holy_script_arg *
holy_script_arg_add (struct holy_parser_param *state,
		     struct holy_script_arg *arg, holy_script_arg_type_t type,
		     char *str)
{
  struct holy_script_arg *argpart;
  struct holy_script_arg *ll;
  int len;

  argpart =
    (struct holy_script_arg *) holy_script_malloc (state, sizeof (*arg));
  if (!argpart)
    return arg;

  argpart->type = type;
  argpart->script = 0;

  len = holy_strlen (str) + 1;
  argpart->str = holy_script_malloc (state, len);
  if (!argpart->str)
    return arg; /* argpart is freed later, during holy_script_free.  */

  holy_memcpy (argpart->str, str, len);
  argpart->next = 0;

  if (!arg)
    return argpart;

  for (ll = arg; ll->next; ll = ll->next);
  ll->next = argpart;

  return arg;
}

/* Add the argument ARG to the end of the argument list LIST.  If LIST
   is zero, a new list will be created.  */
struct holy_script_arglist *
holy_script_add_arglist (struct holy_parser_param *state,
			 struct holy_script_arglist *list,
			 struct holy_script_arg *arg)
{
  struct holy_script_arglist *link;
  struct holy_script_arglist *ll;

  holy_dprintf ("scripting", "arglist\n");

  link =
    (struct holy_script_arglist *) holy_script_malloc (state, sizeof (*link));
  if (!link)
    return list;

  link->next = 0;
  link->arg = arg;
  link->argcount = 0;

  if (!list)
    {
      link->argcount++;
      return link;
    }

  list->argcount++;

  /* Look up the last link in the chain.  */
  for (ll = list; ll->next; ll = ll->next);
  ll->next = link;

  return list;
}

/* Create a command that describes a single command line.  CMDLINE
   contains the name of the command that should be executed.  ARGLIST
   holds all arguments for this command.  */
struct holy_script_cmd *
holy_script_create_cmdline (struct holy_parser_param *state,
			    struct holy_script_arglist *arglist)
{
  struct holy_script_cmdline *cmd;

  holy_dprintf ("scripting", "cmdline\n");

  cmd = holy_script_malloc (state, sizeof (*cmd));
  if (!cmd)
    return 0;

  cmd->cmd.exec = holy_script_execute_cmdline;
  cmd->cmd.next = 0;
  cmd->arglist = arglist;

  return (struct holy_script_cmd *) cmd;
}

/* Create a command that functions as an if statement.  If BOOL is
   evaluated to true (the value is returned in envvar '?'), the
   interpreter will run the command TRUE, otherwise the interpreter
   runs the command FALSE.  */
struct holy_script_cmd *
holy_script_create_cmdif (struct holy_parser_param *state,
			  struct holy_script_cmd *exec_to_evaluate,
			  struct holy_script_cmd *exec_on_true,
			  struct holy_script_cmd *exec_on_false)
{
  struct holy_script_cmdif *cmd;

  holy_dprintf ("scripting", "cmdif\n");

  cmd = holy_script_malloc (state, sizeof (*cmd));
  if (!cmd)
    return 0;

  cmd->cmd.exec = holy_script_execute_cmdif;
  cmd->cmd.next = 0;
  cmd->exec_to_evaluate = exec_to_evaluate;
  cmd->exec_on_true = exec_on_true;
  cmd->exec_on_false = exec_on_false;

  return (struct holy_script_cmd *) cmd;
}

/* Create a command that functions as a for statement.  */
struct holy_script_cmd *
holy_script_create_cmdfor (struct holy_parser_param *state,
			   struct holy_script_arg *name,
			   struct holy_script_arglist *words,
			   struct holy_script_cmd *list)
{
  struct holy_script_cmdfor *cmd;

  holy_dprintf ("scripting", "cmdfor\n");

  cmd = holy_script_malloc (state, sizeof (*cmd));
  if (! cmd)
    return 0;

  cmd->cmd.exec = holy_script_execute_cmdfor;
  cmd->cmd.next = 0;
  cmd->name = name;
  cmd->words = words;
  cmd->list = list;

  return (struct holy_script_cmd *) cmd;
}

/* Create a "while" or "until" command.  */
struct holy_script_cmd *
holy_script_create_cmdwhile (struct holy_parser_param *state,
			     struct holy_script_cmd *cond,
			     struct holy_script_cmd *list,
			     int is_an_until_loop)
{
  struct holy_script_cmdwhile *cmd;

  cmd = holy_script_malloc (state, sizeof (*cmd));
  if (! cmd)
    return 0;

  cmd->cmd.exec = holy_script_execute_cmdwhile;
  cmd->cmd.next = 0;
  cmd->cond = cond;
  cmd->list = list;
  cmd->until = is_an_until_loop;

  return (struct holy_script_cmd *) cmd;
}

/* Create a chain of commands.  LAST contains the command that should
   be added at the end of LIST's list.  If LIST is zero, a new list
   will be created.  */
struct holy_script_cmd *
holy_script_append_cmd (struct holy_parser_param *state,
			struct holy_script_cmd *list,
			struct holy_script_cmd *last)
{
  struct holy_script_cmd *ptr;

  holy_dprintf ("scripting", "append command\n");

  if (! last)
    return list;

  if (! list)
    {
      list = holy_script_malloc (state, sizeof (*list));
      if (! list)
	return 0;

      list->exec = holy_script_execute_cmdlist;
      list->next = last;
    }
  else
    {
      ptr = list;
      while (ptr->next)
	ptr = ptr->next;

      ptr->next = last;
    }

  return list;
}



struct holy_script *
holy_script_create (struct holy_script_cmd *cmd, struct holy_script_mem *mem)
{
  struct holy_script *parsed;

  parsed = holy_malloc (sizeof (*parsed));
  if (! parsed)
    return 0;

  parsed->mem = mem;
  parsed->cmd = cmd;
  parsed->refcnt = 0;
  parsed->children = 0;
  parsed->next_siblings = 0;

  return parsed;
}

/* Parse the script passed in SCRIPT and return the parsed
   datastructure that is ready to be interpreted.  */
struct holy_script *
holy_script_parse (char *script,
		   holy_reader_getline_t getline, void *getline_data)
{
  struct holy_script *parsed;
  struct holy_script_mem *membackup;
  struct holy_lexer_param *lexstate;
  struct holy_parser_param *parsestate;

  parsed = holy_script_create (0, 0);
  if (!parsed)
    return 0;

  parsestate = holy_zalloc (sizeof (*parsestate));
  if (!parsestate)
    {
      holy_free (parsed);
      return 0;
    }

  /* Initialize the lexer.  */
  lexstate = holy_script_lexer_init (parsestate, script,
				     getline, getline_data);
  if (!lexstate)
    {
      holy_free (parsed);
      holy_free (parsestate);
      return 0;
    }

  parsestate->lexerstate = lexstate;

  membackup = holy_script_mem_record (parsestate);

  /* Parse the script.  */
  if (holy_script_yyparse (parsestate) || parsestate->err)
    {
      struct holy_script_mem *memfree;
      memfree = holy_script_mem_record_stop (parsestate, membackup);
      holy_script_mem_free (memfree);
      holy_script_lexer_fini (lexstate);
      holy_free (parsestate);
      holy_free (parsed);
      return 0;
    }

  parsed->mem = holy_script_mem_record_stop (parsestate, membackup);
  parsed->cmd = parsestate->parsed;
  parsed->children = parsestate->scripts;

  holy_script_lexer_fini (lexstate);
  holy_free (parsestate);

  return parsed;
}
