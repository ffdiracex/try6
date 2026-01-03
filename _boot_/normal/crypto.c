/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/env.h>
#include <holy/misc.h>
#include <holy/crypto.h>
#include <holy/normal.h>

struct load_spec
{
  struct load_spec *next;
  char *name;
  char *modname;
};

static struct load_spec *crypto_specs = NULL;

static void 
holy_crypto_autoload (const char *name)
{
  struct load_spec *cur;
  holy_dl_t mod;
  static int depth = 0;

  /* Some bufio of filesystems may want some crypto modules.
     It may result in infinite recursion. Hence this check.  */
  if (depth)
    return;
  depth++;

  for (cur = crypto_specs; cur; cur = cur->next)
    if (holy_strcasecmp (name, cur->name) == 0)
      {
	mod = holy_dl_load (cur->modname);
	if (mod)
	  holy_dl_ref (mod);
	holy_errno = holy_ERR_NONE;
      }
  depth--;
}

static void 
holy_crypto_spec_free (void)
{
  struct load_spec *cur, *next;
  for (cur = crypto_specs; cur; cur = next)
    {
      next = cur->next;
      holy_free (cur->name);
      holy_free (cur->modname);
      holy_free (cur);
    }
  crypto_specs = NULL;
}


/* Read the file crypto.lst for auto-loading.  */
void
read_crypto_list (const char *prefix)
{
  char *filename;
  holy_file_t file;
  char *buf = NULL;

  if (!prefix)
    {
      holy_errno = holy_ERR_NONE;
      return;
    }
  
  filename = holy_xasprintf ("%s/" holy_TARGET_CPU "-" holy_PLATFORM
			     "/crypto.lst", prefix);
  if (!filename)
    {
      holy_errno = holy_ERR_NONE;
      return;
    }

  file = holy_file_open (filename);
  holy_free (filename);
  if (!file)
    {
      holy_errno = holy_ERR_NONE;
      return;
    }

  /* Override previous crypto.lst.  */
  holy_crypto_spec_free ();

  for (;; holy_free (buf))
    {
      char *p, *name;
      struct load_spec *cur;
      
      buf = holy_file_getline (file);
	
      if (! buf)
	break;
      
      name = buf;
      while (holy_isspace (name[0]))
	name++;

      p = holy_strchr (name, ':');
      if (! p)
	continue;
      
      *p = '\0';
      p++;
      while (*p == ' ' || *p == '\t')
	p++;

      cur = holy_malloc (sizeof (*cur));
      if (!cur)
	{
	  holy_errno = holy_ERR_NONE;
	  continue;
	}
      
      cur->name = holy_strdup (name);
      if (! cur->name)
	{
	  holy_errno = holy_ERR_NONE;
	  holy_free (cur);
	  continue;
	}
	
      cur->modname = holy_strdup (p);
      if (! cur->modname)
	{
	  holy_errno = holy_ERR_NONE;
	  holy_free (cur->name);
	  holy_free (cur);
	  continue;
	}
      cur->next = crypto_specs;
      crypto_specs = cur;
    }
  
  holy_file_close (file);

  holy_errno = holy_ERR_NONE;

  holy_crypto_autoload_hook = holy_crypto_autoload;
}
