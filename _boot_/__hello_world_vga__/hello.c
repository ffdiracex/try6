/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t
holy_cmd_hello (holy_extcmd_context_t ctxt __attribute__ ((unused)),
		int argc __attribute__ ((unused)),
		char **args __attribute__ ((unused)))
{
  holy_printf ("%s\n", _("Hello World"));
  return 0;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(hello)
{
  cmd = holy_register_extcmd ("hello", holy_cmd_hello, 0, 0,
			      N_("Say `Hello World'."), 0);
}

holy_MOD_FINI(hello)
{
  holy_unregister_extcmd (cmd);
}
