/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/script_sh.h>
#include <holy/parser.h>
#include <holy/mm.h>
#include <holy/charset.h>

holy_script_function_t holy_script_function_list;

holy_script_function_t
holy_script_function_create (struct holy_script_arg *functionname_arg,
			     struct holy_script *cmd)
{
  holy_script_function_t func;
  holy_script_function_t *p;

  func = (holy_script_function_t) holy_malloc (sizeof (*func));
  if (! func)
    return 0;

  func->name = holy_strdup (functionname_arg->str);
  if (! func->name)
    {
      holy_free (func);
      return 0;
    }

  func->func = cmd;

  /* Keep the list sorted for simplicity.  */
  p = &holy_script_function_list;
  while (*p)
    {
      if (holy_strcmp ((*p)->name, func->name) >= 0)
	break;

      p = &((*p)->next);
    }

  /* If the function already exists, overwrite the old function.  */
  if (*p && holy_strcmp ((*p)->name, func->name) == 0)
    {
      holy_script_function_t q;

      q = *p;
      holy_script_free (q->func);
      q->func = cmd;
      holy_free (func);
      func = q;
    }
  else
    {
      func->next = *p;
      *p = func;
    }

  return func;
}

void
holy_script_function_remove (const char *name)
{
  holy_script_function_t *p, q;

  for (p = &holy_script_function_list, q = *p; q; p = &(q->next), q = q->next)
    if (holy_strcmp (name, q->name) == 0)
      {
        *p = q->next;
	holy_free (q->name);
	holy_script_free (q->func);
        holy_free (q);
        break;
      }
}

holy_script_function_t
holy_script_function_find (char *functionname)
{
  holy_script_function_t func;

  for (func = holy_script_function_list; func; func = func->next)
    if (holy_strcmp (functionname, func->name) == 0)
      break;

  if (! func)
    {
      char tmp[21];
      holy_strncpy (tmp, functionname, 20);
      tmp[20] = 0;
      /* Avoid truncating inside UTF-8 character.  */
      tmp[holy_getend (tmp, tmp + holy_strlen (tmp))] = 0;
      holy_error (holy_ERR_UNKNOWN_COMMAND, N_("can't find command `%s'"), tmp);
    }

  return func;
}
