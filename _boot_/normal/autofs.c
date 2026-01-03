/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/dl.h>
#include <holy/env.h>
#include <holy/misc.h>
#include <holy/fs.h>
#include <holy/normal.h>

/* This is used to store the names of filesystem modules for auto-loading.  */
static holy_named_list_t fs_module_list;

/* The auto-loading hook for filesystems.  */
static int
autoload_fs_module (void)
{
  holy_named_list_t p;
  int ret = 0;
  holy_file_filter_t holy_file_filters_was[holy_FILE_FILTER_MAX];

  holy_memcpy (holy_file_filters_was, holy_file_filters_enabled,
	       sizeof (holy_file_filters_enabled));
  holy_memcpy (holy_file_filters_enabled, holy_file_filters_all,
	       sizeof (holy_file_filters_enabled));

  while ((p = fs_module_list) != NULL)
    {
      if (! holy_dl_get (p->name) && holy_dl_load (p->name))
	{
	  ret = 1;
	  break;
	}

      if (holy_errno)
	holy_print_error ();

      fs_module_list = p->next;
      holy_free (p->name);
      holy_free (p);
    }

  holy_memcpy (holy_file_filters_enabled, holy_file_filters_was,
	       sizeof (holy_file_filters_enabled));

  return ret;
}

/* Read the file fs.lst for auto-loading.  */
void
read_fs_list (const char *prefix)
{
  if (prefix)
    {
      char *filename;

      filename = holy_xasprintf ("%s/" holy_TARGET_CPU "-" holy_PLATFORM
				 "/fs.lst", prefix);
      if (filename)
	{
	  holy_file_t file;
	  holy_fs_autoload_hook_t tmp_autoload_hook;

	  /* This rules out the possibility that read_fs_list() is invoked
	     recursively when we call holy_file_open() below.  */
	  tmp_autoload_hook = holy_fs_autoload_hook;
	  holy_fs_autoload_hook = NULL;

	  file = holy_file_open (filename);
	  if (file)
	    {
	      /* Override previous fs.lst.  */
	      while (fs_module_list)
		{
		  holy_named_list_t tmp;
		  tmp = fs_module_list->next;
		  holy_free (fs_module_list);
		  fs_module_list = tmp;
		}

	      while (1)
		{
		  char *buf;
		  char *p;
		  char *q;
		  holy_named_list_t fs_mod;

		  buf = holy_file_getline (file);
		  if (! buf)
		    break;

		  p = buf;
		  q = buf + holy_strlen (buf) - 1;

		  /* Ignore space.  */
		  while (holy_isspace (*p))
		    p++;

		  while (p < q && holy_isspace (*q))
		    *q-- = '\0';

		  /* If the line is empty, skip it.  */
		  if (p >= q)
		    {
		      holy_free (buf);
		      continue;
		    }

		  fs_mod = holy_malloc (sizeof (*fs_mod));
		  if (! fs_mod)
		    {
		      holy_free (buf);
		      continue;
		    }

		  fs_mod->name = holy_strdup (p);
		  holy_free (buf);
		  if (! fs_mod->name)
		    {
		      holy_free (fs_mod);
		      continue;
		    }

		  fs_mod->next = fs_module_list;
		  fs_module_list = fs_mod;
		}

	      holy_file_close (file);
	      holy_fs_autoload_hook = tmp_autoload_hook;
	    }

	  holy_free (filename);
	}
    }

  /* Ignore errors.  */
  holy_errno = holy_ERR_NONE;

  /* Set the hook.  */
  holy_fs_autoload_hook = autoload_fs_module;
}
