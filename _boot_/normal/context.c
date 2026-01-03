/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/env.h>
#include <holy/env_private.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/command.h>
#include <holy/normal.h>
#include <holy/i18n.h>

struct menu_pointer
{
  holy_menu_t menu;
  struct menu_pointer *prev;
};

static struct menu_pointer initial_menu;
static struct menu_pointer *current_menu = &initial_menu;

void
holy_env_unset_menu (void)
{
  current_menu->menu = NULL;
}

holy_menu_t
holy_env_get_menu (void)
{
  return current_menu->menu;
}

void
holy_env_set_menu (holy_menu_t nmenu)
{
  current_menu->menu = nmenu;
}

static holy_err_t
holy_env_new_context (int export_all)
{
  struct holy_env_context *context;
  int i;
  struct menu_pointer *menu;

  context = holy_zalloc (sizeof (*context));
  if (! context)
    return holy_errno;
  menu = holy_zalloc (sizeof (*menu));
  if (! menu)
    {
      holy_free (context);
      return holy_errno;
    }

  context->prev = holy_current_context;
  holy_current_context = context;

  menu->prev = current_menu;
  current_menu = menu;

  /* Copy exported variables.  */
  for (i = 0; i < HASHSZ; i++)
    {
      struct holy_env_var *var;

      for (var = context->prev->vars[i]; var; var = var->next)
	if (var->global || export_all)
	  {
	    if (holy_env_set (var->name, var->value) != holy_ERR_NONE)
	      {
		holy_env_context_close ();
		return holy_errno;
	      }
	    holy_env_export (var->name);
	    holy_register_variable_hook (var->name, var->read_hook, var->write_hook);
	  }
    }

  return holy_ERR_NONE;
}

holy_err_t
holy_env_context_open (void)
{
  return holy_env_new_context (0);
}

int holy_extractor_level = 0;

holy_err_t
holy_env_extractor_open (int source)
{
  holy_extractor_level++;
  return holy_env_new_context (source);
}

holy_err_t
holy_env_context_close (void)
{
  struct holy_env_context *context;
  int i;
  struct menu_pointer *menu;

  if (! holy_current_context->prev)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       "cannot close the initial context");

  /* Free the variables associated with this context.  */
  for (i = 0; i < HASHSZ; i++)
    {
      struct holy_env_var *p, *q;

      for (p = holy_current_context->vars[i]; p; p = q)
	{
	  q = p->next;
          holy_free (p->name);
	  holy_free (p->value);
	  holy_free (p);
	}
    }

  /* Restore the previous context.  */
  context = holy_current_context->prev;
  holy_free (holy_current_context);
  holy_current_context = context;

  menu = current_menu->prev;
  if (current_menu->menu)
    holy_normal_free_menu (current_menu->menu);
  holy_free (current_menu);
  current_menu = menu;

  return holy_ERR_NONE;
}

holy_err_t
holy_env_extractor_close (int source)
{
  holy_menu_t menu = NULL;
  holy_menu_entry_t *last;
  holy_err_t err;

  if (source)
    {
      menu = holy_env_get_menu ();
      holy_env_unset_menu ();
    }
  err = holy_env_context_close ();

  if (source && menu)
    {
      holy_menu_t menu2;
      menu2 = holy_env_get_menu ();
      
      last = &menu2->entry_list;
      while (*last)
	last = &(*last)->next;
      
      *last = menu->entry_list;
      menu2->size += menu->size;
    }

  holy_extractor_level--;
  return err;
}

static holy_command_t export_cmd;

static holy_err_t
holy_cmd_export (struct holy_command *cmd __attribute__ ((unused)),
		 int argc, char **args)
{
  int i;

  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("one argument expected"));

  for (i = 0; i < argc; i++)
    holy_env_export (args[i]);

  return 0;
}

void
holy_context_init (void)
{
  export_cmd = holy_register_command ("export", holy_cmd_export,
				      N_("ENVVAR [ENVVAR] ..."),
				      N_("Export variables."));
}

void
holy_context_fini (void)
{
  holy_unregister_command (export_cmd);
}
