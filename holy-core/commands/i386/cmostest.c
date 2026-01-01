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
parse_args (int argc, char *argv[], int *byte, int *bit)
{
  char *rest;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, "address required");

  *byte = holy_strtoul (argv[0], &rest, 0);
  if (*rest != ':')
    return holy_error (holy_ERR_BAD_ARGUMENT, "address required");

  *bit = holy_strtoul (rest + 1, 0, 0);

  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_cmostest (struct holy_command *cmd __attribute__ ((unused)),
		   int argc, char *argv[])
{
  int byte = 0, bit = 0;
  holy_err_t err;
  holy_uint8_t value;

  err = parse_args (argc, argv, &byte, &bit);
  if (err)
    return err;

  err = holy_cmos_read (byte, &value);
  if (err)
    return err;

  if (value & (1 << bit))
    return holy_ERR_NONE;

  return holy_error (holy_ERR_TEST_FAILURE, N_("false"));
}

static holy_err_t
holy_cmd_cmosclean (struct holy_command *cmd __attribute__ ((unused)),
		    int argc, char *argv[])
{
  int byte = 0, bit = 0;
  holy_err_t err;
  holy_uint8_t value;

  err = parse_args (argc, argv, &byte, &bit);
  if (err)
    return err;
  err = holy_cmos_read (byte, &value);
  if (err)
    return err;

  return holy_cmos_write (byte, value & (~(1 << bit)));
}

static holy_err_t
holy_cmd_cmosset (struct holy_command *cmd __attribute__ ((unused)),
		    int argc, char *argv[])
{
  int byte = 0, bit = 0;
  holy_err_t err;
  holy_uint8_t value;

  err = parse_args (argc, argv, &byte, &bit);
  if (err)
    return err;
  err = holy_cmos_read (byte, &value);
  if (err)
    return err;

  return holy_cmos_write (byte, value | (1 << bit));
}

static holy_command_t cmd, cmd_clean, cmd_set;


holy_MOD_INIT(cmostest)
{
  cmd = holy_register_command ("cmostest", holy_cmd_cmostest,
			       N_("BYTE:BIT"),
			       N_("Test bit at BYTE:BIT in CMOS."));
  cmd_clean = holy_register_command ("cmosclean", holy_cmd_cmosclean,
				     N_("BYTE:BIT"),
				     N_("Clear bit at BYTE:BIT in CMOS."));
  cmd_set = holy_register_command ("cmosset", holy_cmd_cmosset,
				   N_("BYTE:BIT"),
				   /* TRANSLATORS: A bit may be either set (1) or clear (0).  */
				   N_("Set bit at BYTE:BIT in CMOS."));
}

holy_MOD_FINI(cmostest)
{
  holy_unregister_command (cmd);
  holy_unregister_command (cmd_clean);
  holy_unregister_command (cmd_set);
}
