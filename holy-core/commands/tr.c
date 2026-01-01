/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/env.h>
#include <holy/i18n.h>
#include <holy/misc.h>
#include <holy/extcmd.h>

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options[] =
  {
    { "set", 's', 0, N_("Set a variable to return value."), N_("VARNAME"), ARG_TYPE_STRING },
    { "upcase", 'U', 0, N_("Translate to upper case."), 0, 0 },
    { "downcase", 'D', 0, N_("Translate to lower case."), 0, 0 },
    { 0, 0, 0, 0, 0, 0 }
  };

static const char *letters_lowercase = "abcdefghijklmnopqrstuvwxyz";
static const char *letters_uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static holy_err_t
holy_cmd_tr (holy_extcmd_context_t ctxt, int argc, char **args)
{
  char *var = 0;
  const char *input = 0;
  char *output = 0, *optr;
  const char *s1 = 0;
  const char *s2 = 0;
  const char *iptr;

  /* Select the defaults from options. */
  if (ctxt->state[0].set) {
    var = ctxt->state[0].arg;
    input = holy_env_get (var);
  }

  if (ctxt->state[1].set) {
    s1 = letters_lowercase;
    s2 = letters_uppercase;
  }

  if (ctxt->state[2].set) {
    s1 = letters_uppercase;
    s2 = letters_lowercase;
  }

  /* Check for arguments and update the defaults.  */
  if (argc == 1)
    input = args[0];

  else if (argc == 2) {
    s1 = args[0];
    s2 = args[1];

  } else if (argc == 3) {
    s1 = args[0];
    s2 = args[1];
    input = args[2];

  } else if (argc > 3)
    return holy_error (holy_ERR_BAD_ARGUMENT, "too many parameters");

  if (!s1 || !s2 || !input)
    return holy_error (holy_ERR_BAD_ARGUMENT, "missing parameters");

  if (holy_strlen (s1) != holy_strlen (s2))
    return holy_error (holy_ERR_BAD_ARGUMENT, "set sizes did not match");

  /* Translate input into output buffer.  */

  output = holy_malloc (holy_strlen (input) + 1);
  if (! output)
    return holy_errno;

  optr = output;
  for (iptr = input; *iptr; iptr++)
    {
      char *p = holy_strchr (s1, *iptr);
      if (p)
	*optr++ = s2[p - s1];
      else
	*optr++ = *iptr;
    }
  *optr = '\0';

  if (ctxt->state[0].set)
    holy_env_set (var, output);
  else
    holy_printf ("%s\n", output);

  holy_free (output);
  return holy_ERR_NONE;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(tr)
{
  cmd = holy_register_extcmd ("tr", holy_cmd_tr, 0, N_("[OPTIONS] [SET1] [SET2] [STRING]"),
			      N_("Translate SET1 characters to SET2 in STRING."), options);
}

holy_MOD_FINI(tr)
{
  holy_unregister_extcmd (cmd);
}
