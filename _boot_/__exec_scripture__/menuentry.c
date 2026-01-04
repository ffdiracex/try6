/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>
#include <holy/normal.h>

static const struct holy_arg_option options[] =
  {
    {"class", 1, holy_ARG_OPTION_REPEATABLE,
     N_("Menu entry type."), N_("STRING"), ARG_TYPE_STRING},
    {"users", 2, 0,
     N_("List of users allowed to boot this entry."), N_("USERNAME[,USERNAME]"),
     ARG_TYPE_STRING},
    {"hotkey", 3, 0,
     N_("Keyboard key to quickly boot this entry."), N_("KEYBOARD_KEY"), ARG_TYPE_STRING},
    {"source", 4, 0,
     N_("Use STRING as menu entry body."), N_("STRING"), ARG_TYPE_STRING},
    {"id", 0, 0, N_("Menu entry identifier."), N_("STRING"), ARG_TYPE_STRING},
    /* TRANSLATORS: menu entry can either be bootable by anyone or only by
       handful of users. By default when security is active only superusers can
       boot a given menu entry. With --unrestricted (this option)
       anyone can boot it.  */
    {"unrestricted", 0, 0, N_("This entry can be booted by any user."),
     0, ARG_TYPE_NONE},
    {0, 0, 0, 0, 0, 0}
  };

static struct
{
  const char *name;
  int key;
} hotkey_aliases[] =
  {
    {"backspace", '\b'},
    {"tab", '\t'},
    {"delete", holy_TERM_KEY_DC},
    {"insert", holy_TERM_KEY_INSERT},
    {"f1", holy_TERM_KEY_F1},
    {"f2", holy_TERM_KEY_F2},
    {"f3", holy_TERM_KEY_F3},
    {"f4", holy_TERM_KEY_F4},
    {"f5", holy_TERM_KEY_F5},
    {"f6", holy_TERM_KEY_F6},
    {"f7", holy_TERM_KEY_F7},
    {"f8", holy_TERM_KEY_F8},
    {"f9", holy_TERM_KEY_F9},
    {"f10", holy_TERM_KEY_F10},
    {"f11", holy_TERM_KEY_F11},
    {"f12", holy_TERM_KEY_F12},
  };

/* Add a menu entry to the current menu context (as given by the environment
   variable data slot `menu').  As the configuration file is read, the script
   parser calls this when a menu entry is to be created.  */
holy_err_t
holy_normal_add_menu_entry (int argc, const char **args,
			    char **classes, const char *id,
			    const char *users, const char *hotkey,
			    const char *prefix, const char *sourcecode,
			    int submenu)
{
  int menu_hotkey = 0;
  char **menu_args = NULL;
  char *menu_users = NULL;
  char *menu_title = NULL;
  char *menu_sourcecode = NULL;
  char *menu_id = NULL;
  struct holy_menu_entry_class *menu_classes = NULL;

  holy_menu_t menu;
  holy_menu_entry_t *last;

  menu = holy_env_get_menu ();
  if (! menu)
    return holy_error (holy_ERR_MENU, "no menu context");

  last = &menu->entry_list;

  menu_sourcecode = holy_xasprintf ("%s%s", prefix ?: "", sourcecode);
  if (! menu_sourcecode)
    return holy_errno;

  if (classes && classes[0])
    {
      int i;
      for (i = 0; classes[i]; i++); /* count # of menuentry classes */
      menu_classes = holy_zalloc (sizeof (struct holy_menu_entry_class)
				  * (i + 1));
      if (! menu_classes)
	goto fail;

      for (i = 0; classes[i]; i++)
	{
	  menu_classes[i].name = holy_strdup (classes[i]);
	  if (! menu_classes[i].name)
	    goto fail;
	  menu_classes[i].next = classes[i + 1] ? &menu_classes[i + 1] : NULL;
	}
    }

  if (users)
    {
      menu_users = holy_strdup (users);
      if (! menu_users)
	goto fail;
    }

  if (hotkey)
    {
      unsigned i;
      for (i = 0; i < ARRAY_SIZE (hotkey_aliases); i++)
	if (holy_strcmp (hotkey, hotkey_aliases[i].name) == 0)
	  {
	    menu_hotkey = hotkey_aliases[i].key;
	    break;
	  }
      if (i == ARRAY_SIZE (hotkey_aliases))
	menu_hotkey = hotkey[0];
    }

  if (! argc)
    {
      holy_error (holy_ERR_MENU, "menuentry is missing title");
      goto fail;
    }

  menu_title = holy_strdup (args[0]);
  if (! menu_title)
    goto fail;

  menu_id = holy_strdup (id ? : menu_title);
  if (! menu_id)
    goto fail;

  /* Save argc, args to pass as parameters to block arg later. */
  menu_args = holy_malloc (sizeof (char*) * (argc + 1));
  if (! menu_args)
    goto fail;

  {
    int i;
    for (i = 0; i < argc; i++)
      {
	menu_args[i] = holy_strdup (args[i]);
	if (! menu_args[i])
	  goto fail;
      }
    menu_args[argc] = NULL;
  }

  /* Add the menu entry at the end of the list.  */
  while (*last)
    last = &(*last)->next;

  *last = holy_zalloc (sizeof (**last));
  if (! *last)
    goto fail;

  (*last)->title = menu_title;
  (*last)->id = menu_id;
  (*last)->hotkey = menu_hotkey;
  (*last)->classes = menu_classes;
  if (menu_users)
    (*last)->restricted = 1;
  (*last)->users = menu_users;
  (*last)->argc = argc;
  (*last)->args = menu_args;
  (*last)->sourcecode = menu_sourcecode;
  (*last)->submenu = submenu;

  menu->size++;
  return holy_ERR_NONE;

 fail:

  holy_free (menu_sourcecode);
  {
    int i;
    for (i = 0; menu_classes && menu_classes[i].name; i++)
      holy_free (menu_classes[i].name);
    holy_free (menu_classes);
  }

  {
    int i;
    for (i = 0; menu_args && menu_args[i]; i++)
      holy_free (menu_args[i]);
    holy_free (menu_args);
  }

  holy_free (menu_users);
  holy_free (menu_title);
  holy_free (menu_id);
  return holy_errno;
}

static char *
setparams_prefix (int argc, char **args)
{
  int i;
  int j;
  char *p;
  char *result;
  holy_size_t len = 10;

  /* Count resulting string length */
  for (i = 0; i < argc; i++)
    {
      len += 3; /* 3 = 1 space + 2 quotes */
      p = args[i];
      while (*p)
	len += (*p++ == '\'' ? 3 : 1);
    }

  result = holy_malloc (len + 2);
  if (! result)
    return 0;

  holy_strcpy (result, "setparams");
  p = result + 9;

  for (j = 0; j < argc; j++)
    {
      *p++ = ' ';
      *p++ = '\'';
      p = holy_strchrsub (p, args[j], '\'', "'\\''");
      *p++ = '\'';
    }
  *p++ = '\n';
  *p = '\0';
  return result;
}

static holy_err_t
holy_cmd_menuentry (holy_extcmd_context_t ctxt, int argc, char **args)
{
  char ch;
  char *src;
  char *prefix;
  unsigned len;
  holy_err_t r;
  const char *users;

  if (! argc)
    return holy_error (holy_ERR_BAD_ARGUMENT, "missing arguments");

  if (ctxt->state[3].set && ctxt->script)
    return holy_error (holy_ERR_BAD_ARGUMENT, "multiple menuentry definitions");

  if (! ctxt->state[3].set && ! ctxt->script)
    return holy_error (holy_ERR_BAD_ARGUMENT, "no menuentry definition");

  if (ctxt->state[1].set)
    users = ctxt->state[1].arg;
  else if (ctxt->state[5].set)
    users = NULL;
  else
    users = "";

  if (! ctxt->script)
    return holy_normal_add_menu_entry (argc, (const char **) args,
				       (ctxt->state[0].set ? ctxt->state[0].args
					: NULL),
				       ctxt->state[4].arg,
				       users,
				       ctxt->state[2].arg, 0,
				       ctxt->state[3].arg,
				       ctxt->extcmd->cmd->name[0] == 's');

  src = args[argc - 1];
  args[argc - 1] = NULL;

  len = holy_strlen(src);
  ch = src[len - 1];
  src[len - 1] = '\0';

  prefix = setparams_prefix (argc - 1, args);
  if (! prefix)
    return holy_errno;

  r = holy_normal_add_menu_entry (argc - 1, (const char **) args,
				  ctxt->state[0].args, ctxt->state[4].arg,
				  users,
				  ctxt->state[2].arg, prefix, src + 1,
				  ctxt->extcmd->cmd->name[0] == 's');

  src[len - 1] = ch;
  args[argc - 1] = src;
  holy_free (prefix);
  return r;
}

static holy_extcmd_t cmd, cmd_sub;

void
holy_menu_init (void)
{
  cmd = holy_register_extcmd ("menuentry", holy_cmd_menuentry,
			      holy_COMMAND_FLAG_BLOCKS
			      | holy_COMMAND_ACCEPT_DASH
			      | holy_COMMAND_FLAG_EXTRACTOR,
			      N_("BLOCK"), N_("Define a menu entry."), options);
  cmd_sub = holy_register_extcmd ("submenu", holy_cmd_menuentry,
				  holy_COMMAND_FLAG_BLOCKS
				  | holy_COMMAND_ACCEPT_DASH
				  | holy_COMMAND_FLAG_EXTRACTOR,
				  N_("BLOCK"), N_("Define a submenu."),
				  options);
}

void
holy_menu_fini (void)
{
  holy_unregister_extcmd (cmd);
  holy_unregister_extcmd (cmd_sub);
}
