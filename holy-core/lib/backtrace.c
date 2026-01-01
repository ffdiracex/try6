/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/command.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/term.h>
#include <holy/backtrace.h>

holy_MOD_LICENSE ("GPLv2+");

void
holy_backtrace_print_address (void *addr)
{
  holy_dl_t mod;

  FOR_DL_MODULES (mod)
  {
    holy_dl_segment_t segment;
    for (segment = mod->segment; segment; segment = segment->next)
      if (segment->addr <= addr && (holy_uint8_t *) segment->addr
	  + segment->size > (holy_uint8_t *) addr)
	{
	  holy_printf ("%s.%x+%" PRIxholy_SIZE, mod->name, segment->section,
		       (holy_size_t) ((holy_uint8_t *) addr - (holy_uint8_t *) segment->addr));
	  return;
	}
  }

  holy_printf ("%p", addr);
}

static holy_err_t
holy_cmd_backtrace (holy_command_t cmd __attribute__ ((unused)),
		    int argc __attribute__ ((unused)),
		    char **args __attribute__ ((unused)))
{
  holy_backtrace ();
  return 0;
}

static holy_command_t cmd;

holy_MOD_INIT(backtrace)
{
  cmd = holy_register_command ("backtrace", holy_cmd_backtrace,
			       0, N_("Print backtrace."));
}

holy_MOD_FINI(backtrace)
{
  holy_unregister_command (cmd);
}
