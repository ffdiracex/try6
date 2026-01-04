/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>
#include <holy/term.h>

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options[] =
  {
    {0, 'n', 0, N_("Do not output the trailing newline."), 0, 0},
    {0, 'e', 0, N_("Enable interpretation of backslash escapes."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static holy_err_t
holy_cmd_echo (holy_extcmd_context_t ctxt, int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;
  int newline = 1;
  int i;

  /* Check if `-n' was used.  */
  if (state[0].set)
    newline = 0;

  for (i = 0; i < argc; i++)
    {
      char *arg = *args;
      /* Unescaping results in a string no longer than the original.  */
      char *unescaped = holy_malloc (holy_strlen (arg) + 1);
      char *p = unescaped;
      args++;

      if (!unescaped)
	return holy_errno;

      while (*arg)
	{
	  /* In case `-e' is used, parse backslashes.  */
	  if (*arg == '\\' && state[1].set)
	    {
	      arg++;
	      if (*arg == '\0')
		break;

	      switch (*arg)
		{
		case '\\':
		  *p++ = '\\';
		  break;

		case 'a':
		  *p++ = '\a';
		  break;

		case 'c':
		  newline = 0;
		  break;

		case 'f':
		  *p++ = '\f';
		  break;

		case 'n':
		  *p++ = '\n';
		  break;

		case 'r':
		  *p++ = '\r';
		  break;

		case 't':
		  *p++ = '\t';
		  break;

		case 'v':
		  *p++ = '\v';
		  break;
		}
	      arg++;
	      continue;
	    }

	  /* This was not an escaped character, or escaping is not
	     enabled.  */
	  *p++ = *arg;
	  arg++;
	}

      *p = '\0';
      holy_xputs (unescaped);
      holy_free (unescaped);

      /* If another argument follows, insert a space.  */
      if (i != argc - 1)
	holy_printf (" " );
    }

  if (newline)
    holy_printf ("\n");

  holy_refresh ();

  return 0;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(echo)
{
  cmd = holy_register_extcmd ("echo", holy_cmd_echo,
			      holy_COMMAND_ACCEPT_DASH
			      | holy_COMMAND_OPTIONS_AT_START,
			      N_("[-e|-n] STRING"), N_("Display a line of text."),
			      options);
}

holy_MOD_FINI(echo)
{
  holy_unregister_extcmd (cmd);
}
