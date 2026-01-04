/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/file.h>
#include <holy/disk.h>
#include <holy/misc.h>
#include <holy/lib/hexdump.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options[] = {
  {"skip", 's', 0, N_("Skip offset bytes from the beginning of file."), 0,
   ARG_TYPE_INT},
  {"length", 'n', 0, N_("Read only LENGTH bytes."), 0, ARG_TYPE_INT},
  {0, 0, 0, 0, 0, 0}
};

static holy_err_t
holy_cmd_hexdump (holy_extcmd_context_t ctxt, int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;
  char buf[holy_DISK_SECTOR_SIZE * 4];
  holy_ssize_t size, length;
  holy_disk_addr_t skip;
  int namelen;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  namelen = holy_strlen (args[0]);
  skip = (state[0].set) ? holy_strtoull (state[0].arg, 0, 0) : 0;
  length = (state[1].set) ? holy_strtoul (state[1].arg, 0, 0) : 256;

  if (!holy_strcmp (args[0], "(mem)"))
    hexdump (skip, (char *) (holy_addr_t) skip, length);
  else if ((args[0][0] == '(') && (args[0][namelen - 1] == ')'))
    {
      holy_disk_t disk;
      holy_disk_addr_t sector;
      holy_size_t ofs;

      args[0][namelen - 1] = 0;
      disk = holy_disk_open (&args[0][1]);
      if (! disk)
        return 0;

      sector = (skip >> (holy_DISK_SECTOR_BITS + 2)) * 4;
      ofs = skip & (holy_DISK_SECTOR_SIZE * 4 - 1);
      while (length)
        {
          holy_size_t len;

          len = length;
          if (len > sizeof (buf))
            len = sizeof (buf);

          if (holy_disk_read (disk, sector, ofs, len, buf))
            break;

          hexdump (skip, buf, len);

          ofs = 0;
          skip += len;
          length -= len;
          sector += 4;
        }

      holy_disk_close (disk);
    }
  else
    {
      holy_file_t file;

      file = holy_file_open (args[0]);
      if (! file)
	return 0;

      file->offset = skip;

      while ((size = holy_file_read (file, buf, sizeof (buf))) > 0)
	{
	  unsigned long len;

	  len = ((length) && (size > length)) ? length : size;
	  hexdump (skip, buf, len);
	  skip += len;
	  if (length)
	    {
	      length -= len;
	      if (!length)
		break;
	    }
	}

      holy_file_close (file);
    }

  return 0;
}

static holy_extcmd_t cmd;

holy_MOD_INIT (hexdump)
{
  cmd = holy_register_extcmd ("hexdump", holy_cmd_hexdump, 0,
			      N_("[OPTIONS] FILE_OR_DEVICE"),
			      N_("Show raw contents of a file or memory."),
			      options);
}

holy_MOD_FINI (hexdump)
{
  holy_unregister_extcmd (cmd);
}
