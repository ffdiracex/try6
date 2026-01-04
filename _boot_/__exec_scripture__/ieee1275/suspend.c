/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/term.h>
#include <holy/ieee1275/ieee1275.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t
holy_cmd_suspend (holy_command_t cmd __attribute__ ((unused)),
		  int argc __attribute__ ((unused)),
		  char **args __attribute__ ((unused)))
{
  holy_puts_ (N_("Run `go' to resume holy."));
  holy_ieee1275_enter ();
  holy_cls ();
  return 0;
}

static holy_command_t cmd;

holy_MOD_INIT(ieee1275_suspend)
{
  cmd = holy_register_command ("suspend", holy_cmd_suspend,
			       0, N_("Return to IEEE1275 prompt."));
}

holy_MOD_FINI(ieee1275_suspend)
{
  holy_unregister_command (cmd);
}
