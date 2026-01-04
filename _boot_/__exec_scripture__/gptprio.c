/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/device.h>
#include <holy/env.h>
#include <holy/err.h>
#include <holy/extcmd.h>
#include <holy/gpt_partition.h>
#include <holy/i18n.h>
#include <holy/misc.h>

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options_next[] = {
  {"set-device", 'd', 0,
   N_("Set a variable to the name of selected partition."),
   N_("VARNAME"), ARG_TYPE_STRING},
  {"set-uuid", 'u', 0,
   N_("Set a variable to the GPT UUID of selected partition."),
   N_("VARNAME"), ARG_TYPE_STRING},
  {0, 0, 0, 0, 0, 0}
};

enum options_next
{
  NEXT_SET_DEVICE,
  NEXT_SET_UUID,
};

static unsigned int
holy_gptprio_priority (struct holy_gpt_partentry *entry)
{
  return (unsigned int) holy_gpt_entry_attribute
    (entry, holy_GPT_PART_ATTR_OFFSET_GPTPRIO_PRIORITY, 4);
}

static unsigned int
holy_gptprio_tries_left (struct holy_gpt_partentry *entry)
{
  return (unsigned int) holy_gpt_entry_attribute
    (entry, holy_GPT_PART_ATTR_OFFSET_GPTPRIO_TRIES_LEFT, 4);
}

static void
holy_gptprio_set_tries_left (struct holy_gpt_partentry *entry,
			     unsigned int tries_left)
{
  holy_gpt_entry_set_attribute
    (entry, tries_left, holy_GPT_PART_ATTR_OFFSET_GPTPRIO_TRIES_LEFT, 4);
}

static unsigned int
holy_gptprio_successful (struct holy_gpt_partentry *entry)
{
  return (unsigned int) holy_gpt_entry_attribute
    (entry, holy_GPT_PART_ATTR_OFFSET_GPTPRIO_SUCCESSFUL, 1);
}

static holy_err_t
holy_find_next (const char *disk_name,
		const holy_gpt_part_type_t *part_type,
		char **part_name, char **part_guid)
{
  struct holy_gpt_partentry *part, *part_found = NULL;
  holy_device_t dev = NULL;
  holy_gpt_t gpt = NULL;
  holy_uint32_t i, part_index;

  dev = holy_device_open (disk_name);
  if (!dev)
    goto done;

  gpt = holy_gpt_read (dev->disk);
  if (!gpt)
    goto done;

  if (holy_gpt_repair (dev->disk, gpt))
    goto done;

  for (i = 0; (part = holy_gpt_get_partentry (gpt, i)) != NULL; i++)
    {
      if (holy_memcmp (part_type, &part->type, sizeof (*part_type)) == 0)
	{
	  unsigned int priority, tries_left, successful, old_priority = 0;

	  priority = holy_gptprio_priority (part);
	  tries_left = holy_gptprio_tries_left (part);
	  successful = holy_gptprio_successful (part);

	  if (part_found)
	    old_priority = holy_gptprio_priority (part_found);

	  if ((tries_left || successful) && priority > old_priority)
	    {
	      part_index = i;
	      part_found = part;
	    }
	}
    }

  if (!part_found)
    {
      holy_error (holy_ERR_UNKNOWN_DEVICE, N_("no such partition"));
      goto done;
    }

  if (holy_gptprio_tries_left (part_found))
    {
      unsigned int tries_left = holy_gptprio_tries_left (part_found);

      holy_gptprio_set_tries_left (part_found, tries_left - 1);

      if (holy_gpt_update (gpt))
	goto done;

      if (holy_gpt_write (dev->disk, gpt))
	goto done;
    }

  *part_name = holy_xasprintf ("%s,gpt%u", disk_name, part_index + 1);
  if (!*part_name)
    goto done;

  *part_guid = holy_gpt_guid_to_str (&part_found->guid);
  if (!*part_guid)
    goto done;

  holy_errno = holy_ERR_NONE;

done:
  holy_gpt_free (gpt);

  if (dev)
    holy_device_close (dev);

  return holy_errno;
}



static holy_err_t
holy_cmd_next (holy_extcmd_context_t ctxt, int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;
  char *p, *root = NULL, *part_name = NULL, *part_guid = NULL;

  /* TODO: Add a uuid parser and a command line flag for providing type.  */
  holy_gpt_part_type_t part_type = holy_GPT_PARTITION_TYPE_USR_X86_64;

  if (!state[NEXT_SET_DEVICE].set || !state[NEXT_SET_UUID].set)
    {
      holy_error (holy_ERR_INVALID_COMMAND, N_("-d and -u are required"));
      goto done;
    }

  if (argc == 0)
    root = holy_strdup (holy_env_get ("root"));
  else if (argc == 1)
    root = holy_strdup (args[0]);
  else
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("unexpected arguments"));
      goto done;
    }

  if (!root)
    goto done;

  /* To make using $root practical strip off the partition name.  */
  p = holy_strchr (root, ',');
  if (p)
    *p = '\0';

  if (holy_find_next (root, &part_type, &part_name, &part_guid))
    goto done;

  if (holy_env_set (state[NEXT_SET_DEVICE].arg, part_name))
    goto done;

  if (holy_env_set (state[NEXT_SET_UUID].arg, part_guid))
    goto done;

  holy_errno = holy_ERR_NONE;

done:
  holy_free (root);
  holy_free (part_name);
  holy_free (part_guid);

  return holy_errno;
}

static holy_extcmd_t cmd_next;

holy_MOD_INIT(gptprio)
{
  cmd_next = holy_register_extcmd ("gptprio.next", holy_cmd_next, 0,
				   N_("-d VARNAME -u VARNAME [DEVICE]"),
				   N_("Select next partition to boot."),
				   options_next);
}

holy_MOD_FINI(gptprio)
{
  holy_unregister_extcmd (cmd_next);
}
