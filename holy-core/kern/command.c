/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/command.h>

holy_command_t holy_command_list;

holy_command_t
holy_register_command_prio (const char *name,
			    holy_command_func_t func,
			    const char *summary,
			    const char *description,
			    int prio)
{
  holy_command_t cmd;
  int inactive = 0;

  holy_command_t *p, q;

  cmd = (holy_command_t) holy_zalloc (sizeof (*cmd));
  if (! cmd)
    return 0;

  cmd->name = name;
  cmd->func = func;
  cmd->summary = (summary) ? summary : "";
  cmd->description = description;

  cmd->flags = 0;
  cmd->prio = prio;
    
  for (p = &holy_command_list, q = *p; q; p = &(q->next), q = q->next)
    {
      int r;

      r = holy_strcmp (cmd->name, q->name);
      if (r < 0)
	break;
      if (r > 0)
	continue;

      if (cmd->prio >= (q->prio & holy_COMMAND_PRIO_MASK))
	{
	  q->prio &= ~holy_COMMAND_FLAG_ACTIVE;
	  break;
	}

      inactive = 1;
    }

  *p = cmd;
  cmd->next = q;
  if (q)
    q->prev = &cmd->next;
  cmd->prev = p;

  if (! inactive)
    cmd->prio |= holy_COMMAND_FLAG_ACTIVE;

  return cmd;
}

void
holy_unregister_command (holy_command_t cmd)
{
  if ((cmd->prio & holy_COMMAND_FLAG_ACTIVE) && (cmd->next))
    cmd->next->prio |= holy_COMMAND_FLAG_ACTIVE;
  holy_list_remove (holy_AS_LIST (cmd));
  holy_free (cmd);
}
