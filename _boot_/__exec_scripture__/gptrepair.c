/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/command.h>
#include <holy/device.h>
#include <holy/err.h>
#include <holy/gpt_partition.h>
#include <holy/i18n.h>
#include <holy/misc.h>

holy_MOD_LICENSE ("GPLv2+");

static char *
trim_dev_name (char *name)
{
  holy_size_t len = holy_strlen (name);
  if (len && name[0] == '(' && name[len - 1] == ')')
    {
      name[len - 1] = '\0';
      name = name + 1;
    }
  return name;
}

static holy_err_t
holy_cmd_gptrepair (holy_command_t cmd __attribute__ ((unused)),
		    int argc, char **args)
{
  holy_device_t dev = NULL;
  holy_gpt_t gpt = NULL;
  char *dev_name;

  if (argc != 1 || !holy_strlen(args[0]))
    return holy_error (holy_ERR_BAD_ARGUMENT, "device name required");

  dev_name = trim_dev_name (args[0]);
  dev = holy_device_open (dev_name);
  if (!dev)
    goto done;

  if (!dev->disk)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, "not a disk");
      goto done;
    }

  gpt = holy_gpt_read (dev->disk);
  if (!gpt)
    goto done;

  if (holy_gpt_both_valid (gpt))
    {
      holy_printf_ (N_("GPT already valid, %s unmodified.\n"), dev_name);
      goto done;
    }

  if (!holy_gpt_primary_valid (gpt))
    holy_printf_ (N_("Found invalid primary GPT on %s\n"), dev_name);

  if (!holy_gpt_backup_valid (gpt))
    holy_printf_ (N_("Found invalid backup GPT on %s\n"), dev_name);

  if (holy_gpt_repair (dev->disk, gpt))
    goto done;

  if (holy_gpt_write (dev->disk, gpt))
    goto done;

  holy_printf_ (N_("Repaired GPT on %s\n"), dev_name);

done:
  if (gpt)
    holy_gpt_free (gpt);

  if (dev)
    holy_device_close (dev);

  return holy_errno;
}

static holy_command_t cmd;

holy_MOD_INIT(gptrepair)
{
  cmd = holy_register_command ("gptrepair", holy_cmd_gptrepair,
			       N_("DEVICE"),
			       N_("Verify and repair GPT on drive DEVICE."));
}

holy_MOD_FINI(gptrepair)
{
  holy_unregister_command (cmd);
}
