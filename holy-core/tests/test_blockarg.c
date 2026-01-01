/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/err.h>
#include <holy/misc.h>
#include <holy/i18n.h>
#include <holy/extcmd.h>
#include <holy/script_sh.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t
test_blockarg (holy_extcmd_context_t ctxt, int argc, char **args)
{
  if (! ctxt->script)
    return holy_error (holy_ERR_BAD_ARGUMENT, "no block parameter");

  holy_printf ("%s\n", args[argc - 1]);
  holy_script_execute (ctxt->script);
  return holy_ERR_NONE;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(test_blockarg)
{
  cmd = holy_register_extcmd ("test_blockarg", test_blockarg,
			      holy_COMMAND_FLAG_BLOCKS,
			      N_("BLOCK"),
			      /* TRANSLATORS: this is the BLOCK-argument, not
			       environment block.  */
			      N_("Print and execute block argument."), 0);
}

holy_MOD_FINI(test_blockarg)
{
  holy_unregister_extcmd (cmd);
}
