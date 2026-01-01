/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/normal.h>
#include <holy/device.h>
#include <holy/disk.h>
#include <holy/partition.h>
#include <holy/parttool.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static struct holy_parttool *parts = 0;
static int curhandle = 0;
static holy_dl_t mymod;
static char helpmsg[] =
  N_("Perform COMMANDS on partition.\n"
     "Use `parttool PARTITION help' for the list "
     "of available commands.");

int
holy_parttool_register(const char *part_name,
		       const holy_parttool_function_t func,
		       const struct holy_parttool_argdesc *args)
{
  struct holy_parttool *cur;
  int nargs = 0;

  if (! parts)
    holy_dl_ref (mymod);

  cur = (struct holy_parttool *) holy_malloc (sizeof (struct holy_parttool));
  cur->next = parts;
  cur->name = holy_strdup (part_name);
  cur->handle = curhandle++;
  for (nargs = 0; args[nargs].name != 0; nargs++);
  cur->nargs = nargs;
  cur->args = (struct holy_parttool_argdesc *)
    holy_malloc ((nargs + 1) * sizeof (struct holy_parttool_argdesc));
  holy_memcpy (cur->args, args,
	       (nargs + 1) * sizeof (struct holy_parttool_argdesc));

  cur->func = func;
  parts = cur;
  return cur->handle;
}

void
holy_parttool_unregister (int handle)
{
  struct holy_parttool *prev = 0, *cur, *t;
  for (cur = parts; cur; )
    if (cur->handle == handle)
      {
	holy_free (cur->args);
	holy_free (cur->name);
	if (prev)
	  prev->next = cur->next;
	else
	  parts = cur->next;
	t = cur;
	cur = cur->next;
	holy_free (t);
      }
    else
      {
	prev = cur;
	cur = cur->next;
      }
  if (! parts)
    holy_dl_unref (mymod);
}

static holy_err_t
show_help (holy_device_t dev)
{
  int found = 0;
  struct holy_parttool *cur;

  for (cur = parts; cur; cur = cur->next)
    if (holy_strcmp (dev->disk->partition->partmap->name, cur->name) == 0)
      {
	struct holy_parttool_argdesc *curarg;
	found = 1;
	for (curarg = cur->args; curarg->name; curarg++)
	  {
	    int spacing = 20;

	    spacing -= holy_strlen (curarg->name);
	    holy_printf ("%s", curarg->name);

	    switch (curarg->type)
	      {
	      case holy_PARTTOOL_ARG_BOOL:
		holy_printf ("+/-");
		spacing -= 3;
		break;

	      case holy_PARTTOOL_ARG_VAL:
		holy_xputs (_("=VAL"));
		spacing -= 4;
		break;

	      case holy_PARTTOOL_ARG_END:
		break;
	      }
	    while (spacing-- > 0)
	      holy_printf (" ");
	    holy_puts_ (curarg->desc);
	  }
      }
  if (! found)
    holy_printf_ (N_("Sorry, no parttool is available for %s\n"),
		  dev->disk->partition->partmap->name);
  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_parttool (holy_command_t cmd __attribute__ ((unused)),
		   int argc, char **args)
{
  holy_device_t dev;
  struct holy_parttool *cur, *ptool;
  int *parsed;
  int i, j;
  holy_err_t err = holy_ERR_NONE;

  if (argc < 1)
    {
      holy_puts_ (helpmsg);
      return holy_error (holy_ERR_BAD_ARGUMENT, "too few arguments");
    }

  if (args[0][0] == '(' && args[0][holy_strlen (args[0]) - 1] == ')')
    {
      args[0][holy_strlen (args[0]) - 1] = 0;
      dev = holy_device_open (args[0] + 1);
      args[0][holy_strlen (args[0]) - 1] = ')';
    }
  else
    dev = holy_device_open (args[0]);

  if (! dev)
    return holy_errno;

  if (! dev->disk)
    {
      holy_device_close (dev);
      return holy_error (holy_ERR_BAD_ARGUMENT, "not a disk");
    }

  if (! dev->disk->partition)
    {
      holy_device_close (dev);
      return holy_error (holy_ERR_BAD_ARGUMENT, "not a partition");
    }

  /* Load modules. */
  if (! holy_no_modules)
  {
    const char *prefix;
    prefix = holy_env_get ("prefix");
    if (prefix)
      {
	char *filename;

	filename = holy_xasprintf ("%s/" holy_TARGET_CPU "-" holy_PLATFORM
				   "/parttool.lst", prefix);
	if (filename)
	  {
	    holy_file_t file;

	    file = holy_file_open (filename);
	    if (file)
	      {
		char *buf = 0;
		for (;; holy_free(buf))
		  {
		    char *p, *name;

		    buf = holy_file_getline (file);

		    if (! buf)
		      break;

		    name = buf;
		    while (holy_isspace (name[0]))
		      name++;

		    if (! holy_isgraph (name[0]))
		      continue;

		    p = holy_strchr (name, ':');
		    if (! p)
		      continue;

		    *p = '\0';
		    p++;
		    while (*p == ' ' || *p == '\t')
		      p++;

		    if (! holy_isgraph (*p))
		      continue;

		    if (holy_strcmp (name, dev->disk->partition->partmap->name)
			!= 0)
		      continue;

		    holy_dl_load (p);
		  }

		holy_file_close (file);
	      }

	    holy_free (filename);
	  }
      }
    /* Ignore errors.  */
    holy_errno = holy_ERR_NONE;
  }

  if (argc == 1)
    {
      err = show_help (dev);
      holy_device_close (dev);
      return err;
    }

  for (i = 1; i < argc; i++)
    if (holy_strcmp (args[i], "help") == 0)
      {
	err = show_help (dev);
	holy_device_close (dev);
	return err;
      }

  parsed = (int *) holy_zalloc (argc * sizeof (int));

  for (i = 1; i < argc; i++)
    if (! parsed[i])
      {
	struct holy_parttool_argdesc *curarg;
	struct holy_parttool_args *pargs;
	for (cur = parts; cur; cur = cur->next)
	  if (holy_strcmp (dev->disk->partition->partmap->name, cur->name) == 0)
	    {
	      for (curarg = cur->args; curarg->name; curarg++)
		if (holy_strncmp (curarg->name, args[i],
				  holy_strlen (curarg->name)) == 0
		    && ((curarg->type == holy_PARTTOOL_ARG_BOOL
			 && (args[i][holy_strlen (curarg->name)] == '+'
			     || args[i][holy_strlen (curarg->name)] == '-'
			     || args[i][holy_strlen (curarg->name)] == 0))
			|| (curarg->type == holy_PARTTOOL_ARG_VAL
			    && args[i][holy_strlen (curarg->name)] == '=')))

		  break;
	      if (curarg->name)
		break;
	    }
	if (! cur)
	  {
	    holy_free (parsed);
	    holy_device_close (dev);
	    return holy_error (holy_ERR_BAD_ARGUMENT, N_("unknown argument `%s'"),
			     args[i]);
	  }
	ptool = cur;
	pargs = (struct holy_parttool_args *)
	  holy_zalloc (ptool->nargs * sizeof (struct holy_parttool_args));
	for (j = i; j < argc; j++)
	  if (! parsed[j])
	    {
	      for (curarg = ptool->args; curarg->name; curarg++)
		if (holy_strncmp (curarg->name, args[j],
				   holy_strlen (curarg->name)) == 0
		    && ((curarg->type == holy_PARTTOOL_ARG_BOOL
			 && (args[j][holy_strlen (curarg->name)] == '+'
			     || args[j][holy_strlen (curarg->name)] == '-'
			     || args[j][holy_strlen (curarg->name)] == 0))
			|| (curarg->type == holy_PARTTOOL_ARG_VAL
			    && args[j][holy_strlen (curarg->name)] == '=')))
		  {
		    parsed[j] = 1;
		    pargs[curarg - ptool->args].set = 1;
		    switch (curarg->type)
		      {
		      case holy_PARTTOOL_ARG_BOOL:
			pargs[curarg - ptool->args].bool
			  = (args[j][holy_strlen (curarg->name)] != '-');
			break;

		      case holy_PARTTOOL_ARG_VAL:
			pargs[curarg - ptool->args].str
			  = (args[j] + holy_strlen (curarg->name) + 1);
			break;

		      case holy_PARTTOOL_ARG_END:
			break;
		      }
		  }
	    }

	err = ptool->func (dev, pargs);
	holy_free (pargs);
	if (err)
	  break;
      }

  holy_free (parsed);
  holy_device_close (dev);
  return err;
}

static holy_command_t cmd;

holy_MOD_INIT(parttool)
{
  mymod = mod;
  cmd = holy_register_command ("parttool", holy_cmd_parttool,
			       N_("PARTITION COMMANDS"),
			       helpmsg);
}

holy_MOD_FINI(parttool)
{
  holy_unregister_command (cmd);
}
