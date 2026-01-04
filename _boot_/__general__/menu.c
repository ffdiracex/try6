/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/normal.h>
#include <holy/misc.h>
#include <holy/loader.h>
#include <holy/mm.h>
#include <holy/time.h>
#include <holy/env.h>
#include <holy/menu_viewer.h>
#include <holy/command.h>
#include <holy/parser.h>
#include <holy/auth.h>
#include <holy/i18n.h>
#include <holy/term.h>
#include <holy/script_sh.h>
#include <holy/gfxterm.h>
#include <holy/dl.h>

/* Time to delay after displaying an error message about a default/fallback
   entry failing to boot.  */
#define DEFAULT_ENTRY_ERROR_DELAY_MS  2500

holy_err_t (*holy_gfxmenu_try_hook) (int entry, holy_menu_t menu,
				     int nested) = NULL;

enum timeout_style {
  TIMEOUT_STYLE_MENU,
  TIMEOUT_STYLE_COUNTDOWN,
  TIMEOUT_STYLE_HIDDEN
};

struct timeout_style_name {
  const char *name;
  enum timeout_style style;
} timeout_style_names[] = {
  {"menu", TIMEOUT_STYLE_MENU},
  {"countdown", TIMEOUT_STYLE_COUNTDOWN},
  {"hidden", TIMEOUT_STYLE_HIDDEN},
  {NULL, 0}
};

/* Wait until the user pushes any key so that the user
   can see what happened.  */
void
holy_wait_after_message (void)
{
  holy_uint64_t endtime;
  holy_xputs ("\n");
  holy_printf_ (N_("Press any key to continue..."));
  holy_refresh ();

  endtime = holy_get_time_ms () + 10000;

  while (holy_get_time_ms () < endtime
	 && holy_getkey_noblock () == holy_TERM_NO_KEY);

  holy_xputs ("\n");
}

/* Get a menu entry by its index in the entry list.  */
holy_menu_entry_t
holy_menu_get_entry (holy_menu_t menu, int no)
{
  holy_menu_entry_t e;

  for (e = menu->entry_list; e && no > 0; e = e->next, no--)
    ;

  return e;
}

/* Get the index of a menu entry associated with a given hotkey, or -1.  */
static int
get_entry_index_by_hotkey (holy_menu_t menu, int hotkey)
{
  holy_menu_entry_t entry;
  int i;

  for (i = 0, entry = menu->entry_list; i < menu->size;
       i++, entry = entry->next)
    if (entry->hotkey == hotkey)
      return i;

  return -1;
}

/* Return the timeout style.  If the variable "timeout_style" is not set or
   invalid, default to TIMEOUT_STYLE_MENU.  */
static enum timeout_style
get_timeout_style (void)
{
  const char *val;
  struct timeout_style_name *style_name;

  val = holy_env_get ("timeout_style");
  if (!val)
    return TIMEOUT_STYLE_MENU;

  for (style_name = timeout_style_names; style_name->name; style_name++)
    if (holy_strcmp (style_name->name, val) == 0)
      return style_name->style;

  return TIMEOUT_STYLE_MENU;
}

/* Return the current timeout. If the variable "timeout" is not set or
   invalid, return -1.  */
int
holy_menu_get_timeout (void)
{
  const char *val;
  int timeout;

  val = holy_env_get ("timeout");
  if (! val)
    return -1;

  holy_error_push ();

  timeout = (int) holy_strtoul (val, 0, 0);

  /* If the value is invalid, unset the variable.  */
  if (holy_errno != holy_ERR_NONE)
    {
      holy_env_unset ("timeout");
      holy_errno = holy_ERR_NONE;
      timeout = -1;
    }

  holy_error_pop ();

  return timeout;
}

/* Set current timeout in the variable "timeout".  */
void
holy_menu_set_timeout (int timeout)
{
  /* Ignore TIMEOUT if it is zero, because it will be unset really soon.  */
  if (timeout > 0)
    {
      char buf[16];

      holy_snprintf (buf, sizeof (buf), "%d", timeout);
      holy_env_set ("timeout", buf);
    }
}

/* Get the first entry number from the value of the environment variable NAME,
   which is a space-separated list of non-negative integers.  The entry number
   which is returned is stripped from the value of NAME.  If no entry number
   can be found, -1 is returned.  */
static int
get_and_remove_first_entry_number (const char *name)
{
  const char *val;
  char *tail;
  int entry;

  val = holy_env_get (name);
  if (! val)
    return -1;

  holy_error_push ();

  entry = (int) holy_strtoul (val, &tail, 0);

  if (holy_errno == holy_ERR_NONE)
    {
      /* Skip whitespace to find the next digit.  */
      while (*tail && holy_isspace (*tail))
	tail++;
      holy_env_set (name, tail);
    }
  else
    {
      holy_env_unset (name);
      holy_errno = holy_ERR_NONE;
      entry = -1;
    }

  holy_error_pop ();

  return entry;
}

/* Run a menu entry.  */
static void
holy_menu_execute_entry(holy_menu_entry_t entry, int auto_boot)
{
  holy_err_t err = holy_ERR_NONE;
  int errs_before;
  holy_menu_t menu = NULL;
  char *optr, *buf, *oldchosen = NULL, *olddefault = NULL;
  const char *ptr, *chosen, *def;
  holy_size_t sz = 0;

  if (entry->restricted)
    err = holy_auth_check_authentication (entry->users);

  if (err)
    {
      holy_print_error ();
      holy_errno = holy_ERR_NONE;
      return;
    }

  errs_before = holy_err_printed_errors;

  chosen = holy_env_get ("chosen");
  def = holy_env_get ("default");

  if (entry->submenu)
    {
      holy_env_context_open ();
      menu = holy_zalloc (sizeof (*menu));
      if (! menu)
	return;
      holy_env_set_menu (menu);
      if (auto_boot)
	holy_env_set ("timeout", "0");
    }

  for (ptr = entry->id; *ptr; ptr++)
    sz += (*ptr == '>') ? 2 : 1;
  if (chosen)
    {
      oldchosen = holy_strdup (chosen);
      if (!oldchosen)
	holy_print_error ();
    }
  if (def)
    {
      olddefault = holy_strdup (def);
      if (!olddefault)
	holy_print_error ();
    }
  sz++;
  if (chosen)
    sz += holy_strlen (chosen);
  sz++;
  buf = holy_malloc (sz);
  if (!buf)
    holy_print_error ();
  else
    {
      optr = buf;
      if (chosen)
	{
	  optr = holy_stpcpy (optr, chosen);
	  *optr++ = '>';
	}
      for (ptr = entry->id; *ptr; ptr++)
	{
	  if (*ptr == '>')
	    *optr++ = '>';
	  *optr++ = *ptr;
	}
      *optr = 0;
      holy_env_set ("chosen", buf);
      holy_env_export ("chosen");
      holy_free (buf);
    }

  for (ptr = def; ptr && *ptr; ptr++)
    {
      if (ptr[0] == '>' && ptr[1] == '>')
	{
	  ptr++;
	  continue;
	}
      if (ptr[0] == '>')
	break;
    }

  if (ptr && ptr[0] && ptr[1])
    holy_env_set ("default", ptr + 1);
  else
    holy_env_unset ("default");

  holy_script_execute_new_scope (entry->sourcecode, entry->argc, entry->args);

  if (errs_before != holy_err_printed_errors)
    holy_wait_after_message ();

  errs_before = holy_err_printed_errors;

  if (holy_errno == holy_ERR_NONE && holy_loader_is_loaded ())
    /* Implicit execution of boot, only if something is loaded.  */
    holy_command_execute ("boot", 0, 0);

  if (errs_before != holy_err_printed_errors)
    holy_wait_after_message ();

  if (entry->submenu)
    {
      if (menu && menu->size)
	{
	  holy_show_menu (menu, 1, auto_boot);
	  holy_normal_free_menu (menu);
	}
      holy_env_context_close ();
    }
  if (oldchosen)
    holy_env_set ("chosen", oldchosen);
  else
    holy_env_unset ("chosen");
  if (olddefault)
    holy_env_set ("default", olddefault);
  else
    holy_env_unset ("default");
  holy_env_unset ("timeout");
}

/* Execute ENTRY from the menu MENU, falling back to entries specified
   in the environment variable "fallback" if it fails.  CALLBACK is a
   pointer to a struct of function pointers which are used to allow the
   caller provide feedback to the user.  */
static void
holy_menu_execute_with_fallback (holy_menu_t menu,
				 holy_menu_entry_t entry,
				 int autobooted,
				 holy_menu_execute_callback_t callback,
				 void *callback_data)
{
  int fallback_entry;

  callback->notify_booting (entry, callback_data);

  holy_menu_execute_entry (entry, 1);

  /* Deal with fallback entries.  */
  while ((fallback_entry = get_and_remove_first_entry_number ("fallback"))
	 >= 0)
    {
      holy_print_error ();
      holy_errno = holy_ERR_NONE;

      entry = holy_menu_get_entry (menu, fallback_entry);
      callback->notify_fallback (entry, callback_data);
      holy_menu_execute_entry (entry, 1);
      /* If the function call to execute the entry returns at all, then this is
	 taken to indicate a boot failure.  For menu entries that do something
	 other than actually boot an operating system, this could assume
	 incorrectly that something failed.  */
    }

  if (!autobooted)
    callback->notify_failure (callback_data);
}

static struct holy_menu_viewer *viewers;

static void
menu_set_chosen_entry (int entry)
{
  struct holy_menu_viewer *cur;
  for (cur = viewers; cur; cur = cur->next)
    cur->set_chosen_entry (entry, cur->data);
}

static void
menu_print_timeout (int timeout)
{
  struct holy_menu_viewer *cur;
  for (cur = viewers; cur; cur = cur->next)
    cur->print_timeout (timeout, cur->data);
}

static void
menu_fini (void)
{
  struct holy_menu_viewer *cur, *next;
  for (cur = viewers; cur; cur = next)
    {
      next = cur->next;
      cur->fini (cur->data);
      holy_free (cur);
    }
  viewers = NULL;
}

static void
menu_init (int entry, holy_menu_t menu, int nested)
{
  struct holy_term_output *term;
  int gfxmenu = 0;

  FOR_ACTIVE_TERM_OUTPUTS(term)
    if (term->fullscreen)
      {
	if (holy_env_get ("theme"))
	  {
	    if (!holy_gfxmenu_try_hook)
	      {
		holy_dl_load ("gfxmenu");
		holy_print_error ();
	      }
	    if (holy_gfxmenu_try_hook)
	      {
		holy_err_t err;
		err = holy_gfxmenu_try_hook (entry, menu, nested);
		if(!err)
		  {
		    gfxmenu = 1;
		    break;
		  }
	      }
	    else
	      holy_error (holy_ERR_BAD_MODULE,
			  N_("module `%s' isn't loaded"),
			  "gfxmenu");
	    holy_print_error ();
	    holy_wait_after_message ();
	  }
	holy_errno = holy_ERR_NONE;
	term->fullscreen ();
	break;
      }

  FOR_ACTIVE_TERM_OUTPUTS(term)
  {
    holy_err_t err;

    if (holy_strcmp (term->name, "gfxterm") == 0 && gfxmenu)
      continue;

    err = holy_menu_try_text (term, entry, menu, nested);
    if(!err)
      continue;
    holy_print_error ();
    holy_errno = holy_ERR_NONE;
  }
}

static void
clear_timeout (void)
{
  struct holy_menu_viewer *cur;
  for (cur = viewers; cur; cur = cur->next)
    cur->clear_timeout (cur->data);
}

void
holy_menu_register_viewer (struct holy_menu_viewer *viewer)
{
  viewer->next = viewers;
  viewers = viewer;
}

static int
menuentry_eq (const char *id, const char *spec)
{
  const char *ptr1, *ptr2;
  ptr1 = id;
  ptr2 = spec;
  while (1)
    {
      if (*ptr2 == '>' && ptr2[1] != '>' && *ptr1 == 0)
	return 1;
      if (*ptr2 == '>' && ptr2[1] != '>')
	return 0;
      if (*ptr2 == '>')
	ptr2++;
      if (*ptr1 != *ptr2)
	return 0;
      if (*ptr1 == 0)
	return 1;
      ptr1++;
      ptr2++;
    }
}


/* Get the entry number from the variable NAME.  */
static int
get_entry_number (holy_menu_t menu, const char *name)
{
  const char *val;
  int entry;

  val = holy_env_get (name);
  if (! val)
    return -1;

  holy_error_push ();

  entry = (int) holy_strtoul (val, 0, 0);

  if (holy_errno == holy_ERR_BAD_NUMBER)
    {
      /* See if the variable matches the title of a menu entry.  */
      holy_menu_entry_t e = menu->entry_list;
      int i;

      holy_errno = holy_ERR_NONE;

      for (i = 0; e; i++)
	{
	  if (menuentry_eq (e->title, val)
	      || menuentry_eq (e->id, val))
	    {
	      entry = i;
	      break;
	    }
	  e = e->next;
	}

      if (! e)
	entry = -1;
    }

  if (holy_errno != holy_ERR_NONE)
    {
      holy_errno = holy_ERR_NONE;
      entry = -1;
    }

  holy_error_pop ();

  return entry;
}

/* Check whether a second has elapsed since the last tick.  If so, adjust
   the timer and return 1; otherwise, return 0.  */
static int
has_second_elapsed (holy_uint64_t *saved_time)
{
  holy_uint64_t current_time;

  current_time = holy_get_time_ms ();
  if (current_time - *saved_time >= 1000)
    {
      *saved_time = current_time;
      return 1;
    }
  else
    return 0;
}

static void
print_countdown (struct holy_term_coordinate *pos, int n)
{
  holy_term_restore_pos (pos);
  /* NOTE: Do not remove the trailing space characters.
     They are required to clear the line.  */
  holy_printf ("%d    ", n);
  holy_refresh ();
}

#define holy_MENU_PAGE_SIZE 10

/* Show the menu and handle menu entry selection.  Returns the menu entry
   index that should be executed or -1 if no entry should be executed (e.g.,
   Esc pressed to exit a sub-menu or switching menu viewers).
   If the return value is not -1, then *AUTO_BOOT is nonzero iff the menu
   entry to be executed is a result of an automatic default selection because
   of the timeout.  */
static int
run_menu (holy_menu_t menu, int nested, int *auto_boot)
{
  holy_uint64_t saved_time;
  int default_entry, current_entry;
  int timeout;
  enum timeout_style timeout_style;

  default_entry = get_entry_number (menu, "default");

  /* If DEFAULT_ENTRY is not within the menu entries, fall back to
     the first entry.  */
  if (default_entry < 0 || default_entry >= menu->size)
    default_entry = 0;

  timeout = holy_menu_get_timeout ();
  if (timeout < 0)
    /* If there is no timeout, the "countdown" and "hidden" styles result in
       the system doing nothing and providing no or very little indication
       why.  Technically this is what the user asked for, but it's not very
       useful and likely to be a source of confusion, so we disallow this.  */
    holy_env_unset ("timeout_style");

  timeout_style = get_timeout_style ();

  if (timeout_style == TIMEOUT_STYLE_COUNTDOWN
      || timeout_style == TIMEOUT_STYLE_HIDDEN)
    {
      static struct holy_term_coordinate *pos;
      int entry = -1;

      if (timeout_style == TIMEOUT_STYLE_COUNTDOWN && timeout)
	{
	  pos = holy_term_save_pos ();
	  print_countdown (pos, timeout);
	}

      /* Enter interruptible sleep until Escape or a menu hotkey is pressed,
         or the timeout expires.  */
      saved_time = holy_get_time_ms ();
      while (1)
	{
	  int key;

	  key = holy_getkey_noblock ();
	  if (key != holy_TERM_NO_KEY)
	    {
	      entry = get_entry_index_by_hotkey (menu, key);
	      if (entry >= 0)
		break;
	    }
	  if (key == holy_TERM_ESC)
	    {
	      timeout = -1;
	      break;
	    }

	  if (timeout > 0 && has_second_elapsed (&saved_time))
	    {
	      timeout--;
	      if (timeout_style == TIMEOUT_STYLE_COUNTDOWN)
		print_countdown (pos, timeout);
	    }

	  if (timeout == 0)
	    /* We will fall through to auto-booting the default entry.  */
	    break;
	}

      holy_env_unset ("timeout");
      holy_env_unset ("timeout_style");
      if (entry >= 0)
	{
	  *auto_boot = 0;
	  return entry;
	}
    }

  /* If timeout is 0, drawing is pointless (and ugly).  */
  if (timeout == 0)
    {
      *auto_boot = 1;
      return default_entry;
    }

  current_entry = default_entry;

 refresh:
  menu_init (current_entry, menu, nested);

  /* Initialize the time.  */
  saved_time = holy_get_time_ms ();

  timeout = holy_menu_get_timeout ();

  if (timeout > 0)
    menu_print_timeout (timeout);
  else
    clear_timeout ();

  while (1)
    {
      int c;
      timeout = holy_menu_get_timeout ();

      if (holy_normal_exit_level)
	return -1;

      if (timeout > 0 && has_second_elapsed (&saved_time))
	{
	  timeout--;
	  holy_menu_set_timeout (timeout);
	  menu_print_timeout (timeout);
	}

      if (timeout == 0)
	{
	  holy_env_unset ("timeout");
          *auto_boot = 1;
	  menu_fini ();
	  return default_entry;
	}

      c = holy_getkey_noblock ();

      if (c != holy_TERM_NO_KEY)
	{
	  if (timeout >= 0)
	    {
	      holy_env_unset ("timeout");
	      holy_env_unset ("fallback");
	      clear_timeout ();
	    }

	  switch (c)
	    {
	    case holy_TERM_KEY_HOME:
	    case holy_TERM_CTRL | 'a':
	      current_entry = 0;
	      menu_set_chosen_entry (current_entry);
	      break;

	    case holy_TERM_KEY_END:
	    case holy_TERM_CTRL | 'e':
	      current_entry = menu->size - 1;
	      menu_set_chosen_entry (current_entry);
	      break;

	    case holy_TERM_KEY_UP:
	    case holy_TERM_CTRL | 'p':
	    case '^':
	      if (current_entry > 0)
		current_entry--;
	      menu_set_chosen_entry (current_entry);
	      break;

	    case holy_TERM_CTRL | 'n':
	    case holy_TERM_KEY_DOWN:
	    case 'v':
	      if (current_entry < menu->size - 1)
		current_entry++;
	      menu_set_chosen_entry (current_entry);
	      break;

	    case holy_TERM_CTRL | 'g':
	    case holy_TERM_KEY_PPAGE:
	      if (current_entry < holy_MENU_PAGE_SIZE)
		current_entry = 0;
	      else
		current_entry -= holy_MENU_PAGE_SIZE;
	      menu_set_chosen_entry (current_entry);
	      break;

	    case holy_TERM_CTRL | 'c':
	    case holy_TERM_KEY_NPAGE:
	      if (current_entry + holy_MENU_PAGE_SIZE < menu->size)
		current_entry += holy_MENU_PAGE_SIZE;
	      else
		current_entry = menu->size - 1;
	      menu_set_chosen_entry (current_entry);
	      break;

	    case '\n':
	    case '\r':
	    case holy_TERM_KEY_RIGHT:
	    case holy_TERM_CTRL | 'f':
	      menu_fini ();
              *auto_boot = 0;
	      return current_entry;

	    case '\e':
	      if (nested)
		{
		  menu_fini ();
		  return -1;
		}
	      break;

	    case 'c':
	      menu_fini ();
	      holy_cmdline_run (1, 0);
	      goto refresh;

	    case 'e':
	      menu_fini ();
		{
		  holy_menu_entry_t e = holy_menu_get_entry (menu, current_entry);
		  if (e)
		    holy_menu_entry_run (e);
		}
	      goto refresh;

	    default:
	      {
		int entry;

		entry = get_entry_index_by_hotkey (menu, c);
		if (entry >= 0)
		  {
		    menu_fini ();
		    *auto_boot = 0;
		    return entry;
		  }
	      }
	      break;
	    }
	}
    }

  /* Never reach here.  */
}

/* Callback invoked immediately before a menu entry is executed.  */
static void
notify_booting (holy_menu_entry_t entry,
		void *userdata __attribute__((unused)))
{
  holy_printf ("  ");
  holy_printf_ (N_("Booting `%s'"), entry->title);
  holy_printf ("\n\n");
}

/* Callback invoked when a default menu entry executed because of a timeout
   has failed and an attempt will be made to execute the next fallback
   entry, ENTRY.  */
static void
notify_fallback (holy_menu_entry_t entry,
		 void *userdata __attribute__((unused)))
{
  holy_printf ("\n   ");
  holy_printf_ (N_("Falling back to `%s'"), entry->title);
  holy_printf ("\n\n");
  holy_millisleep (DEFAULT_ENTRY_ERROR_DELAY_MS);
}

/* Callback invoked when a menu entry has failed and there is no remaining
   fallback entry to attempt.  */
static void
notify_execution_failure (void *userdata __attribute__((unused)))
{
  if (holy_errno != holy_ERR_NONE)
    {
      holy_print_error ();
      holy_errno = holy_ERR_NONE;
    }
  holy_printf ("\n  ");
  holy_printf_ (N_("Failed to boot both default and fallback entries.\n"));
  holy_wait_after_message ();
}

/* Callbacks used by the text menu to provide user feedback when menu entries
   are executed.  */
static struct holy_menu_execute_callback execution_callback =
{
  .notify_booting = notify_booting,
  .notify_fallback = notify_fallback,
  .notify_failure = notify_execution_failure
};

static holy_err_t
show_menu (holy_menu_t menu, int nested, int autobooted)
{
  while (1)
    {
      int boot_entry;
      holy_menu_entry_t e;
      int auto_boot;

      boot_entry = run_menu (menu, nested, &auto_boot);
      if (boot_entry < 0)
	break;

      e = holy_menu_get_entry (menu, boot_entry);
      if (! e)
	continue; /* Menu is empty.  */

      holy_cls ();

      if (auto_boot)
	holy_menu_execute_with_fallback (menu, e, autobooted,
					 &execution_callback, 0);
      else
	holy_menu_execute_entry (e, 0);
      if (autobooted)
	break;
    }

  return holy_ERR_NONE;
}

holy_err_t
holy_show_menu (holy_menu_t menu, int nested, int autoboot)
{
  holy_err_t err1, err2;

  while (1)
    {
      err1 = show_menu (menu, nested, autoboot);
      autoboot = 0;
      holy_print_error ();

      if (holy_normal_exit_level)
	break;

      err2 = holy_auth_check_authentication (NULL);
      if (err2)
	{
	  holy_print_error ();
	  holy_errno = holy_ERR_NONE;
	  continue;
	}

      break;
    }

  return err1;
}
