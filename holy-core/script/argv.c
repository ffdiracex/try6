/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/script_sh.h>

/* Return nearest power of two that is >= v.  */
static unsigned
round_up_exp (unsigned v)
{
  COMPILE_TIME_ASSERT (sizeof (v) == 4);

  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;

  v++;
  v += (v == 0);

  return v;
}

void
holy_script_argv_free (struct holy_script_argv *argv)
{
  unsigned i;

  if (argv->args)
    {
      for (i = 0; i < argv->argc; i++)
	holy_free (argv->args[i]);

      holy_free (argv->args);
    }

  argv->argc = 0;
  argv->args = 0;
  argv->script = 0;
}

/* Make argv from argc, args pair.  */
int
holy_script_argv_make (struct holy_script_argv *argv, int argc, char **args)
{
  int i;
  struct holy_script_argv r = { 0, 0, 0 };

  for (i = 0; i < argc; i++)
    if (holy_script_argv_next (&r)
	|| holy_script_argv_append (&r, args[i], holy_strlen (args[i])))
      {
	holy_script_argv_free (&r);
	return 1;
      }
  *argv = r;
  return 0;
}

/* Prepare for next argc.  */
int
holy_script_argv_next (struct holy_script_argv *argv)
{
  char **p = argv->args;

  if (argv->args && argv->argc && argv->args[argv->argc - 1] == 0)
    return 0;

  p = holy_realloc (p, round_up_exp ((argv->argc + 2) * sizeof (char *)));
  if (! p)
    return 1;

  argv->argc++;
  argv->args = p;

  if (argv->argc == 1)
    argv->args[0] = 0;
  argv->args[argv->argc] = 0;
  return 0;
}

/* Append `s' to the last argument.  */
int
holy_script_argv_append (struct holy_script_argv *argv, const char *s,
			 holy_size_t slen)
{
  holy_size_t a;
  char *p = argv->args[argv->argc - 1];

  if (! s)
    return 0;

  a = p ? holy_strlen (p) : 0;

  p = holy_realloc (p, round_up_exp ((a + slen + 1) * sizeof (char)));
  if (! p)
    return 1;

  holy_memcpy (p + a, s, slen);
  p[a+slen] = 0;
  argv->args[argv->argc - 1] = p;

  return 0;
}

/* Split `s' and append words as multiple arguments.  */
int
holy_script_argv_split_append (struct holy_script_argv *argv, const char *s)
{
  const char *p;
  int errors = 0;

  if (! s)
    return 0;

  while (*s && holy_isspace (*s))
    s++;

  while (! errors && *s)
    {
      p = s;
      while (*s && ! holy_isspace (*s))
	s++;

      errors += holy_script_argv_append (argv, p, s - p);

      while (*s && holy_isspace (*s))
	s++;

      if (*s)
	errors += holy_script_argv_next (argv);
    }
  return errors;
}
