/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/extcmd.h>
#include <holy/term.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options[] =
  {
    /* TRANSLATORS: "Check" in a sense that if this key is pressed then
       "true" is returned, otherwise "false".  */
    {"shift", 's', 0, N_("Check Shift key."), 0, 0},
    {"ctrl", 'c', 0, N_("Check Control key."), 0, 0},
    {"alt", 'a', 0, N_("Check Alt key."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static int
holy_getkeystatus (void)
{
  int status = 0;
  holy_term_input_t term;

  if (holy_term_poll_usb)
    holy_term_poll_usb (0);

  FOR_ACTIVE_TERM_INPUTS(term)
  {
    if (term->getkeystatus)
      status |= term->getkeystatus (term);
  }

  return status;
}

static holy_err_t
holy_cmd_keystatus (holy_extcmd_context_t ctxt,
		    int argc __attribute__ ((unused)),
		    char **args __attribute__ ((unused)))
{
  struct holy_arg_list *state = ctxt->state;
  int expect_mods = 0;
  int mods;

  if (state[0].set)
    expect_mods |= (holy_TERM_STATUS_LSHIFT | holy_TERM_STATUS_RSHIFT);
  if (state[1].set)
    expect_mods |= (holy_TERM_STATUS_LCTRL | holy_TERM_STATUS_RCTRL);
  if (state[2].set)
    expect_mods |= (holy_TERM_STATUS_LALT | holy_TERM_STATUS_RALT);

  holy_dprintf ("keystatus", "expect_mods: %d\n", expect_mods);

  /* Without arguments, just check whether getkeystatus is supported at
     all.  */
  if (expect_mods == 0)
    {
      holy_term_input_t term;
      int nterms = 0;

      FOR_ACTIVE_TERM_INPUTS (term)
	if (!term->getkeystatus)
	  return holy_error (holy_ERR_TEST_FAILURE, N_("false"));
	else
	  nterms++;
      if (!nterms)
	return holy_error (holy_ERR_TEST_FAILURE, N_("false"));
      return 0;
    }

  mods = holy_getkeystatus ();
  holy_dprintf ("keystatus", "mods: %d\n", mods);
  if (mods >= 0 && (mods & expect_mods) != 0)
    return 0;
  else
    return holy_error (holy_ERR_TEST_FAILURE, N_("false"));
}

static holy_extcmd_t cmd;

holy_MOD_INIT(keystatus)
{
  cmd = holy_register_extcmd ("keystatus", holy_cmd_keystatus, 0,
			      "[--shift] [--ctrl] [--alt]",
			      /* TRANSLATORS: there are 3 modifiers.  */
			      N_("Check key modifier status."),
			      options);
}

holy_MOD_FINI(keystatus)
{
  holy_unregister_extcmd (cmd);
}
