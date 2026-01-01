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
holy_cmd_reboot (holy_command_t cmd __attribute__ ((unused)),
		 int argc __attribute__ ((unused)),
		 char **args __attribute__ ((unused)))
{
  holy_reboot ();
}

static holy_command_t cmd;

holy_MOD_INIT(reboot)
{
  cmd = holy_register_command ("reboot", holy_cmd_reboot,
			       0, N_("Reboot the computer."));
}

holy_MOD_FINI(reboot)
{
  holy_unregister_command (cmd);
}
