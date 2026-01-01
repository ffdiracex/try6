/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/xen.h>
#include <holy/term.h>

holy_MOD_LICENSE ("GPLv2+");

static int
hook (const char *dir, void *hook_data __attribute__ ((unused)))
{
  holy_printf ("%s\n", dir);
  return 0;
}

static holy_err_t
holy_cmd_lsxen (holy_command_t cmd __attribute__ ((unused)),
		int argc, char **args)
{
  char *dir;
  holy_err_t err;
  char *buf;

  if (argc >= 1)
    return holy_xenstore_dir (args[0], hook, NULL);

  buf = holy_xenstore_get_file ("domid", NULL);
  if (!buf)
    return holy_errno;
  dir = holy_xasprintf ("/local/domain/%s", buf);
  holy_free (buf);
  err = holy_xenstore_dir (dir, hook, NULL);
  holy_free (dir);
  return err;
}

static holy_err_t
holy_cmd_catxen (holy_command_t cmd __attribute__ ((unused)),
		 int argc, char **args)
{
  const char *dir = "domid";
  char *buf;

  if (argc >= 1)
    dir = args[0];

  buf = holy_xenstore_get_file (dir, NULL);
  if (!buf)
    return holy_errno;
  holy_xputs (buf);
  holy_xputs ("\n");
  holy_free (buf);
  return holy_ERR_NONE;

}

static holy_command_t cmd_ls, cmd_cat;

holy_MOD_INIT (lsxen)
{
  cmd_ls = holy_register_command ("xen_ls", holy_cmd_lsxen, N_("[DIR]"),
				  N_("List Xen storage."));
  cmd_cat = holy_register_command ("xen_cat", holy_cmd_catxen, N_("[DIR]"),
				   N_("List Xen storage."));
}

holy_MOD_FINI (lsxen)
{
  holy_unregister_command (cmd_ls);
  holy_unregister_command (cmd_cat);
}
