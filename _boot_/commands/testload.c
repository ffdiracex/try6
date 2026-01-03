/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/env.h>
#include <holy/misc.h>
#include <holy/file.h>
#include <holy/disk.h>
#include <holy/term.h>
#include <holy/loader.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

/* Helper for holy_cmd_testload.  */
static void
read_progress (holy_disk_addr_t sector __attribute__ ((unused)),
	       unsigned offset __attribute__ ((unused)),
	       unsigned len,
	       void *data __attribute__ ((unused)))
{
  for (; len >= holy_DISK_SECTOR_SIZE; len -= holy_DISK_SECTOR_SIZE)
    holy_xputs (".");
  if (len)
    holy_xputs (".");
  holy_refresh ();
}

static holy_err_t
holy_cmd_testload (struct holy_command *cmd __attribute__ ((unused)),
		   int argc, char *argv[])
{
  holy_file_t file;
  char *buf;
  holy_size_t size;
  holy_off_t pos;

  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  file = holy_file_open (argv[0]);
  if (! file)
    return holy_errno;

  size = holy_file_size (file) & ~(holy_DISK_SECTOR_SIZE - 1);
  if (size == 0)
    {
      holy_file_close (file);
      return holy_ERR_NONE;
    }

  buf = holy_malloc (size);
  if (! buf)
    goto fail;

  holy_printf ("Reading %s sequentially", argv[0]);
  file->read_hook = read_progress;
  if (holy_file_read (file, buf, size) != (holy_ssize_t) size)
    goto fail;
  holy_printf (" Done.\n");

  /* Read sequentially again.  */
  holy_printf ("Reading %s sequentially again", argv[0]);
  holy_file_seek (file, 0);

  for (pos = 0; pos < size;)
    {
      char sector[holy_DISK_SECTOR_SIZE];
      holy_size_t curlen = holy_DISK_SECTOR_SIZE;

      if (curlen > size - pos)
	curlen = size - pos;

      if (holy_file_read (file, sector, curlen)
	  != (holy_ssize_t) curlen)
	goto fail;

      if (holy_memcmp (sector, buf + pos, curlen) != 0)
	{
	  holy_printf ("\nDiffers in %lld\n", (unsigned long long) pos);
	  goto fail;
	}
      pos += curlen;
    }
  holy_printf (" Done.\n");

  /* Read backwards and compare.  */
  holy_printf ("Reading %s backwards", argv[0]);
  pos = size;
  while (pos > 0)
    {
      char sector[holy_DISK_SECTOR_SIZE];

      if (pos >= holy_DISK_SECTOR_SIZE)
	pos -= holy_DISK_SECTOR_SIZE;
      else
	pos = 0;

      holy_file_seek (file, pos);

      if (holy_file_read (file, sector, holy_DISK_SECTOR_SIZE)
	  != holy_DISK_SECTOR_SIZE)
	goto fail;

      if (holy_memcmp (sector, buf + pos, holy_DISK_SECTOR_SIZE) != 0)
	{
	  int i;

	  holy_printf ("\nDiffers in %lld\n", (unsigned long long) pos);

	  for (i = 0; i < holy_DISK_SECTOR_SIZE; i++)
	    {
	      holy_printf ("%02x ", buf[pos + i]);
	      if ((i & 15) == 15)
		holy_printf ("\n");
	    }

	  if (i)
	    holy_refresh ();

	  goto fail;
	}
    }
  holy_printf (" Done.\n");

  return holy_ERR_NONE;

 fail:

  holy_file_close (file);
  holy_free (buf);

  if (!holy_errno)
    holy_error (holy_ERR_IO, "bad read");
  return holy_errno;
}

static holy_command_t cmd;

holy_MOD_INIT(testload)
{
  cmd =
    holy_register_command ("testload", holy_cmd_testload,
			   N_("FILE"),
			   N_("Load the same file in multiple ways."));
}

holy_MOD_FINI(testload)
{
  holy_unregister_command (cmd);
}
