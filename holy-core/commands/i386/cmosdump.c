/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/command.h>
#include <holy/misc.h>
#include <holy/cmos.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t
holy_cmd_cmosdump (struct holy_command *cmd __attribute__ ((unused)),
		   int argc __attribute__ ((unused)), char *argv[] __attribute__ ((unused)))
{
  int i;

  for (i = 0; i < 256; i++)
    {
      holy_err_t err;
      holy_uint8_t value;
      if ((i & 0xf) == 0)
	holy_printf ("%02x: ", i);

      err = holy_cmos_read (i, &value);
      if (err)
	return err;

      holy_printf ("%02x ", value);
      if ((i & 0xf) == 0xf)
	holy_printf ("\n");
    }
  return holy_ERR_NONE;
}

static holy_command_t cmd;


holy_MOD_INIT(cmosdump)
{
  cmd = holy_register_command ("cmosdump", holy_cmd_cmosdump,
			       0,
			       N_("Show raw dump of the CMOS contents."));
}

holy_MOD_FINI(cmosdump)
{
  holy_unregister_command (cmd);
}
