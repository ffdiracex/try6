/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/file.h>
#include <holy/mm.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

#define BUFFER_SIZE 512

static holy_err_t
holy_cmd_cmp (holy_command_t cmd __attribute__ ((unused)),
	      int argc, char **args)
{
  holy_ssize_t rd1, rd2;
  holy_off_t pos;
  holy_file_t file1 = 0;
  holy_file_t file2 = 0;
  char *buf1 = 0;
  char *buf2 = 0;

  if (argc != 2)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("two arguments expected"));

  holy_printf_ (N_("Compare file `%s' with `%s':\n"), args[0],
		args[1]);

  file1 = holy_file_open (args[0]);
  file2 = holy_file_open (args[1]);
  if (! file1 || ! file2)
    goto cleanup;

  if (holy_file_size (file1) != holy_file_size (file2))
    holy_printf_ (N_("Files differ in size: %llu [%s], %llu [%s]\n"),
		  (unsigned long long) holy_file_size (file1), args[0],
		  (unsigned long long) holy_file_size (file2), args[1]);
  else
    {
      pos = 0;

      buf1 = holy_malloc (BUFFER_SIZE);
      buf2 = holy_malloc (BUFFER_SIZE);

      if (! buf1 || ! buf2)
        goto cleanup;

      do
	{
	  int i;

	  rd1 = holy_file_read (file1, buf1, BUFFER_SIZE);
	  rd2 = holy_file_read (file2, buf2, BUFFER_SIZE);

	  if (rd1 != rd2)
	    goto cleanup;

	  for (i = 0; i < rd2; i++)
	    {
	      if (buf1[i] != buf2[i])
		{
		  holy_printf_ (N_("Files differ at the offset %llu: 0x%x [%s], 0x%x [%s]\n"),
				(unsigned long long) (i + pos), buf1[i],
				args[0], buf2[i], args[1]);
		  goto cleanup;
		}
	    }
	  pos += BUFFER_SIZE;

	}
      while (rd2);

      /* TRANSLATORS: it's always exactly 2 files.  */
      holy_printf_ (N_("The files are identical.\n"));
    }

cleanup:

  holy_free (buf1);
  holy_free (buf2);
  if (file1)
    holy_file_close (file1);
  if (file2)
    holy_file_close (file2);

  return holy_errno;
}

static holy_command_t cmd;

holy_MOD_INIT(cmp)
{
  cmd = holy_register_command ("cmp", holy_cmd_cmp,
			       N_("FILE1 FILE2"), N_("Compare two files."));
}

holy_MOD_FINI(cmp)
{
  holy_unregister_command (cmd);
}
