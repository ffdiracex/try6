/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/script_sh.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/term.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t
holy_cmd_eval (holy_command_t cmd __attribute__((__unused__)),
	       int argc, char *argv[])
{
  int i;
  holy_size_t size = argc; /* +1 for final zero */
  char *str, *p;
  holy_err_t ret;

  if (argc == 0)
    return holy_ERR_NONE;

  for (i = 0; i < argc; i++)
    size += holy_strlen (argv[i]);

  str = p = holy_malloc (size);
  if (!str)
    return holy_errno;

  for (i = 0; i < argc; i++)
    {
      p = holy_stpcpy (p, argv[i]);
      *p++ = ' ';
    }
  *--p = '\0';

  ret = holy_script_execute_sourcecode (str);
  holy_free (str);
  return ret;
}

static holy_command_t cmd;

holy_MOD_INIT(eval)
{
  cmd = holy_register_command ("eval", holy_cmd_eval, N_("STRING ..."),
				N_("Evaluate arguments as holy commands"));
}

holy_MOD_FINI(eval)
{
  holy_unregister_command (cmd);
}

