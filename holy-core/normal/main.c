/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/kernel.h>
#include <holy/normal.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/file.h>
#include <holy/mm.h>
#include <holy/term.h>
#include <holy/env.h>
#include <holy/parser.h>
#include <holy/reader.h>
#include <holy/menu_viewer.h>
#include <holy/auth.h>
#include <holy/i18n.h>
#include <holy/charset.h>
#include <holy/script_sh.h>
#include <holy/bufio.h>

holy_MOD_LICENSE ("GPLv2+");

#define holy_DEFAULT_HISTORY_SIZE	50

static int nested_level = 0;
int holy_normal_exit_level = 0;

void
holy_normal_free_menu (holy_menu_t menu)
{
  holy_menu_entry_t entry = menu->entry_list;

  while (entry)
    {
      holy_menu_entry_t next_entry = entry->next;
      holy_size_t i;

      if (entry->classes)
	{
	  struct holy_menu_entry_class *class;
	  for (class = entry->classes; class; class = class->next)
	    holy_free (class->name);
	  holy_free (entry->classes);
	}

      if (entry->args)
	{
	  for (i = 0; entry->args[i]; i++)
	    holy_free (entry->args[i]);
	  holy_free (entry->args);
	}

      holy_free ((void *) entry->id);
      holy_free ((void *) entry->users);
      holy_free ((void *) entry->title);
      holy_free ((void *) entry->sourcecode);
      holy_free (entry);
      entry = next_entry;
    }

  holy_free (menu);
  holy_env_unset_menu ();
}

/* Helper for read_config_file.  */
static holy_err_t
read_config_file_getline (char **line, int cont __attribute__ ((unused)),
			  void *data)
{
  holy_file_t file = data;

  while (1)
    {
      char *buf;

      *line = buf = holy_file_getline (file);
      if (! buf)
	return holy_errno;

      if (buf[0] == '#')
	holy_free (*line);
      else
	break;
    }

  return holy_ERR_NONE;
}

static holy_menu_t
read_config_file (const char *config)
{
  holy_file_t rawfile, file;
  char *old_file = 0, *old_dir = 0;
  char *config_dir, *ptr = 0;
  const char *ctmp;

  holy_menu_t newmenu;

  newmenu = holy_env_get_menu ();
  if (! newmenu)
    {
      newmenu = holy_zalloc (sizeof (*newmenu));
      if (! newmenu)
	return 0;

      holy_env_set_menu (newmenu);
    }

  /* Try to open the config file.  */
  rawfile = holy_file_open (config);
  if (! rawfile)
    return 0;

  file = holy_bufio_open (rawfile, 0);
  if (! file)
    {
      holy_file_close (rawfile);
      return 0;
    }

  ctmp = holy_env_get ("config_file");
  if (ctmp)
    old_file = holy_strdup (ctmp);
  ctmp = holy_env_get ("config_directory");
  if (ctmp)
    old_dir = holy_strdup (ctmp);
  if (*config == '(')
    {
      holy_env_set ("config_file", config);
      config_dir = holy_strdup (config);
    }
  else
    {
      /* $root is guranteed to be defined, otherwise open above would fail */
      config_dir = holy_xasprintf ("(%s)%s", holy_env_get ("root"), config);
      if (config_dir)
	holy_env_set ("config_file", config_dir);
    }
  if (config_dir)
    {
      ptr = holy_strrchr (config_dir, '/');
      if (ptr)
	*ptr = 0;
      holy_env_set ("config_directory", config_dir);
      holy_free (config_dir);
    }

  holy_env_export ("config_file");
  holy_env_export ("config_directory");

  while (1)
    {
      char *line;

      /* Print an error, if any.  */
      holy_print_error ();
      holy_errno = holy_ERR_NONE;

      if ((read_config_file_getline (&line, 0, file)) || (! line))
	break;

      holy_normal_parse_line (line, read_config_file_getline, file);
      holy_free (line);
    }

  if (old_file)
    holy_env_set ("config_file", old_file);
  else
    holy_env_unset ("config_file");
  if (old_dir)
    holy_env_set ("config_directory", old_dir);
  else
    holy_env_unset ("config_directory");
  holy_free (old_file);
  holy_free (old_dir);

  holy_file_close (file);

  return newmenu;
}

/* Initialize the screen.  */
void
holy_normal_init_page (struct holy_term_output *term,
		       int y)
{
  holy_ssize_t msg_len;
  int posx;
  char *msg_formatted;
  holy_uint32_t *unicode_msg;
  holy_uint32_t *last_position;
 
  holy_term_cls (term);

  msg_formatted = holy_xasprintf (_("GNU holy  version %s"), PACKAGE_VERSION);
  if (!msg_formatted)
    return;
 
  msg_len = holy_utf8_to_ucs4_alloc (msg_formatted,
  				     &unicode_msg, &last_position);
  holy_free (msg_formatted);
 
  if (msg_len < 0)
    {
      return;
    }

  posx = holy_getstringwidth (unicode_msg, last_position, term);
  posx = ((int) holy_term_width (term) - posx) / 2;
  if (posx < 0)
    posx = 0;
  holy_term_gotoxy (term, (struct holy_term_coordinate) { posx, y });

  holy_print_ucs4 (unicode_msg, last_position, 0, 0, term);
  holy_putcode ('\n', term);
  holy_putcode ('\n', term);
  holy_free (unicode_msg);
}

static void
read_lists (const char *val)
{
  if (! holy_no_modules)
    {
      read_command_list (val);
      read_fs_list (val);
      read_crypto_list (val);
      read_terminal_list (val);
    }
  holy_gettext_reread_prefix (val);
}

static char *
read_lists_hook (struct holy_env_var *var __attribute__ ((unused)),
		 const char *val)
{
  read_lists (val);
  return val ? holy_strdup (val) : NULL;
}

/* Read the config file CONFIG and execute the menu interface or
   the command line interface if BATCH is false.  */
void
holy_normal_execute (const char *config, int nested, int batch)
{
  holy_menu_t menu = 0;
  const char *prefix;

  if (! nested)
    {
      prefix = holy_env_get ("prefix");
      read_lists (prefix);
      holy_register_variable_hook ("prefix", NULL, read_lists_hook);
    }

  holy_boot_time ("Executing config file");

  if (config)
    {
      menu = read_config_file (config);

      /* Ignore any error.  */
      holy_errno = holy_ERR_NONE;
    }

  holy_boot_time ("Executed config file");

  if (! batch)
    {
      if (menu && menu->size)
	{

	  holy_boot_time ("Entering menu");
	  holy_show_menu (menu, nested, 0);
	  if (nested)
	    holy_normal_free_menu (menu);
	}
    }
}

/* This starts the normal mode.  */
void
holy_enter_normal_mode (const char *config)
{
  holy_boot_time ("Entering normal mode");
  nested_level++;
  holy_normal_execute (config, 0, 0);
  holy_boot_time ("Entering shell");
  holy_cmdline_run (0, 1);
  nested_level--;
  if (holy_normal_exit_level)
    holy_normal_exit_level--;
  holy_boot_time ("Exiting normal mode");
}

/* Enter normal mode from rescue mode.  */
static holy_err_t
holy_cmd_normal (struct holy_command *cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  if (argc == 0)
    {
      /* Guess the config filename. It is necessary to make CONFIG static,
	 so that it won't get broken by longjmp.  */
      char *config;
      const char *prefix;

      prefix = holy_env_get ("prefix");
      if (prefix)
	{
	  config = holy_xasprintf ("%s/holy.cfg", prefix);
	  if (! config)
	    goto quit;

	  holy_enter_normal_mode (config);
	  holy_free (config);
	}
      else
	holy_enter_normal_mode (0);
    }
  else
    holy_enter_normal_mode (argv[0]);

quit:
  return 0;
}

/* Exit from normal mode to rescue mode.  */
static holy_err_t
holy_cmd_normal_exit (struct holy_command *cmd __attribute__ ((unused)),
		      int argc __attribute__ ((unused)),
		      char *argv[] __attribute__ ((unused)))
{
  if (nested_level <= holy_normal_exit_level)
    return holy_error (holy_ERR_BAD_ARGUMENT, "not in normal environment");
  holy_normal_exit_level++;
  return holy_ERR_NONE;
}

static holy_err_t
holy_normal_reader_init (int nested)
{
  struct holy_term_output *term;
  const char *msg_esc = _("ESC at any time exits.");
  char *msg_formatted;

  msg_formatted = holy_xasprintf (_("Minimal BASH-like line editing is supported. For "
				    "the first word, TAB lists possible command completions. Anywhere "
				    "else TAB lists possible device or file completions. %s"),
				  nested ? msg_esc : "");
  if (!msg_formatted)
    return holy_errno;

  FOR_ACTIVE_TERM_OUTPUTS(term)
  {
    holy_normal_init_page (term, 1);
    holy_term_setcursor (term, 1);

    if (holy_term_width (term) > 3 + STANDARD_MARGIN + 20)
      holy_print_message_indented (msg_formatted, 3, STANDARD_MARGIN, term);
    else
      holy_print_message_indented (msg_formatted, 0, 0, term);
    holy_putcode ('\n', term);
    holy_putcode ('\n', term);
    holy_putcode ('\n', term);
  }
  holy_free (msg_formatted);
 
  return 0;
}

static holy_err_t
holy_normal_read_line_real (char **line, int cont, int nested)
{
  const char *prompt;

  if (cont)
    /* TRANSLATORS: it's command line prompt.  */
    prompt = _(">");
  else
    /* TRANSLATORS: it's command line prompt.  */
    prompt = _("holy>");

  if (!prompt)
    return holy_errno;

  while (1)
    {
      *line = holy_cmdline_get (prompt);
      if (*line)
	return 0;

      if (cont || nested)
	{
	  holy_free (*line);
	  *line = 0;
	  return holy_errno;
	}
    }
 
}

static holy_err_t
holy_normal_read_line (char **line, int cont,
		       void *data __attribute__ ((unused)))
{
  return holy_normal_read_line_real (line, cont, 0);
}

void
holy_cmdline_run (int nested, int force_auth)
{
  holy_err_t err = holy_ERR_NONE;

  do
    {
      err = holy_auth_check_authentication (NULL);
    }
  while (err && force_auth);

  if (err)
    {
      holy_print_error ();
      holy_errno = holy_ERR_NONE;
      return;
    }

  holy_normal_reader_init (nested);

  while (1)
    {
      char *line = NULL;

      if (holy_normal_exit_level)
	break;

      /* Print an error, if any.  */
      holy_print_error ();
      holy_errno = holy_ERR_NONE;

      holy_normal_read_line_real (&line, 0, nested);
      if (! line)
	break;

      holy_normal_parse_line (line, holy_normal_read_line, NULL);
      holy_free (line);
    }
}

static char *
holy_env_write_pager (struct holy_env_var *var __attribute__ ((unused)),
		      const char *val)
{
  holy_set_more ((*val == '1'));
  return holy_strdup (val);
}

/* clear */
static holy_err_t
holy_mini_cmd_clear (struct holy_command *cmd __attribute__ ((unused)),
		   int argc __attribute__ ((unused)),
		   char *argv[] __attribute__ ((unused)))
{
  holy_cls ();
  return 0;
}

static holy_command_t cmd_clear;

static void (*holy_xputs_saved) (const char *str);
static const char *features[] = {
  "feature_chainloader_bpb", "feature_ntldr", "feature_platform_search_hint",
  "feature_default_font_path", "feature_all_video_module",
  "feature_menuentry_id", "feature_menuentry_options", "feature_200_final",
  "feature_nativedisk_cmd", "feature_timeout_style"
};

holy_MOD_INIT(normal)
{
  unsigned i;

  holy_boot_time ("Preparing normal module");

  /* Previously many modules depended on gzio. Be nice to user and load it.  */
  holy_dl_load ("gzio");
  holy_errno = 0;

  holy_normal_auth_init ();
  holy_context_init ();
  holy_script_init ();
  holy_menu_init ();

  holy_xputs_saved = holy_xputs;
  holy_xputs = holy_xputs_normal;

  /* Normal mode shouldn't be unloaded.  */
  if (mod)
    holy_dl_ref (mod);

  cmd_clear =
    holy_register_command ("clear", holy_mini_cmd_clear,
			   0, N_("Clear the screen."));

  holy_set_history (holy_DEFAULT_HISTORY_SIZE);

  holy_register_variable_hook ("pager", 0, holy_env_write_pager);
  holy_env_export ("pager");

  /* Register a command "normal" for the rescue mode.  */
  holy_register_command ("normal", holy_cmd_normal,
			 0, N_("Enter normal mode."));
  holy_register_command ("normal_exit", holy_cmd_normal_exit,
			 0, N_("Exit from normal mode."));

  /* Reload terminal colors when these variables are written to.  */
  holy_register_variable_hook ("color_normal", NULL, holy_env_write_color_normal);
  holy_register_variable_hook ("color_highlight", NULL, holy_env_write_color_highlight);

  /* Preserve hooks after context changes.  */
  holy_env_export ("color_normal");
  holy_env_export ("color_highlight");

  /* Set default color names.  */
  holy_env_set ("color_normal", "light-gray/black");
  holy_env_set ("color_highlight", "black/light-gray");

  for (i = 0; i < ARRAY_SIZE (features); i++)
    {
      holy_env_set (features[i], "y");
      holy_env_export (features[i]);
    }
  holy_env_set ("holy_cpu", holy_TARGET_CPU);
  holy_env_export ("holy_cpu");
  holy_env_set ("holy_platform", holy_PLATFORM);
  holy_env_export ("holy_platform");

  holy_boot_time ("Normal module prepared");
}

holy_MOD_FINI(normal)
{
  holy_context_fini ();
  holy_script_fini ();
  holy_menu_fini ();
  holy_normal_auth_fini ();

  holy_xputs = holy_xputs_saved;

  holy_set_history (0);
  holy_register_variable_hook ("pager", 0, 0);
  holy_fs_autoload_hook = 0;
  holy_unregister_command (cmd_clear);
}
