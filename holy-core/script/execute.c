/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/env.h>
#include <holy/script_sh.h>
#include <holy/command.h>
#include <holy/menu.h>
#include <holy/lib/arg.h>
#include <holy/normal.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>
#include <holy/tpm.h>

/* Max digits for a char is 3 (0xFF is 255), similarly for an int it
   is sizeof (int) * 3, and one extra for a possible -ve sign.  */
#define ERRNO_DIGITS_MAX  (sizeof (int) * 3 + 1)

static unsigned long is_continue;
static unsigned long active_loops;
static unsigned long active_breaks;
static unsigned long function_return;

#define holy_SCRIPT_SCOPE_MALLOCED      1
#define holy_SCRIPT_SCOPE_ARGS_MALLOCED 2

/* Scope for holy script functions.  */
struct holy_script_scope
{
  unsigned flags;
  unsigned shifts;
  struct holy_script_argv argv;
};
static struct holy_script_scope *scope = 0;

/* Wildcard translator for holy script.  */
struct holy_script_wildcard_translator *holy_wildcard_translator;

static char*
wildcard_escape (const char *s)
{
  int i;
  int len;
  char ch;
  char *p;

  len = holy_strlen (s);
  p = holy_malloc (len * 2 + 1);
  if (! p)
    return NULL;

  i = 0;
  while ((ch = *s++))
    {
      if (ch == '*' || ch == '\\' || ch == '?')
	p[i++] = '\\';
      p[i++] = ch;
    }
  p[i] = '\0';
  return p;
}

static char*
wildcard_unescape (const char *s)
{
  int i;
  int len;
  char ch;
  char *p;

  len = holy_strlen (s);
  p = holy_malloc (len + 1);
  if (! p)
    return NULL;

  i = 0;
  while ((ch = *s++))
    {
      if (ch == '\\')
	p[i++] = *s++;
      else
	p[i++] = ch;
    }
  p[i] = '\0';
  return p;
}

static void
replace_scope (struct holy_script_scope *new_scope)
{
  if (scope)
    {
      scope->argv.argc += scope->shifts;
      scope->argv.args -= scope->shifts;

      if (scope->flags & holy_SCRIPT_SCOPE_ARGS_MALLOCED)
	holy_script_argv_free (&scope->argv);

      if (scope->flags & holy_SCRIPT_SCOPE_MALLOCED)
	holy_free (scope);
    }
  scope = new_scope;
}

holy_err_t
holy_script_break (holy_command_t cmd, int argc, char *argv[])
{
  char *p = 0;
  unsigned long count;

  if (argc == 0)
    count = 1;
  else if (argc > 1)
    return  holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));
  else
    {
      count = holy_strtoul (argv[0], &p, 10);
      if (holy_errno)
	return holy_errno;
      if (*p != '\0')
	return holy_error (holy_ERR_BAD_ARGUMENT, N_("unrecognized number"));
      if (count == 0)
	/* TRANSLATORS: 0 is a quantifier. "break" (similar to bash)
	   can be used e.g. to break 3 loops at once.
	   But asking it to break 0 loops makes no sense. */
	return holy_error (holy_ERR_BAD_ARGUMENT, N_("can't break 0 loops"));
    }

  is_continue = holy_strcmp (cmd->name, "break") ? 1 : 0;
  active_breaks = count;
  if (active_breaks > active_loops)
    active_breaks = active_loops;
  return holy_ERR_NONE;
}

holy_err_t
holy_script_shift (holy_command_t cmd __attribute__((unused)),
		   int argc, char *argv[])
{
  char *p = 0;
  unsigned long n = 0;

  if (! scope)
    return holy_ERR_NONE;

  if (argc == 0)
    n = 1;

  else if (argc > 1)
    return holy_ERR_BAD_ARGUMENT;

  else
    {
      n = holy_strtoul (argv[0], &p, 10);
      if (*p != '\0')
	return holy_ERR_BAD_ARGUMENT;
    }

  if (n > scope->argv.argc)
    return holy_ERR_BAD_ARGUMENT;

  scope->shifts += n;
  scope->argv.argc -= n;
  scope->argv.args += n;
  return holy_ERR_NONE;
}

holy_err_t
holy_script_setparams (holy_command_t cmd __attribute__((unused)),
		       int argc, char **args)
{
  struct holy_script_scope *new_scope;
  struct holy_script_argv argv = { 0, 0, 0 };

  if (! scope)
    return holy_ERR_INVALID_COMMAND;

  new_scope = holy_malloc (sizeof (*new_scope));
  if (! new_scope)
    return holy_errno;

  if (holy_script_argv_make (&argv, argc, args))
    {
      holy_free (new_scope);
      return holy_errno;
    }

  new_scope->shifts = 0;
  new_scope->argv = argv;
  new_scope->flags = holy_SCRIPT_SCOPE_MALLOCED |
    holy_SCRIPT_SCOPE_ARGS_MALLOCED;

  replace_scope (new_scope);
  return holy_ERR_NONE;
}

holy_err_t
holy_script_return (holy_command_t cmd __attribute__((unused)),
		    int argc, char *argv[])
{
  char *p;
  unsigned long n;

  if (! scope || argc > 1)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       /* TRANSLATORS: It's about not being
			  inside a function. "return" can be used only
			  in a function and this error occurs if it's used
			  anywhere else.  */
		       N_("not in function body"));

  if (argc == 0)
    {
      const char *t;
      function_return = 1;
      t = holy_env_get ("?");
      if (!t)
	return holy_ERR_NONE;
      return holy_strtoul (t, NULL, 10);
    }

  n = holy_strtoul (argv[0], &p, 10);
  if (holy_errno)
    return holy_errno;
  if (*p != '\0')
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("unrecognized number"));

  function_return = 1;
  return n ? holy_error (n, N_("false")) : holy_ERR_NONE;
}

static int
holy_env_special (const char *name)
{
  if (holy_isdigit (name[0]) ||
      holy_strcmp (name, "#") == 0 ||
      holy_strcmp (name, "*") == 0 ||
      holy_strcmp (name, "@") == 0)
    return 1;
  return 0;
}

static char **
holy_script_env_get (const char *name, holy_script_arg_type_t type)
{
  unsigned i;
  struct holy_script_argv result = { 0, 0, 0 };

  if (holy_script_argv_next (&result))
    goto fail;

  if (! holy_env_special (name))
    {
      const char *v = holy_env_get (name);
      if (v && v[0])
	{
	  if (type == holy_SCRIPT_ARG_TYPE_VAR)
	    {
	      if (holy_script_argv_split_append (&result, v))
		goto fail;
	    }
	  else
	    if (holy_script_argv_append (&result, v, holy_strlen (v)))
	      goto fail;
	}
    }
  else if (! scope)
    {
      if (holy_script_argv_append (&result, 0, 0))
	goto fail;
    }
  else if (holy_strcmp (name, "#") == 0)
    {
      char buffer[ERRNO_DIGITS_MAX + 1];
      holy_snprintf (buffer, sizeof (buffer), "%u", scope->argv.argc);
      if (holy_script_argv_append (&result, buffer, holy_strlen (buffer)))
	goto fail;
    }
  else if (holy_strcmp (name, "*") == 0)
    {
      for (i = 0; i < scope->argv.argc; i++)
	if (type == holy_SCRIPT_ARG_TYPE_VAR)
	  {
	    if (i != 0 && holy_script_argv_next (&result))
	      goto fail;

	    if (holy_script_argv_split_append (&result, scope->argv.args[i]))
	      goto fail;
	  }
	else
	  {
	    if (i != 0 && holy_script_argv_append (&result, " ", 1))
	      goto fail;

	    if (holy_script_argv_append (&result, scope->argv.args[i],
					 holy_strlen (scope->argv.args[i])))
	      goto fail;
	  }
    }
  else if (holy_strcmp (name, "@") == 0)
    {
      for (i = 0; i < scope->argv.argc; i++)
	{
	  if (i != 0 && holy_script_argv_next (&result))
	    goto fail;

	  if (type == holy_SCRIPT_ARG_TYPE_VAR)
	    {
	      if (holy_script_argv_split_append (&result, scope->argv.args[i]))
		goto fail;
	    }
	  else
	    if (holy_script_argv_append (&result, scope->argv.args[i],
					 holy_strlen (scope->argv.args[i])))
	      goto fail;
	}
    }
  else
    {
      unsigned long num = holy_strtoul (name, 0, 10);
      if (num == 0)
	; /* XXX no file name, for now.  */

      else if (num <= scope->argv.argc)
	{
	  if (type == holy_SCRIPT_ARG_TYPE_VAR)
	    {
	      if (holy_script_argv_split_append (&result,
						 scope->argv.args[num - 1]))
		goto fail;
	    }
	  else
	    if (holy_script_argv_append (&result, scope->argv.args[num - 1],
					 holy_strlen (scope->argv.args[num - 1])
					 ))
	      goto fail;
	}
    }

  return result.args;

 fail:

  holy_script_argv_free (&result);
  return 0;
}

static holy_err_t
holy_script_env_set (const char *name, const char *val)
{
  if (holy_env_special (name))
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("invalid variable name `%s'"), name);

  return holy_env_set (name, val);
}

struct gettext_context
{
  char **allowed_strings;
  holy_size_t nallowed_strings;
  holy_size_t additional_len;
};

static int
parse_string (const char *str,
	      int (*hook) (const char *var, holy_size_t varlen,
			   char **ptr, struct gettext_context *ctx),
	      struct gettext_context *ctx,
	      char *put)
{
  const char *ptr;
  int escaped = 0;
  const char *optr;

  for (ptr = str; ptr && *ptr; )
    switch (*ptr)
      {
      case '\\':
	escaped = !escaped;
	if (!escaped && put)
	  *(put++) = '\\';
	ptr++;
	break;
      case '$':
	if (escaped)
	  {
	    escaped = 0;
	    if (put)
	      *(put++) = *ptr;
	    ptr++;
	    break;
	  }

	ptr++;
	switch (*ptr)
	  {
	  case '{':
	    {
	      optr = ptr + 1;
	      ptr = holy_strchr (optr, '}');
	      if (!ptr)
		break;
	      if (hook (optr, ptr - optr, &put, ctx))
		return 1;
	      ptr++;
	      break;
	    }
	  case '0' ... '9':
	    optr = ptr;
	    while (*ptr >= '0' && *ptr <= '9')
	      ptr++;
	    if (hook (optr, ptr - optr, &put, ctx))
	      return 1;
	    break;
	  case 'a' ... 'z':
	  case 'A' ... 'Z':
	  case '_':
	    optr = ptr;
	    while ((*ptr >= '0' && *ptr <= '9')
		   || (*ptr >= 'a' && *ptr <= 'z')
		   || (*ptr >= 'A' && *ptr <= 'Z')
		   || *ptr == '_')
	      ptr++;
	    if (hook (optr, ptr - optr, &put, ctx))
	      return 1;
	    break;
	  case '?':
	  case '#':
	    if (hook (ptr, 1, &put, ctx))
	      return 1;
	    ptr++;
	    break;
	  default:
	    if (put)
	      *(put++) = '$';
	  }
	break;
      default:
	if (escaped && put)
	  *(put++) = '\\';
	escaped = 0;
	if (put)
	  *(put++) = *ptr;
	ptr++;
	break;
      }
  if (put)
    *(put++) = 0;
  return 0;
}

static int
gettext_putvar (const char *str, holy_size_t len,
		char **ptr, struct gettext_context *ctx)
{
  const char *var;
  holy_size_t i;

  for (i = 0; i < ctx->nallowed_strings; i++)
    if (holy_strncmp (ctx->allowed_strings[i], str, len) == 0
	&& ctx->allowed_strings[i][len] == 0)
      {
	break;
      }
  if (i == ctx->nallowed_strings)
    return 0;

  /* Enough for any number.  */
  if (len == 1 && str[0] == '#')
    {
      holy_snprintf (*ptr, 30, "%u", scope->argv.argc);
      *ptr += holy_strlen (*ptr);
      return 0;
    }
  var = holy_env_get (ctx->allowed_strings[i]);
  if (var)
    *ptr = holy_stpcpy (*ptr, var);
  return 0;
}

static int
gettext_save_allow (const char *str, holy_size_t len,
		    char **ptr __attribute__ ((unused)),
		    struct gettext_context *ctx)
{
  ctx->allowed_strings[ctx->nallowed_strings++] = holy_strndup (str, len);
  if (!ctx->allowed_strings[ctx->nallowed_strings - 1])
    return 1;
  return 0;
}

static int
gettext_getlen (const char *str, holy_size_t len,
		char **ptr __attribute__ ((unused)),
		struct gettext_context *ctx)
{
  const char *var;
  holy_size_t i;

  for (i = 0; i < ctx->nallowed_strings; i++)
    if (holy_strncmp (ctx->allowed_strings[i], str, len) == 0
	&& ctx->allowed_strings[i][len] == 0)
      break;
  if (i == ctx->nallowed_strings)
    return 0;

  /* Enough for any number.  */
  if (len == 1 && str[0] == '#')
    {
      ctx->additional_len += 30;
      return 0;
    }
  var = holy_env_get (ctx->allowed_strings[i]);
  if (var)
    ctx->additional_len += holy_strlen (var);
  return 0;
}

static int
gettext_append (struct holy_script_argv *result, const char *orig_str)
{
  const char *template;
  char *res = 0;
  struct gettext_context ctx = {
    .allowed_strings = 0,
    .nallowed_strings = 0,
    .additional_len = 1
  };
  int rval = 1;
  const char *iptr;

  holy_size_t dollar_cnt = 0;

  for (iptr = orig_str; *iptr; iptr++)
    if (*iptr == '$')
      dollar_cnt++;
  ctx.allowed_strings = holy_malloc (sizeof (ctx.allowed_strings[0]) * dollar_cnt);

  if (parse_string (orig_str, gettext_save_allow, &ctx, 0))
    goto fail;

  template = _(orig_str);

  if (parse_string (template, gettext_getlen, &ctx, 0))
    goto fail;

  res = holy_malloc (holy_strlen (template) + ctx.additional_len);
  if (!res)
    goto fail;

  if (parse_string (template, gettext_putvar, &ctx, res))
    goto fail;

  char *escaped = 0;
  escaped = wildcard_escape (res);
  if (holy_script_argv_append (result, escaped, holy_strlen (escaped)))
    {
      holy_free (escaped);
      goto fail;
    }
  holy_free (escaped);

  rval = 0;
 fail:
  holy_free (res);
  {
    holy_size_t i;
    for (i = 0; i < ctx.nallowed_strings; i++)
      holy_free (ctx.allowed_strings[i]);
  }
  holy_free (ctx.allowed_strings);
  return rval;
}

static int
append (struct holy_script_argv *result,
	const char *s, int escape_type)
{
  int r;
  char *p = 0;

  if (escape_type == 0)
    return holy_script_argv_append (result, s, holy_strlen (s));

  if (escape_type > 0)
    p = wildcard_escape (s);
  else if (escape_type < 0)
    p = wildcard_unescape (s);

  if (! p)
    return 1;

  r = holy_script_argv_append (result, p, holy_strlen (p));
  holy_free (p);
  return r;
}

/* Convert arguments in ARGLIST into ARGV form.  */
static int
holy_script_arglist_to_argv (struct holy_script_arglist *arglist,
			     struct holy_script_argv *argv)
{
  int i;
  char **values = 0;
  struct holy_script_arg *arg = 0;
  struct holy_script_argv result = { 0, 0, 0 };

  for (; arglist && arglist->arg; arglist = arglist->next)
    {
      if (holy_script_argv_next (&result))
	goto fail;

      arg = arglist->arg;
      while (arg)
	{
	  switch (arg->type)
	    {
	    case holy_SCRIPT_ARG_TYPE_VAR:
	    case holy_SCRIPT_ARG_TYPE_DQVAR:
	      {
		int need_cleanup = 0;

		values = holy_script_env_get (arg->str, arg->type);
		for (i = 0; values && values[i]; i++)
		  {
		    if (!need_cleanup)
		      {
			if (i != 0 && holy_script_argv_next (&result))
			  {
			    need_cleanup = 1;
			    goto cleanup;
			  }

			if (arg->type == holy_SCRIPT_ARG_TYPE_VAR)
			  {
			    int len;
			    char ch;
			    char *p;
			    char *op;
			    const char *s = values[i];

			    len = holy_strlen (values[i]);
			    /* \? -> \\\? */
			    /* \* -> \\\* */
			    /* \ -> \\ */
			    p = holy_malloc (len * 2 + 1);
			    if (! p)
			      {
				need_cleanup = 1;
				goto cleanup;
			      }

			    op = p;
			    while ((ch = *s++))
			      {
				if (ch == '\\')
				  {
				    *op++ = '\\';
				    if (*s == '?' || *s == '*')
				      *op++ = '\\';
				  }
				*op++ = ch;
			      }
			    *op = '\0';

			    need_cleanup = holy_script_argv_append (&result, p, op - p);
			    holy_free (p);
			    /* Fall through to cleanup */
			  }
			else
			  {
			    need_cleanup = append (&result, values[i], 1);
			    /* Fall through to cleanup */
			  }
		      }

cleanup:
		    holy_free (values[i]);
		  }
		holy_free (values);

		if (need_cleanup)
		  goto fail;

		break;
	      }

	    case holy_SCRIPT_ARG_TYPE_BLOCK:
	      {
		char *p;
		if (holy_script_argv_append (&result, "{", 1))
		  goto fail;
		p = wildcard_escape (arg->str);
		if (!p)
		  goto fail;
		if (holy_script_argv_append (&result, p,
					     holy_strlen (p)))
		  {
		    holy_free (p);
		    goto fail;
		  }
		holy_free (p);
		if (holy_script_argv_append (&result, "}", 1))
		  goto fail;
	      }
	      result.script = arg->script;
	      break;

	    case holy_SCRIPT_ARG_TYPE_TEXT:
	      if (arg->str[0] &&
		  holy_script_argv_append (&result, arg->str,
					   holy_strlen (arg->str)))
		goto fail;
	      break;

	    case holy_SCRIPT_ARG_TYPE_GETTEXT:
	      {
		if (gettext_append (&result, arg->str))
		  goto fail;
	      }
	      break;

	    case holy_SCRIPT_ARG_TYPE_DQSTR:
	    case holy_SCRIPT_ARG_TYPE_SQSTR:
	      if (append (&result, arg->str, 1))
		goto fail;
	      break;
	    }
	  arg = arg->next;
	}
    }

  if (! result.args[result.argc - 1])
    result.argc--;

  /* Perform wildcard expansion.  */

  int j;
  int failed = 0;
  struct holy_script_argv unexpanded = result;

  result.argc = 0;
  result.args = 0;
  for (i = 0; unexpanded.args[i]; i++)
    {
      char **expansions = 0;
      if (holy_wildcard_translator
	  && holy_wildcard_translator->expand (unexpanded.args[i],
					       &expansions))
	{
	  holy_script_argv_free (&unexpanded);
	  goto fail;
	}

      if (! expansions)
	{
	  holy_script_argv_next (&result);
	  append (&result, unexpanded.args[i], -1);
	}
      else
	{
	  for (j = 0; expansions[j]; j++)
	    {
	      failed = (failed || holy_script_argv_next (&result) ||
			append (&result, expansions[j], 0));
	      holy_free (expansions[j]);
	    }
	  holy_free (expansions);
	  
	  if (failed)
	    {
	      holy_script_argv_free (&unexpanded);
	      goto fail;
	    }
	}
    }
  holy_script_argv_free (&unexpanded);

  *argv = result;
  return 0;

 fail:

  holy_script_argv_free (&result);
  return 1;
}

static holy_err_t
holy_script_execute_cmd (struct holy_script_cmd *cmd)
{
  int ret;
  char errnobuf[ERRNO_DIGITS_MAX + 1];

  if (cmd == 0)
    return 0;

  ret = cmd->exec (cmd);

  holy_snprintf (errnobuf, sizeof (errnobuf), "%d", ret);
  holy_env_set ("?", errnobuf);
  return ret;
}

/* Execute a function call.  */
holy_err_t
holy_script_function_call (holy_script_function_t func, int argc, char **args)
{
  holy_err_t ret = 0;
  unsigned long loops = active_loops;
  struct holy_script_scope *old_scope;
  struct holy_script_scope new_scope;

  active_loops = 0;
  new_scope.flags = 0;
  new_scope.shifts = 0;
  new_scope.argv.argc = argc;
  new_scope.argv.args = args;

  old_scope = scope;
  scope = &new_scope;

  ret = holy_script_execute (func->func);

  function_return = 0;
  active_loops = loops;
  replace_scope (old_scope); /* free any scopes by setparams */
  return ret;
}

/* Helper for holy_script_execute_sourcecode.  */
static holy_err_t
holy_script_execute_sourcecode_getline (char **line,
					int cont __attribute__ ((unused)),
					void *data)
{
  const char **source = data;
  const char *p;

  if (! *source)
    {
      *line = 0;
      return 0;
    }

  p = holy_strchr (*source, '\n');

  if (p)
    *line = holy_strndup (*source, p - *source);
  else
    *line = holy_strdup (*source);
  *source = p ? p + 1 : 0;
  return 0;
}

/* Execute a source script.  */
holy_err_t
holy_script_execute_sourcecode (const char *source)
{
  holy_err_t ret = 0;
  struct holy_script *parsed_script;

  while (source)
    {
      char *line;

      holy_script_execute_sourcecode_getline (&line, 0, &source);
      parsed_script = holy_script_parse
	(line, holy_script_execute_sourcecode_getline, &source);
      if (! parsed_script)
	{
	  ret = holy_errno;
	  holy_free (line);
	  break;
	}

      ret = holy_script_execute (parsed_script);
      holy_script_free (parsed_script);
      holy_free (line);
    }

  return ret;
}

/* Execute a source script in new scope.  */
holy_err_t
holy_script_execute_new_scope (const char *source, int argc, char **args)
{
  holy_err_t ret = 0;
  struct holy_script_scope new_scope;
  struct holy_script_scope *old_scope;

  new_scope.argv.argc = argc;
  new_scope.argv.args = args;
  new_scope.flags = 0;

  old_scope = scope;
  scope = &new_scope;

  ret = holy_script_execute_sourcecode (source);

  scope = old_scope;
  return ret;
}

/* Execute a single command line.  */
holy_err_t
holy_script_execute_cmdline (struct holy_script_cmd *cmd)
{
  struct holy_script_cmdline *cmdline = (struct holy_script_cmdline *) cmd;
  holy_command_t holycmd;
  holy_err_t ret = 0;
  holy_script_function_t func = 0;
  char errnobuf[18];
  char *cmdname, *cmdstring;
  int argc, offset = 0, cmdlen = 0;
  unsigned int i;
  char **args;
  int invert;
  struct holy_script_argv argv = { 0, 0, 0 };

  /* Lookup the command.  */
  if (holy_script_arglist_to_argv (cmdline->arglist, &argv) || ! argv.args[0])
    return holy_errno;

  for (i = 0; i < argv.argc; i++) {
	  cmdlen += holy_strlen (argv.args[i]) + 1;
  }

  cmdstring = holy_malloc (cmdlen);
  if (!cmdstring)
  {
	  return holy_error (holy_ERR_OUT_OF_MEMORY,
			     N_("cannot allocate command buffer"));
  }

  for (i = 0; i < argv.argc; i++) {
	  offset += holy_snprintf (cmdstring + offset, cmdlen - offset, "%s ",
				   argv.args[i]);
  }
  cmdstring[cmdlen-1]= '\0';
  holy_tpm_measure ((unsigned char *)cmdstring, cmdlen, holy_ASCII_PCR,
		    "holy_cmd", cmdstring);
  holy_print_error();
  holy_free(cmdstring);
  invert = 0;
  argc = argv.argc - 1;
  args = argv.args + 1;
  cmdname = argv.args[0];
  if (holy_strcmp (cmdname, "!") == 0)
    {
      if (argv.argc < 2 || ! argv.args[1])
	{
	  holy_script_argv_free (&argv);
	  return holy_error (holy_ERR_BAD_ARGUMENT,
			     N_("no command is specified"));
	}

      invert = 1;
      argc = argv.argc - 2;
      args = argv.args + 2;
      cmdname = argv.args[1];
    }
  holycmd = holy_command_find (cmdname);
  if (! holycmd)
    {
      holy_errno = holy_ERR_NONE;

      /* It's not a holy command, try all functions.  */
      func = holy_script_function_find (cmdname);
      if (! func)
	{
	  /* As a last resort, try if it is an assignment.  */
	  char *assign = holy_strdup (cmdname);
	  char *eq = holy_strchr (assign, '=');

	  if (eq)
	    {
	      /* This was set because the command was not found.  */
	      holy_errno = holy_ERR_NONE;

	      /* Create two strings and set the variable.  */
	      *eq = '\0';
	      eq++;
	      holy_script_env_set (assign, eq);
	    }
	  holy_free (assign);

	  holy_snprintf (errnobuf, sizeof (errnobuf), "%d", holy_errno);
	  holy_script_env_set ("?", errnobuf);

	  holy_script_argv_free (&argv);
	  holy_print_error ();

	  return 0;
	}
    }

  /* Execute the holy command or function.  */
  if (holycmd)
    {
      if (holy_extractor_level && !(holycmd->flags
				    & holy_COMMAND_FLAG_EXTRACTOR))
	ret = holy_error (holy_ERR_EXTRACTOR,
			  "%s isn't allowed to execute in an extractor",
			  cmdname);
      else if ((holycmd->flags & holy_COMMAND_FLAG_BLOCKS) &&
	       (holycmd->flags & holy_COMMAND_FLAG_EXTCMD))
	ret = holy_extcmd_dispatcher (holycmd, argc, args, argv.script);
      else
	ret = (holycmd->func) (holycmd, argc, args);
    }
  else
    ret = holy_script_function_call (func, argc, args);

  if (invert)
    {
      if (ret == holy_ERR_TEST_FAILURE)
	holy_errno = ret = holy_ERR_NONE;
      else if (ret == holy_ERR_NONE)
	ret = holy_error (holy_ERR_TEST_FAILURE, N_("false"));
      else
	{
	  holy_print_error ();
	  ret = holy_ERR_NONE;
	}
    }

  /* Free arguments.  */
  holy_script_argv_free (&argv);

  if (holy_errno == holy_ERR_TEST_FAILURE)
    holy_errno = holy_ERR_NONE;

  holy_print_error ();

  holy_snprintf (errnobuf, sizeof (errnobuf), "%d", ret);
  holy_env_set ("?", errnobuf);

  return ret;
}

/* Execute a block of one or more commands.  */
holy_err_t
holy_script_execute_cmdlist (struct holy_script_cmd *list)
{
  int ret = 0;
  struct holy_script_cmd *cmd;

  /* Loop over every command and execute it.  */
  for (cmd = list->next; cmd; cmd = cmd->next)
    {
      if (active_breaks)
	break;

      ret = holy_script_execute_cmd (cmd);

      if (function_return)
	break;
    }

  return ret;
}

/* Execute an if statement.  */
holy_err_t
holy_script_execute_cmdif (struct holy_script_cmd *cmd)
{
  int ret;
  const char *result;
  struct holy_script_cmdif *cmdif = (struct holy_script_cmdif *) cmd;

  /* Check if the commands results in a true or a false.  The value is
     read from the env variable `?'.  */
  ret = holy_script_execute_cmd (cmdif->exec_to_evaluate);
  if (function_return)
    return ret;

  result = holy_env_get ("?");
  holy_errno = holy_ERR_NONE;

  /* Execute the `if' or the `else' part depending on the value of
     `?'.  */
  if (result && ! holy_strcmp (result, "0"))
    return holy_script_execute_cmd (cmdif->exec_on_true);
  else
    return holy_script_execute_cmd (cmdif->exec_on_false);
}

/* Execute a for statement.  */
holy_err_t
holy_script_execute_cmdfor (struct holy_script_cmd *cmd)
{
  unsigned i;
  holy_err_t result;
  struct holy_script_argv argv = { 0, 0, 0 };
  struct holy_script_cmdfor *cmdfor = (struct holy_script_cmdfor *) cmd;

  if (holy_script_arglist_to_argv (cmdfor->words, &argv))
    return holy_errno;

  active_loops++;
  result = 0;
  for (i = 0; i < argv.argc; i++)
    {
      if (is_continue && active_breaks == 1)
	active_breaks = 0;

      if (! active_breaks)
	{
	  holy_script_env_set (cmdfor->name->str, argv.args[i]);
	  result = holy_script_execute_cmd (cmdfor->list);
	  if (function_return)
	    break;
	}
    }

  if (active_breaks)
    active_breaks--;

  active_loops--;
  holy_script_argv_free (&argv);
  return result;
}

/* Execute a "while" or "until" command.  */
holy_err_t
holy_script_execute_cmdwhile (struct holy_script_cmd *cmd)
{
  int result;
  struct holy_script_cmdwhile *cmdwhile = (struct holy_script_cmdwhile *) cmd;

  active_loops++;
  do {
    result = holy_script_execute_cmd (cmdwhile->cond);
    if (function_return)
      break;

    if (cmdwhile->until ? !result : result)
      break;

    result = holy_script_execute_cmd (cmdwhile->list);
    if (function_return)
      break;

    if (active_breaks == 1 && is_continue)
      active_breaks = 0;

    if (active_breaks)
      break;

  } while (1); /* XXX Put a check for ^C here */

  if (active_breaks)
    active_breaks--;

  active_loops--;
  return result;
}

/* Execute any holy pre-parsed command or script.  */
holy_err_t
holy_script_execute (struct holy_script *script)
{
  if (script == 0)
    return 0;

  return holy_script_execute_cmd (script->cmd);
}

