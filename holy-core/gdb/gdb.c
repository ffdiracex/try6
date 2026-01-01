/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/normal.h>
#include <holy/term.h>
#include <holy/cpu/gdb.h>
#include <holy/gdb.h>
#include <holy/serial.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t
holy_cmd_gdbstub (struct holy_command *cmd __attribute__ ((unused)),
		  int argc, char **args)
{
  struct holy_serial_port *port;
  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, "port required");
  port = holy_serial_find (args[0]);
  if (!port)
    return holy_errno;
  holy_gdb_port = port;
  /* TRANSLATORS: at this position holy waits for the user to do an action
     in remote debugger, namely to tell it to establish connection.  */
  holy_puts_ (N_("Now connect the remote debugger, please."));
  holy_gdb_breakpoint ();
  return 0;
}

static holy_err_t
holy_cmd_gdbstop (struct holy_command *cmd __attribute__ ((unused)),
		  int argc __attribute__ ((unused)),
		  char **args __attribute__ ((unused)))
{
  holy_gdb_port = NULL;
  return 0;
}

static holy_err_t
holy_cmd_gdb_break (struct holy_command *cmd __attribute__ ((unused)),
		    int argc __attribute__ ((unused)),
		    char **args __attribute__ ((unused)))
{
  if (!holy_gdb_port)
    return holy_error (holy_ERR_BAD_ARGUMENT, "No GDB stub is running");
  holy_gdb_breakpoint ();
  return 0;
}

static holy_command_t cmd, cmd_stop, cmd_break;

holy_MOD_INIT (gdb)
{
  holy_gdb_idtinit ();
  cmd = holy_register_command ("gdbstub", holy_cmd_gdbstub,
			       N_("PORT"), 
			       /* TRANSLATORS: GDB stub is a small part of
				  GDB functionality running on local host
				  which allows remote debugger to
				  connect to it.  */
			       N_("Start GDB stub on given port"));
  cmd_break = holy_register_command ("gdbstub_break", holy_cmd_gdb_break,
				     /* TRANSLATORS: this refers to triggering
					a breakpoint so that the user will land
					into GDB.  */
				     0, N_("Break into GDB"));
  cmd_stop = holy_register_command ("gdbstub_stop", holy_cmd_gdbstop,
				    0, N_("Stop GDB stub"));
}

holy_MOD_FINI (gdb)
{
  holy_unregister_command (cmd);
  holy_unregister_command (cmd_stop);
  holy_gdb_idtrestore ();
}

