/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/term.h>
#include <holy/time.h>
#include <holy/types.h>
#include <holy/misc.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options[] =
  {
    {"verbose", 'v', 0, N_("Verbose countdown."), 0, 0},
    {"interruptible", 'i', 0, N_("Allow to interrupt with ESC."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static struct holy_term_coordinate *pos;

static void
do_print (int n)
{
  holy_term_restore_pos (pos);
  /* NOTE: Do not remove the trailing space characters.
     They are required to clear the line.  */
  holy_printf ("%d    ", n);
  holy_refresh ();
}

/* Based on holy_millisleep() from kern/generic/millisleep.c.  */
static int
holy_interruptible_millisleep (holy_uint32_t ms)
{
  holy_uint64_t start;

  start = holy_get_time_ms ();

  while (holy_get_time_ms () - start < ms)
    if (holy_getkey_noblock () == holy_TERM_ESC)
      return 1;

  return 0;
}

static holy_err_t
holy_cmd_sleep (holy_extcmd_context_t ctxt, int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;
  int n;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));

  n = holy_strtoul (args[0], 0, 10);

  if (n == 0)
    {
      /* Either `0' or broken input.  */
      return 0;
    }

  holy_refresh ();

  pos = holy_term_save_pos ();

  for (; n; n--)
    {
      if (state[0].set)
	do_print (n);

      if (state[1].set)
	{
	  if (holy_interruptible_millisleep (1000))
	    return 1;
	}
      else
	holy_millisleep (1000);
    }
  if (state[0].set)
    do_print (0);

  return 0;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(sleep)
{
  cmd = holy_register_extcmd ("sleep", holy_cmd_sleep, 0,
			      N_("NUMBER_OF_SECONDS"),
			      N_("Wait for a specified number of seconds."),
			      options);
}

holy_MOD_FINI(sleep)
{
  holy_unregister_extcmd (cmd);
}
