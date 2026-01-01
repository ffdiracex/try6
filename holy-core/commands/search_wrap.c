/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/env.h>
#include <holy/extcmd.h>
#include <holy/search.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options[] =
  {
    {"file",		'f', 0, N_("Search devices by a file."), 0, 0},
    {"label",		'l', 0, N_("Search devices by a filesystem label."),
     0, 0},
    {"fs-uuid",		'u', 0, N_("Search devices by a filesystem UUID."),
     0, 0},
    {"part-label",	'L', 0, N_("Search devices by a partition label."),
     0, 0},
    {"part-uuid",	'U', 0, N_("Search devices by a partition UUID."),
     0, 0},
    {"disk-uuid",	'U', 0, N_("Search devices by a disk UUID."),
     0, 0},
    {"set",		's', holy_ARG_OPTION_OPTIONAL,
     N_("Set a variable to the first device found."), N_("VARNAME"),
     ARG_TYPE_STRING},
    {"no-floppy",	'n', 0, N_("Do not probe any floppy drive."), 0, 0},
    {"hint",	        'h', holy_ARG_OPTION_REPEATABLE,
     N_("First try the device HINT. If HINT ends in comma, "
	"also try subpartitions"), N_("HINT"), ARG_TYPE_STRING},
    {"hint-ieee1275",   0, holy_ARG_OPTION_REPEATABLE,
     N_("First try the device HINT if currently running on IEEE1275. "
	"If HINT ends in comma, also try subpartitions"),
     N_("HINT"), ARG_TYPE_STRING},
    {"hint-bios",   0, holy_ARG_OPTION_REPEATABLE,
     N_("First try the device HINT if currently running on BIOS. "
	"If HINT ends in comma, also try subpartitions"),
     N_("HINT"), ARG_TYPE_STRING},
    {"hint-baremetal",   0, holy_ARG_OPTION_REPEATABLE,
     N_("First try the device HINT if direct hardware access is supported. "
	"If HINT ends in comma, also try subpartitions"),
     N_("HINT"), ARG_TYPE_STRING},
    {"hint-efi",   0, holy_ARG_OPTION_REPEATABLE,
     N_("First try the device HINT if currently running on EFI. "
	"If HINT ends in comma, also try subpartitions"),
     N_("HINT"), ARG_TYPE_STRING},
    {"hint-arc",   0, holy_ARG_OPTION_REPEATABLE,
     N_("First try the device HINT if currently running on ARC."
	" If HINT ends in comma, also try subpartitions"),
     N_("HINT"), ARG_TYPE_STRING},
    {0, 0, 0, 0, 0, 0}
  };

enum options
  {
    SEARCH_FILE,
    SEARCH_LABEL,
    SEARCH_FS_UUID,
    SEARCH_PART_LABEL,
    SEARCH_PART_UUID,
    SEARCH_DISK_UUID,
    SEARCH_SET,
    SEARCH_NO_FLOPPY,
    SEARCH_HINT,
    SEARCH_HINT_IEEE1275,
    SEARCH_HINT_BIOS,
    SEARCH_HINT_BAREMETAL,
    SEARCH_HINT_EFI,
    SEARCH_HINT_ARC,
 };

static holy_err_t
holy_cmd_search (holy_extcmd_context_t ctxt, int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;
  const char *var = 0;
  const char *id = 0;
  int i = 0, j = 0, nhints = 0;
  char **hints = NULL;

  if (state[SEARCH_HINT].set)
    for (i = 0; state[SEARCH_HINT].args[i]; i++)
      nhints++;

#ifdef holy_MACHINE_IEEE1275
  if (state[SEARCH_HINT_IEEE1275].set)
    for (i = 0; state[SEARCH_HINT_IEEE1275].args[i]; i++)
      nhints++;
#endif

#ifdef holy_MACHINE_EFI
  if (state[SEARCH_HINT_EFI].set)
    for (i = 0; state[SEARCH_HINT_EFI].args[i]; i++)
      nhints++;
#endif

#ifdef holy_MACHINE_PCBIOS
  if (state[SEARCH_HINT_BIOS].set)
    for (i = 0; state[SEARCH_HINT_BIOS].args[i]; i++)
      nhints++;
#endif

#ifdef holy_MACHINE_ARC
  if (state[SEARCH_HINT_ARC].set)
    for (i = 0; state[SEARCH_HINT_ARC].args[i]; i++)
      nhints++;
#endif

  if (state[SEARCH_HINT_BAREMETAL].set)
    for (i = 0; state[SEARCH_HINT_BAREMETAL].args[i]; i++)
      nhints++;

  hints = holy_malloc (sizeof (hints[0]) * nhints);
  if (!hints)
    return holy_errno;
  j = 0;

  if (state[SEARCH_HINT].set)
    for (i = 0; state[SEARCH_HINT].args[i]; i++)
      hints[j++] = state[SEARCH_HINT].args[i];

#ifdef holy_MACHINE_IEEE1275
  if (state[SEARCH_HINT_IEEE1275].set)
    for (i = 0; state[SEARCH_HINT_IEEE1275].args[i]; i++)
      hints[j++] = state[SEARCH_HINT_IEEE1275].args[i];
#endif

#ifdef holy_MACHINE_EFI
  if (state[SEARCH_HINT_EFI].set)
    for (i = 0; state[SEARCH_HINT_EFI].args[i]; i++)
      hints[j++] = state[SEARCH_HINT_EFI].args[i];
#endif

#ifdef holy_MACHINE_ARC
  if (state[SEARCH_HINT_ARC].set)
    for (i = 0; state[SEARCH_HINT_ARC].args[i]; i++)
      hints[j++] = state[SEARCH_HINT_ARC].args[i];
#endif

#ifdef holy_MACHINE_PCBIOS
  if (state[SEARCH_HINT_BIOS].set)
    for (i = 0; state[SEARCH_HINT_BIOS].args[i]; i++)
      hints[j++] = state[SEARCH_HINT_BIOS].args[i];
#endif

  if (state[SEARCH_HINT_BAREMETAL].set)
    for (i = 0; state[SEARCH_HINT_BAREMETAL].args[i]; i++)
      hints[j++] = state[SEARCH_HINT_BAREMETAL].args[i];

  /* Skip hints for future platforms.  */
  for (j = 0; j < argc; j++)
    if (holy_memcmp (args[j], "--hint-", sizeof ("--hint-") - 1) != 0)
      break;

  if (state[SEARCH_SET].set)
    var = state[SEARCH_SET].arg ? state[SEARCH_SET].arg : "root";

  if (argc != j)
    id = args[j];
  else if (state[SEARCH_SET].set && state[SEARCH_SET].arg)
    {
      id = state[SEARCH_SET].arg;
      var = "root";
    }
  else
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));
      goto out;
    }

  if (state[SEARCH_LABEL].set)
    holy_search_label (id, var, state[SEARCH_NO_FLOPPY].set,
		       hints, nhints);
  else if (state[SEARCH_FS_UUID].set)
    holy_search_fs_uuid (id, var, state[SEARCH_NO_FLOPPY].set,
			 hints, nhints);
  else if (state[SEARCH_PART_LABEL].set)
    holy_search_part_label (id, var, state[SEARCH_NO_FLOPPY].set,
			    hints, nhints);
  else if (state[SEARCH_PART_UUID].set)
    holy_search_part_uuid (id, var, state[SEARCH_NO_FLOPPY].set,
			   hints, nhints);
  else if (state[SEARCH_DISK_UUID].set)
    holy_search_disk_uuid (id, var, state[SEARCH_NO_FLOPPY].set,
			   hints, nhints);
  else if (state[SEARCH_FILE].set)
    holy_search_fs_file (id, var, state[SEARCH_NO_FLOPPY].set,
			 hints, nhints);
  else
    holy_error (holy_ERR_INVALID_COMMAND, "unspecified search type");

out:
  holy_free (hints);
  return holy_errno;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(search)
{
  cmd =
    holy_register_extcmd ("search", holy_cmd_search,
			  holy_COMMAND_FLAG_EXTRACTOR | holy_COMMAND_ACCEPT_DASH,
			  N_("[-f|-l|-u|-s|-n] [--hint HINT [--hint HINT] ...]"
			     " NAME"),
			  N_("Search devices by file, filesystem label"
			     " or filesystem UUID."
			     " If --set is specified, the first device found is"
			     " set to a variable. If no variable name is"
			     " specified, `root' is used."),
			  options);
}

holy_MOD_FINI(search)
{
  holy_unregister_extcmd (cmd);
}
