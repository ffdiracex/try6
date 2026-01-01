/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/env.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>
#include <holy/script_sh.h>
#include <regex.h>

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options[] =
  {
    { "set", 's', holy_ARG_OPTION_REPEATABLE,
      /* TRANSLATORS: in regexp you can mark some
	 groups with parentheses. These groups are
	 then numbered and you can save some of
	 them in variables. In other programs
	 those components aree often referenced with
	 back slash, e.g. \1. Compare
	 sed -e 's,\([a-z][a-z]*\),lowercase=\1,g'
	 The whole matching component is saved in VARNAME, not its number.
       */
      N_("Store matched component NUMBER in VARNAME."),
      N_("[NUMBER:]VARNAME"), ARG_TYPE_STRING },
    { 0, 0, 0, 0, 0, 0 }
  };

static holy_err_t
setvar (char *str, char *v, regmatch_t *m)
{
  char ch;
  holy_err_t err;
  ch = str[m->rm_eo];
  str[m->rm_eo] = '\0';
  err = holy_env_set (v, str + m->rm_so);
  str[m->rm_eo] = ch;
  return err;
}

static holy_err_t
set_matches (char **varnames, char *str, holy_size_t nmatches,
	     regmatch_t *matches)
{
  int i;
  char *p;
  char *q;
  holy_err_t err;
  unsigned long j;

  for (i = 0; varnames && varnames[i]; i++)
    {
      err = holy_ERR_NONE;
      p = holy_strchr (varnames[i], ':');
      if (! p)
	{
	  /* varname w/o index defaults to 1 */
	  if (nmatches < 2 || matches[1].rm_so == -1)
	    holy_env_unset (varnames[i]);
	  else
	    err = setvar (str, varnames[i], &matches[1]);
	}
      else
	{
	  j = holy_strtoul (varnames[i], &q, 10);
	  if (q != p)
	    return holy_error (holy_ERR_BAD_ARGUMENT,
			       "invalid variable name format %s", varnames[i]);

	  if (nmatches <= j || matches[j].rm_so == -1)
	    holy_env_unset (p + 1);
	  else
	    err = setvar (str, p + 1, &matches[j]);
	}

      if (err != holy_ERR_NONE)
	return err;
    }
  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_regexp (holy_extcmd_context_t ctxt, int argc, char **args)
{
  regex_t regex;
  int ret;
  holy_size_t s;
  char *comperr;
  holy_err_t err;
  regmatch_t *matches = 0;

  if (argc != 2)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("two arguments expected"));

  ret = regcomp (&regex, args[0], REG_EXTENDED);
  if (ret)
    goto fail;

  matches = holy_zalloc (sizeof (*matches) * (regex.re_nsub + 1));
  if (! matches)
    goto fail;

  ret = regexec (&regex, args[1], regex.re_nsub + 1, matches, 0);
  if (!ret)
    {
      err = set_matches (ctxt->state[0].args, args[1],
			 regex.re_nsub + 1, matches);
      regfree (&regex);
      holy_free (matches);
      return err;
    }

 fail:
  holy_free (matches);
  s = regerror (ret, &regex, 0, 0);
  comperr = holy_malloc (s);
  if (!comperr)
    {
      regfree (&regex);
      return holy_errno;
    }
  regerror (ret, &regex, comperr, s);
  err = holy_error (holy_ERR_TEST_FAILURE, "%s", comperr);
  regfree (&regex);
  holy_free (comperr);
  return err;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(regexp)
{
  cmd = holy_register_extcmd ("regexp", holy_cmd_regexp, 0,
			      /* TRANSLATORS: This are two arguments. So it's
				 two separate units to translate and pay
				 attention not to reverse them.  */
			      N_("REGEXP STRING"),
			      N_("Test if REGEXP matches STRING."), options);

  /* Setup holy script wildcard translator.  */
  holy_wildcard_translator = &holy_filename_translator;
}

holy_MOD_FINI(regexp)
{
  holy_unregister_extcmd (cmd);
  holy_wildcard_translator = 0;
}
