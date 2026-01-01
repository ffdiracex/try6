/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/command.h>
#include <holy/misc.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t __attribute__ ((noreturn))
holy_cmd_halt (holy_command_t cmd __attribute__ ((unused)),
	       int argc __attribute__ ((unused)),
	       char **args __attribute__ ((unused)))
{
  holy_halt ();
}

static holy_command_t cmd;

holy_MOD_INIT(halt)
{
  cmd = holy_register_command ("halt", holy_cmd_halt,
			       0, N_("Halts the computer.  This command does" 
			       " not work on all firmware implementations."));
}

holy_MOD_FINI(halt)
{
  holy_unregister_command (cmd);
}
