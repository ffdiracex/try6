/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/extcmd.h>
#include <holy/env.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_extcmd_t cmd_read_byte, cmd_read_word, cmd_read_dword;
static holy_command_t cmd_write_byte, cmd_write_word, cmd_write_dword;

static const struct holy_arg_option options[] =
  {
    {0, 'v', 0, N_("Save read value into variable VARNAME."),
     N_("VARNAME"), ARG_TYPE_STRING},
    {0, 0, 0, 0, 0, 0}
  };


static holy_err_t
holy_cmd_read (holy_extcmd_context_t ctxt, int argc, char **argv)
{
  holy_addr_t addr;
  holy_uint32_t value = 0;
  char buf[sizeof ("XXXXXXXX")];

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));

  addr = holy_strtoul (argv[0], 0, 0);
  switch (ctxt->extcmd->cmd->name[sizeof ("read_") - 1])
    {
    case 'd':
      value = *((volatile holy_uint32_t *) addr);
      break;

    case 'w':
      value = *((volatile holy_uint16_t *) addr);
      break;

    case 'b':
      value = *((volatile holy_uint8_t *) addr);
      break;
    }

  if (ctxt->state[0].set)
    {
      holy_snprintf (buf, sizeof (buf), "%x", value);
      holy_env_set (ctxt->state[0].arg, buf);
    }
  else
    holy_printf ("0x%x\n", value);

  return 0;
}

static holy_err_t
holy_cmd_write (holy_command_t cmd, int argc, char **argv)
{
  holy_addr_t addr;
  holy_uint32_t value;
  holy_uint32_t mask = 0xffffffff;

  if (argc != 2 && argc != 3)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("two arguments expected"));

  addr = holy_strtoul (argv[0], 0, 0);
  value = holy_strtoul (argv[1], 0, 0);
  if (argc == 3)
    mask = holy_strtoul (argv[2], 0, 0);
  value &= mask;
  switch (cmd->name[sizeof ("write_") - 1])
    {
    case 'd':
      if (mask != 0xffffffff)
	*((volatile holy_uint32_t *) addr)
	  = (*((volatile holy_uint32_t *) addr) & ~mask) | value;
      else
	*((volatile holy_uint32_t *) addr) = value;
      break;

    case 'w':
      if ((mask & 0xffff) != 0xffff)
	*((volatile holy_uint16_t *) addr)
	  = (*((volatile holy_uint16_t *) addr) & ~mask) | value;
      else
	*((volatile holy_uint16_t *) addr) = value;
      break;

    case 'b':
      if ((mask & 0xff) != 0xff)
	*((volatile holy_uint8_t *) addr)
	  = (*((volatile holy_uint8_t *) addr) & ~mask) | value;
      else
	*((volatile holy_uint8_t *) addr) = value;
      break;
    }

  return 0;
}

holy_MOD_INIT(memrw)
{
  cmd_read_byte =
    holy_register_extcmd ("read_byte", holy_cmd_read, 0,
			  N_("ADDR"), N_("Read 8-bit value from ADDR."),
			  options);
  cmd_read_word =
    holy_register_extcmd ("read_word", holy_cmd_read, 0,
			  N_("ADDR"), N_("Read 16-bit value from ADDR."),
			  options);
  cmd_read_dword =
    holy_register_extcmd ("read_dword", holy_cmd_read, 0,
			  N_("ADDR"), N_("Read 32-bit value from ADDR."),
			  options);
  cmd_write_byte =
    holy_register_command ("write_byte", holy_cmd_write,
			   N_("ADDR VALUE [MASK]"),
			   N_("Write 8-bit VALUE to ADDR."));
  cmd_write_word =
    holy_register_command ("write_word", holy_cmd_write,
			   N_("ADDR VALUE [MASK]"),
			   N_("Write 16-bit VALUE to ADDR."));
  cmd_write_dword =
    holy_register_command ("write_dword", holy_cmd_write,
			   N_("ADDR VALUE [MASK]"),
			   N_("Write 32-bit VALUE to ADDR."));
}

holy_MOD_FINI(memrw)
{
  holy_unregister_extcmd (cmd_read_byte);
  holy_unregister_extcmd (cmd_read_word);
  holy_unregister_extcmd (cmd_read_dword);
  holy_unregister_command (cmd_write_byte);
  holy_unregister_command (cmd_write_word);
  holy_unregister_command (cmd_write_dword);
}
