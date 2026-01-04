/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/dl.h>
#include <holy/command.h>
#include <holy/term.h>
#include <holy/i18n.h>
#include <holy/misc.h>

holy_MOD_LICENSE ("GPLv2+");

struct holy_term_autoload *holy_term_input_autoload = NULL;
struct holy_term_autoload *holy_term_output_autoload = NULL;

struct abstract_terminal
{
  struct abstract_terminal *next;
  struct abstract_terminal *prev;
  const char *name;
  holy_err_t (*init) (struct abstract_terminal *term);
  holy_err_t (*fini) (struct abstract_terminal *term);
};

static holy_err_t
handle_command (int argc, char **args, struct abstract_terminal **enabled,
               struct abstract_terminal **disabled,
               struct holy_term_autoload *autoloads,
               const char *active_str,
               const char *available_str)
{
  int i;
  struct abstract_terminal *term;
  struct holy_term_autoload *aut;

  if (argc == 0)
    {
      holy_puts_ (active_str);
      for (term = *enabled; term; term = term->next)
       holy_printf ("%s ", term->name);
      holy_printf ("\n");
      holy_puts_ (available_str);
      for (term = *disabled; term; term = term->next)
       holy_printf ("%s ", term->name);
      /* This is quadratic but we don't expect mode than 30 terminal
        modules ever.  */
      for (aut = autoloads; aut; aut = aut->next)
       {
         for (term = *disabled; term; term = term->next)
           if (holy_strcmp (term->name, aut->name) == 0
	       || (aut->name[0] && aut->name[holy_strlen (aut->name) - 1] == '*'
		   && holy_memcmp (term->name, aut->name,
				   holy_strlen (aut->name) - 1) == 0))
             break;
         if (!term)
           for (term = *enabled; term; term = term->next)
             if (holy_strcmp (term->name, aut->name) == 0
		 || (aut->name[0] && aut->name[holy_strlen (aut->name) - 1] == '*'
		     && holy_memcmp (term->name, aut->name,
				     holy_strlen (aut->name) - 1) == 0))
               break;
         if (!term)
           holy_printf ("%s ", aut->name);
       }
      holy_printf ("\n");
      return holy_ERR_NONE;
    }
  i = 0;

  if (holy_strcmp (args[0], "--append") == 0
      || holy_strcmp (args[0], "--remove") == 0)
    i++;

  if (i == argc)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_ ("no terminal specified"));

  for (; i < argc; i++)
    {
      int again = 0;
      while (1)
       {
         for (term = *disabled; term; term = term->next)
           if (holy_strcmp (args[i], term->name) == 0
	       || (holy_strcmp (args[i], "ofconsole") == 0
		   && holy_strcmp ("console", term->name) == 0))
             break;
         if (term == 0)
           for (term = *enabled; term; term = term->next)
             if (holy_strcmp (args[i], term->name) == 0
		 || (holy_strcmp (args[i], "ofconsole") == 0
		     && holy_strcmp ("console", term->name) == 0))
               break;
         if (term)
           break;
         if (again)
	   return holy_error (holy_ERR_BAD_ARGUMENT,
			      N_("terminal `%s' isn't found"),
			      args[i]);
         for (aut = autoloads; aut; aut = aut->next)
           if (holy_strcmp (args[i], aut->name) == 0
	       || (holy_strcmp (args[i], "ofconsole") == 0
		   && holy_strcmp ("console", aut->name) == 0)
	       || (aut->name[0] && aut->name[holy_strlen (aut->name) - 1] == '*'
		   && holy_memcmp (args[i], aut->name,
				   holy_strlen (aut->name) - 1) == 0))
             {
               holy_dl_t mod;
               mod = holy_dl_load (aut->modname);
               if (mod)
                 holy_dl_ref (mod);
               holy_errno = holy_ERR_NONE;
               break;
             }
	 if (holy_memcmp (args[i], "serial_usb",
				  sizeof ("serial_usb") - 1) == 0
	     && holy_term_poll_usb)
	   {
	     holy_term_poll_usb (1);
	     again = 1;
	     continue;
	   }
         if (!aut)
           return holy_error (holy_ERR_BAD_ARGUMENT,
			      N_("terminal `%s' isn't found"),
                              args[i]);
         again = 1;
       }
    }

  if (holy_strcmp (args[0], "--append") == 0)
    {
      for (i = 1; i < argc; i++)
       {
         for (term = *disabled; term; term = term->next)
           if (holy_strcmp (args[i], term->name) == 0
	       || (holy_strcmp (args[i], "ofconsole") == 0
		   && holy_strcmp ("console", term->name) == 0))
             break;
         if (term)
           {
              if (term->init && term->init (term) != holy_ERR_NONE)
                return holy_errno;

             holy_list_remove (holy_AS_LIST (term));
             holy_list_push (holy_AS_LIST_P (enabled), holy_AS_LIST (term));
           }
       }
      return holy_ERR_NONE;
    }

  if (holy_strcmp (args[0], "--remove") == 0)
    {
      for (i = 1; i < argc; i++)
       {
         for (term = *enabled; term; term = term->next)
           if (holy_strcmp (args[i], term->name) == 0
	       || (holy_strcmp (args[i], "ofconsole") == 0
		   && holy_strcmp ("console", term->name) == 0))
             break;
         if (term)
           {
             if (!term->next && term == *enabled)
               return holy_error (holy_ERR_BAD_ARGUMENT,
                                  "can't remove the last terminal");
             holy_list_remove (holy_AS_LIST (term));
             if (term->fini)
               term->fini (term);
             holy_list_push (holy_AS_LIST_P (disabled), holy_AS_LIST (term));
           }
       }
      return holy_ERR_NONE;
    }
  for (i = 0; i < argc; i++)
    {
      for (term = *disabled; term; term = term->next)
       if (holy_strcmp (args[i], term->name) == 0
	   || (holy_strcmp (args[i], "ofconsole") == 0
	       && holy_strcmp ("console", term->name) == 0))
         break;
      if (term)
       {
         if (term->init && term->init (term) != holy_ERR_NONE)
           return holy_errno;

         holy_list_remove (holy_AS_LIST (term));
         holy_list_push (holy_AS_LIST_P (enabled), holy_AS_LIST (term));
       }       
    }
  
  {
    struct abstract_terminal *next;
    for (term = *enabled; term; term = next)
      {
       next = term->next;
       for (i = 0; i < argc; i++)
         if (holy_strcmp (args[i], term->name) == 0
	     || (holy_strcmp (args[i], "ofconsole") == 0
		 && holy_strcmp ("console", term->name) == 0))
           break;
       if (i == argc)
         {
           if (!term->next && term == *enabled)
             return holy_error (holy_ERR_BAD_ARGUMENT,
                                "can't remove the last terminal");
           holy_list_remove (holy_AS_LIST (term));
           if (term->fini)
             term->fini (term);
           holy_list_push (holy_AS_LIST_P (disabled), holy_AS_LIST (term));
         }
      }
  }

  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_terminal_input (holy_command_t cmd __attribute__ ((unused)),
			 int argc, char **args)
{
  (void) holy_FIELD_MATCH (holy_term_inputs, struct abstract_terminal *, next);
  (void) holy_FIELD_MATCH (holy_term_inputs, struct abstract_terminal *, prev);
  (void) holy_FIELD_MATCH (holy_term_inputs, struct abstract_terminal *, name);
  (void) holy_FIELD_MATCH (holy_term_inputs, struct abstract_terminal *, init);
  (void) holy_FIELD_MATCH (holy_term_inputs, struct abstract_terminal *, fini);
  return handle_command (argc, args,
			 (struct abstract_terminal **) (void *) &holy_term_inputs,
			 (struct abstract_terminal **) (void *) &holy_term_inputs_disabled,
			 holy_term_input_autoload,
			 N_ ("Active input terminals:"),
			 N_ ("Available input terminals:"));
}

static holy_err_t
holy_cmd_terminal_output (holy_command_t cmd __attribute__ ((unused)),
                         int argc, char **args)
{
  (void) holy_FIELD_MATCH (holy_term_outputs, struct abstract_terminal *, next);
  (void) holy_FIELD_MATCH (holy_term_outputs, struct abstract_terminal *, prev);
  (void) holy_FIELD_MATCH (holy_term_outputs, struct abstract_terminal *, name);
  (void) holy_FIELD_MATCH (holy_term_outputs, struct abstract_terminal *, init);
  (void) holy_FIELD_MATCH (holy_term_outputs, struct abstract_terminal *, fini);
  return handle_command (argc, args,
			 (struct abstract_terminal **) (void *) &holy_term_outputs,
			 (struct abstract_terminal **) (void *) &holy_term_outputs_disabled,
			 holy_term_output_autoload,
			 N_ ("Active output terminals:"),
			 N_ ("Available output terminals:"));
}

static holy_command_t cmd_terminal_input, cmd_terminal_output;

holy_MOD_INIT(terminal)
{
  cmd_terminal_input =
    holy_register_command ("terminal_input", holy_cmd_terminal_input,
			   N_("[--append|--remove] "
			      "[TERMINAL1] [TERMINAL2] ..."),
			   N_("List or select an input terminal."));
  cmd_terminal_output =
    holy_register_command ("terminal_output", holy_cmd_terminal_output,
			   N_("[--append|--remove] "
			      "[TERMINAL1] [TERMINAL2] ..."),
			   N_("List or select an output terminal."));
}

holy_MOD_FINI(terminal)
{
  holy_unregister_command (cmd_terminal_input);
  holy_unregister_command (cmd_terminal_output);
}
