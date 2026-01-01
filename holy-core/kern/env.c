/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/env.h>
#include <holy/env_private.h>
#include <holy/misc.h>
#include <holy/mm.h>

/* The initial context.  */
static struct holy_env_context initial_context;

/* The current context.  */
struct holy_env_context *holy_current_context = &initial_context;

/* Return the hash representation of the string S.  */
static unsigned int
holy_env_hashval (const char *s)
{
  unsigned int i = 0;

  /* XXX: This can be done much more efficiently.  */
  while (*s)
    i += 5 * *(s++);

  return i % HASHSZ;
}

static struct holy_env_var *
holy_env_find (const char *name)
{
  struct holy_env_var *var;
  int idx = holy_env_hashval (name);

  /* Look for the variable in the current context.  */
  for (var = holy_current_context->vars[idx]; var; var = var->next)
    if (holy_strcmp (var->name, name) == 0)
      return var;

  return 0;
}

static void
holy_env_insert (struct holy_env_context *context,
		 struct holy_env_var *var)
{
  int idx = holy_env_hashval (var->name);

  /* Insert the variable into the hashtable.  */
  var->prevp = &context->vars[idx];
  var->next = context->vars[idx];
  if (var->next)
    var->next->prevp = &(var->next);
  context->vars[idx] = var;
}

static void
holy_env_remove (struct holy_env_var *var)
{
  /* Remove the entry from the variable table.  */
  *var->prevp = var->next;
  if (var->next)
    var->next->prevp = var->prevp;
}

holy_err_t
holy_env_set (const char *name, const char *val)
{
  struct holy_env_var *var;

  /* If the variable does already exist, just update the variable.  */
  var = holy_env_find (name);
  if (var)
    {
      char *old = var->value;

      if (var->write_hook)
	var->value = var->write_hook (var, val);
      else
	var->value = holy_strdup (val);

      if (! var->value)
	{
	  var->value = old;
	  return holy_errno;
	}

      holy_free (old);
      return holy_ERR_NONE;
    }

  /* The variable does not exist, so create a new one.  */
  var = holy_zalloc (sizeof (*var));
  if (! var)
    return holy_errno;

  var->name = holy_strdup (name);
  if (! var->name)
    goto fail;

  var->value = holy_strdup (val);
  if (! var->value)
    goto fail;

  holy_env_insert (holy_current_context, var);

  return holy_ERR_NONE;

 fail:
  holy_free (var->name);
  holy_free (var->value);
  holy_free (var);

  return holy_errno;
}

const char *
holy_env_get (const char *name)
{
  struct holy_env_var *var;

  var = holy_env_find (name);
  if (! var)
    return 0;

  if (var->read_hook)
    return var->read_hook (var, var->value);

  return var->value;
}

void
holy_env_unset (const char *name)
{
  struct holy_env_var *var;

  var = holy_env_find (name);
  if (! var)
    return;

  if (var->read_hook || var->write_hook)
    {
      holy_env_set (name, "");
      return;
    }

  holy_env_remove (var);

  holy_free (var->name);
  holy_free (var->value);
  holy_free (var);
}

struct holy_env_var *
holy_env_update_get_sorted (void)
{
  struct holy_env_var *sorted_list = 0;
  int i;

  /* Add variables associated with this context into a sorted list.  */
  for (i = 0; i < HASHSZ; i++)
    {
      struct holy_env_var *var;

      for (var = holy_current_context->vars[i]; var; var = var->next)
	{
	  struct holy_env_var *p, **q;

	  for (q = &sorted_list, p = *q; p; q = &((*q)->sorted_next), p = *q)
	    {
	      if (holy_strcmp (p->name, var->name) > 0)
		break;
	    }

	  var->sorted_next = *q;
	  *q = var;
	}
    }

  return sorted_list;
}

holy_err_t
holy_register_variable_hook (const char *name,
			     holy_env_read_hook_t read_hook,
			     holy_env_write_hook_t write_hook)
{
  struct holy_env_var *var = holy_env_find (name);

  if (! var)
    {
      if (holy_env_set (name, "") != holy_ERR_NONE)
	return holy_errno;

      var = holy_env_find (name);
      /* XXX Insert an assertion?  */
    }

  var->read_hook = read_hook;
  var->write_hook = write_hook;

  return holy_ERR_NONE;
}

holy_err_t
holy_env_export (const char *name)
{
  struct holy_env_var *var;

  var = holy_env_find (name);
  if (! var)
    {
      holy_err_t err;
      
      err = holy_env_set (name, "");
      if (err)
	return err;
      var = holy_env_find (name);
    }    
  var->global = 1;

  return holy_ERR_NONE;
}
