/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/file.h>
#include <holy/time.h>
#include <holy/misc.h>
#include <holy/dl.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>
#include <holy/normal.h>

holy_MOD_LICENSE ("GPLv2+");

#define DEFAULT_BLOCK_SIZE	65536

static const struct holy_arg_option options[] =
  {
    {"size", 's', 0, N_("Specify size for each read operation"), 0, ARG_TYPE_INT},
    {0, 0, 0, 0, 0, 0}
  };

static holy_err_t
holy_cmd_testspeed (holy_extcmd_context_t ctxt, int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;
  holy_uint64_t start;
  holy_uint64_t end;
  holy_ssize_t block_size;
  holy_disk_addr_t total_size;
  char *buffer;
  holy_file_t file;
  holy_uint64_t whole, fraction;

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  block_size = (state[0].set) ?
    holy_strtoul (state[0].arg, 0, 0) : DEFAULT_BLOCK_SIZE;

  if (block_size <= 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("invalid block size"));

  buffer = holy_malloc (block_size);
  if (buffer == NULL)
    return holy_errno;

  file = holy_file_open (args[0]);
  if (file == NULL)
    goto quit;

  total_size = 0;
  start = holy_get_time_ms ();
  while (1)
    {
      holy_ssize_t size = holy_file_read (file, buffer, block_size);
      if (size <= 0)
	break;
      total_size += size;
    }
  end = holy_get_time_ms ();
  holy_file_close (file);

  holy_printf_ (N_("File size: %s\n"),
		holy_get_human_size (total_size, holy_HUMAN_SIZE_NORMAL));
  whole = holy_divmod64 (end - start, 1000, &fraction);
  holy_printf_ (N_("Elapsed time: %d.%03d s \n"),
		(unsigned) whole,
		(unsigned) fraction);

  if (end != start)
    {
      holy_uint64_t speed =
	holy_divmod64 (total_size * 100ULL * 1000ULL, end - start, 0);

      holy_printf_ (N_("Speed: %s \n"),
		    holy_get_human_size (speed,
					 holy_HUMAN_SIZE_SPEED));
    }

 quit:
  holy_free (buffer);

  return holy_errno;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(testspeed)
{
  cmd = holy_register_extcmd ("testspeed", holy_cmd_testspeed, 0, N_("[-s SIZE] FILENAME"),
			      N_("Test file read speed."),
			      options);
}

holy_MOD_FINI(testspeed)
{
  holy_unregister_extcmd (cmd);
}
