/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/term.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>
#include <holy/mm.h>
#include <holy/normal.h>
#include <holy/charset.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t
holy_cmd_help (holy_extcmd_context_t ctxt __attribute__ ((unused)), int argc,
	       char **args)
{
  int cnt = 0;
  char *currarg;

  if (argc == 0)
    {
      holy_command_t cmd;
      FOR_COMMANDS(cmd)
      {
	if ((cmd->prio & holy_COMMAND_FLAG_ACTIVE))
	  {
	    struct holy_term_output *term;
	    const char *summary_translated = _(cmd->summary);
	    char *command_help;
	    holy_uint32_t *unicode_command_help;
	    holy_uint32_t *unicode_last_position;

	    command_help = holy_xasprintf ("%s %s", cmd->name, summary_translated);
	    if (!command_help)
	      break;

	    holy_utf8_to_ucs4_alloc (command_help, &unicode_command_help,
				     &unicode_last_position);

	    FOR_ACTIVE_TERM_OUTPUTS(term)
	    {
	      unsigned stringwidth;
	      holy_uint32_t *unicode_last_screen_position;

	      unicode_last_screen_position = unicode_command_help;

	      stringwidth = 0;

	      while (unicode_last_screen_position < unicode_last_position && 
		     stringwidth < ((holy_term_width (term) / 2) - 2))
		{
		  struct holy_unicode_glyph glyph;
		  unicode_last_screen_position 
		    += holy_unicode_aglomerate_comb (unicode_last_screen_position,
						     unicode_last_position
						     - unicode_last_screen_position,
						     &glyph);

		  stringwidth
		    += holy_term_getcharwidth (term, &glyph);
		}

	      holy_print_ucs4 (unicode_command_help,
			       unicode_last_screen_position, 0, 0, term);
	      if (!(cnt % 2))
		holy_print_spaces (term, holy_term_width (term) / 2
				   - stringwidth);
	    }

	    if (cnt % 2)
	      holy_printf ("\n");
	    cnt++;
	  
	    holy_free (command_help);
	    holy_free (unicode_command_help);
	  }
      }
      if (!(cnt % 2))
	holy_printf ("\n");
    }
  else
    {
      int i;
      holy_command_t cmd_iter, cmd, cmd_next;

      for (i = 0; i < argc; i++)
	{
	  currarg = args[i];

	  FOR_COMMANDS_SAFE (cmd_iter, cmd_next)
	  {
	    if (!(cmd_iter->prio & holy_COMMAND_FLAG_ACTIVE))
	      continue;

	    if (holy_strncmp (cmd_iter->name, currarg,
			      holy_strlen (currarg)) != 0)
	      continue;
	    if (cmd_iter->flags & holy_COMMAND_FLAG_DYNCMD)
	      cmd = holy_dyncmd_get_cmd (cmd_iter);
	    else
	      cmd = cmd_iter;
	    if (!cmd)
	      {
		holy_print_error ();
		continue;
	      }
	    if (cnt++ > 0)
	      holy_printf ("\n\n");

	    if ((cmd->flags & holy_COMMAND_FLAG_EXTCMD) &&
		! (cmd->flags & holy_COMMAND_FLAG_DYNCMD))
	      holy_arg_show_help ((holy_extcmd_t) cmd->data);
	    else
	      holy_printf ("%s %s %s\n%s\n", _("Usage:"), cmd->name,
			   _(cmd->summary), _(cmd->description));
	  }
	}
    }

  return 0;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(help)
{
  cmd = holy_register_extcmd ("help", holy_cmd_help, 0,
			      N_("[PATTERN ...]"),
			      N_("Show a help message."), 0);
}

holy_MOD_FINI(help)
{
  holy_unregister_extcmd (cmd);
}
