/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t
holy_cmd_true (struct holy_command *cmd __attribute__ ((unused)),
	       int argc __attribute__ ((unused)),
	       char *argv[] __attribute__ ((unused)))
{
  return 0;
}

static holy_err_t
holy_cmd_false (struct holy_command *cmd __attribute__ ((unused)),
		int argc __attribute__ ((unused)),
		char *argv[] __attribute__ ((unused)))
{
  return holy_error (holy_ERR_TEST_FAILURE, N_("false"));
}

static holy_command_t cmd_true, cmd_false;


holy_MOD_INIT(true)
{
  cmd_true =
    holy_register_command ("true", holy_cmd_true,
			   /* TRANSLATORS: it's a command description.  */
			   0, N_("Do nothing, successfully."));
  cmd_false =
    holy_register_command ("false", holy_cmd_false,
			   /* TRANSLATORS: it's a command description.  */
			   0, N_("Do nothing, unsuccessfully."));
}

holy_MOD_FINI(true)
{
  holy_unregister_command (cmd_true);
  holy_unregister_command (cmd_false);
}
