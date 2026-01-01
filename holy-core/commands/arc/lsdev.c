/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/arc/arc.h>

holy_MOD_LICENSE ("GPLv2+");

/* Helper for holy_cmd_lsdev.  */
static int
holy_cmd_lsdev_iter (const char *name,
		     const struct holy_arc_component *comp __attribute__ ((unused)),
		     void *data __attribute__ ((unused)))
{
  holy_printf ("%s\n", name);
  return 0;
}

static holy_err_t
holy_cmd_lsdev (holy_command_t cmd __attribute__ ((unused)),
		int argc __attribute__ ((unused)),
		char **args __attribute__ ((unused)))
{
  holy_arc_iterate_devs (holy_cmd_lsdev_iter, 0, 0);
  return 0;
}

static holy_command_t cmd;

holy_MOD_INIT(lsdev)
{
  cmd = holy_register_command ("lsdev", holy_cmd_lsdev, "",
			      N_("List devices."));
}

holy_MOD_FINI(lsdev)
{
  holy_unregister_command (cmd);
}
