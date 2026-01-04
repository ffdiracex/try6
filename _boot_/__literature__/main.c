/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/i18n.h>
#include <holy/parser.h>
#include <holy/script_sh.h>

holy_err_t
holy_normal_parse_line (char *line,
			holy_reader_getline_t getline, void *getline_data)
{
  struct holy_script *parsed_script;

  /* Parse the script.  */
  parsed_script = holy_script_parse (line, getline, getline_data);

  if (parsed_script)
    {
      /* Execute the command(s).  */
      holy_script_execute (parsed_script);

      /* The parsed script was executed, throw it away.  */
      holy_script_unref (parsed_script);
    }

  return holy_errno;
}

static holy_command_t cmd_break;
static holy_command_t cmd_continue;
static holy_command_t cmd_shift;
static holy_command_t cmd_setparams;
static holy_command_t cmd_return;

void
holy_script_init (void)
{
  cmd_break = holy_register_command ("break", holy_script_break,
				     N_("[NUM]"), N_("Exit from loops"));
  cmd_continue = holy_register_command ("continue", holy_script_break,
					N_("[NUM]"), N_("Continue loops"));
  cmd_shift = holy_register_command ("shift", holy_script_shift,
				     N_("[NUM]"),
				     /* TRANSLATORS: Positional arguments are
					arguments $0, $1, $2, ...  */
				     N_("Shift positional parameters."));
  cmd_setparams = holy_register_command ("setparams", holy_script_setparams,
					 N_("[VALUE]..."),
					 N_("Set positional parameters."));
  cmd_return = holy_register_command ("return", holy_script_return,
				      N_("[NUM]"),
				      /* TRANSLATORS: It's a command description
					 and "Return" is a verb, not a noun. The
					 command in question is "return" and
					 has exactly the same semanics as bash
					 equivalent.  */
				      N_("Return from a function."));
}

void
holy_script_fini (void)
{
  if (cmd_break)
    holy_unregister_command (cmd_break);
  cmd_break = 0;

  if (cmd_continue)
    holy_unregister_command (cmd_continue);
  cmd_continue = 0;

  if (cmd_shift)
    holy_unregister_command (cmd_shift);
  cmd_shift = 0;

  if (cmd_setparams)
    holy_unregister_command (cmd_setparams);
  cmd_setparams = 0;

  if (cmd_return)
    holy_unregister_command (cmd_return);
  cmd_return = 0;
}
