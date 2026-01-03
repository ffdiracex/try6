/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/env.h>
#include <holy/misc.h>
#include <holy/command.h>
#include <holy/normal.h>
#include <holy/extcmd.h>
#include <holy/script_sh.h>
#include <holy/i18n.h>

holy_command_t
holy_dyncmd_get_cmd (holy_command_t cmd)
{
  holy_extcmd_t extcmd = cmd->data;
  char *modname;
  char *name;
  holy_dl_t mod;

  modname = extcmd->data;
  mod = holy_dl_load (modname);
  if (!mod)
    return NULL;

  holy_free (modname);
  holy_dl_ref (mod);

  name = (char *) cmd->name;
  holy_unregister_extcmd (extcmd);

  cmd = holy_command_find (name);

  holy_free (name);

  return cmd;
}

static holy_err_t
holy_dyncmd_dispatcher (struct holy_extcmd_context *ctxt,
			int argc, char **args)
{
  char *modname;
  holy_dl_t mod;
  holy_err_t ret;
  holy_extcmd_t extcmd = ctxt->extcmd;
  holy_command_t cmd = extcmd->cmd;
  char *name;

  modname = extcmd->data;
  mod = holy_dl_load (modname);
  if (!mod)
    return holy_errno;

  holy_free (modname);
  holy_dl_ref (mod);

  name = (char *) cmd->name;
  holy_unregister_extcmd (extcmd);

  cmd = holy_command_find (name);
  if (cmd)
    {
      if (cmd->flags & holy_COMMAND_FLAG_BLOCKS &&
	  cmd->flags & holy_COMMAND_FLAG_EXTCMD)
	ret = holy_extcmd_dispatcher (cmd, argc, args, ctxt->script);
      else
	ret = (cmd->func) (cmd, argc, args);
    }
  else
    ret = holy_errno;

  holy_free (name);

  return ret;
}

/* Read the file command.lst for auto-loading.  */
void
read_command_list (const char *prefix)
{
  if (prefix)
    {
      char *filename;

      filename = holy_xasprintf ("%s/" holy_TARGET_CPU "-" holy_PLATFORM
				 "/command.lst", prefix);
      if (filename)
	{
	  holy_file_t file;

	  file = holy_file_open (filename);
	  if (file)
	    {
	      char *buf = NULL;
	      holy_command_t ptr, last = 0, next;

	      /* Override previous commands.lst.  */
	      for (ptr = holy_command_list; ptr; ptr = next)
		{
		  next = ptr->next;
		  if (ptr->flags & holy_COMMAND_FLAG_DYNCMD)
		    {
		      if (last)
			last->next = ptr->next;
		      else
			holy_command_list = ptr->next;
		      holy_free (ptr->data); /* extcmd struct */
		      holy_free (ptr);
		    }
		  else
		    last = ptr;
		}

	      for (;; holy_free (buf))
		{
		  char *p, *name, *modname;
		  holy_extcmd_t cmd;
		  int prio = 0;

		  buf = holy_file_getline (file);

		  if (! buf)
		    break;

		  name = buf;
		  while (holy_isspace (name[0]))
		    name++;

		  if (*name == '*')
		    {
		      name++;
		      prio++;
		    }

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

		  if (holy_dl_get (p))
		    continue;

		  name = holy_strdup (name);
		  if (! name)
		    continue;

		  modname = holy_strdup (p);
		  if (! modname)
		    {
		      holy_free (name);
		      continue;
		    }

		  cmd = holy_register_extcmd_prio (name,
						   holy_dyncmd_dispatcher,
						   holy_COMMAND_FLAG_BLOCKS
						   | holy_COMMAND_FLAG_EXTCMD
						   | holy_COMMAND_FLAG_DYNCMD,
						   0, N_("module isn't loaded"),
						   0, prio);
		  if (! cmd)
		    {
		      holy_free (name);
		      holy_free (modname);
		      continue;
		    }
		  cmd->data = modname;

		  /* Update the active flag.  */
		  holy_command_find (name);
		}

	      holy_file_close (file);
	    }

	  holy_free (filename);
	}
    }

  /* Ignore errors.  */
  holy_errno = holy_ERR_NONE;
}
