/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/command.h>
#include <holy/fs.h>
#include <holy/misc.h>
#include <holy/dl.h>
#include <holy/device.h>
#include <holy/disk.h>
#include <holy/hfsplus.h>
#include <holy/hfs.h>
#include <holy/partition.h>
#include <holy/file.h>
#include <holy/mm.h>
#include <holy/err.h>

holy_MOD_LICENSE ("GPLv2+");

struct find_node_context
{
  holy_uint64_t inode_found;
  char *dirname;
  enum
  { FOUND_NONE, FOUND_FILE, FOUND_DIR } found;
};

static int
find_inode (const char *filename,
	    const struct holy_dirhook_info *info, void *data)
{
  struct find_node_context *ctx = data;
  if (!info->inodeset)
    return 0;

  if ((holy_strcmp (ctx->dirname, filename) == 0
       || (info->case_insensitive
	   && holy_strcasecmp (ctx->dirname, filename) == 0)))
    {
      ctx->inode_found = info->inode;
      ctx->found = info->dir ? FOUND_DIR : FOUND_FILE;
    }
  return 0;
}

holy_err_t
holy_mac_bless_inode (holy_device_t dev, holy_uint32_t inode, int is_dir,
		      int intel)
{
  holy_err_t err;
  union
  {
    struct holy_hfs_sblock hfs;
    struct holy_hfsplus_volheader hfsplus;
  } volheader;
  holy_disk_addr_t embedded_offset;

  if (intel && is_dir)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       "can't bless a directory for mactel");
  if (!intel && !is_dir)
    return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		       "can't bless a file for mac PPC");

  /* Read the bootblock.  */
  err = holy_disk_read (dev->disk, holy_HFSPLUS_SBLOCK, 0, sizeof (volheader),
			(char *) &volheader);
  if (err)
    return err;

  embedded_offset = 0;
  if (holy_be_to_cpu16 (volheader.hfs.magic) == holy_HFS_MAGIC)
    {
      int extent_start;
      int ablk_size;
      int ablk_start;

      /* See if there's an embedded HFS+ filesystem.  */
      if (holy_be_to_cpu16 (volheader.hfs.embed_sig) != holy_HFSPLUS_MAGIC)
	{
	  if (intel)
	    volheader.hfs.intel_bootfile = holy_be_to_cpu32 (inode);
	  else
	    volheader.hfs.ppc_bootdir = holy_be_to_cpu32 (inode);
	  return holy_ERR_NONE;
	}

      /* Calculate the offset needed to translate HFS+ sector numbers.  */
      extent_start =
	holy_be_to_cpu16 (volheader.hfs.embed_extent.first_block);
      ablk_size = holy_be_to_cpu32 (volheader.hfs.blksz);
      ablk_start = holy_be_to_cpu16 (volheader.hfs.first_block);
      embedded_offset = (ablk_start
			 + ((holy_uint64_t) extent_start)
			 * (ablk_size >> holy_DISK_SECTOR_BITS));

      err =
	holy_disk_read (dev->disk, embedded_offset + holy_HFSPLUS_SBLOCK, 0,
			sizeof (volheader), (char *) &volheader);
      if (err)
	return err;
    }

  if ((holy_be_to_cpu16 (volheader.hfsplus.magic) != holy_HFSPLUS_MAGIC)
      && (holy_be_to_cpu16 (volheader.hfsplus.magic) != holy_HFSPLUSX_MAGIC))
    return holy_error (holy_ERR_BAD_FS, "not a HFS+ filesystem");
  if (intel)
    volheader.hfsplus.intel_bootfile = holy_be_to_cpu32 (inode);
  else
    volheader.hfsplus.ppc_bootdir = holy_be_to_cpu32 (inode);

  return holy_disk_write (dev->disk, embedded_offset + holy_HFSPLUS_SBLOCK, 0,
			  sizeof (volheader), (char *) &volheader);
}

holy_err_t
holy_mac_bless_file (holy_device_t dev, const char *path_in, int intel)
{
  holy_fs_t fs;

  char *path, *tail;
  struct find_node_context ctx;

  fs = holy_fs_probe (dev);
  if (!fs || (holy_strcmp (fs->name, "hfsplus") != 0
	      && holy_strcmp (fs->name, "hfs") != 0))
    return holy_error (holy_ERR_BAD_FS, "no suitable FS found");

  path = holy_strdup (path_in);
  if (!path)
    return holy_errno;

  tail = path + holy_strlen (path) - 1;

  /* Remove trailing '/'. */
  while (tail != path && *tail == '/')
    *(tail--) = 0;

  tail = holy_strrchr (path, '/');
  ctx.found = 0;

  if (tail)
    {
      *tail = 0;
      ctx.dirname = tail + 1;

      (fs->dir) (dev, *path == 0 ? "/" : path, find_inode, &ctx);
    }
  else
    {
      ctx.dirname = path + 1;
      (fs->dir) (dev, "/", find_inode, &ctx);
    }
  if (!ctx.found)
    {
      holy_free (path);
      return holy_error (holy_ERR_FILE_NOT_FOUND, N_("file `%s' not found"),
			 path_in);
    }
  holy_free (path);

  return holy_mac_bless_inode (dev, (holy_uint32_t) ctx.inode_found,
			       (ctx.found == FOUND_DIR), intel);
}

static holy_err_t
holy_cmd_macbless (holy_command_t cmd, int argc, char **args)
{
  char *device_name;
  char *path = 0;
  holy_device_t dev = 0;
  holy_err_t err;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));
  device_name = holy_file_get_device_name (args[0]);
  dev = holy_device_open (device_name);

  path = holy_strchr (args[0], ')');
  if (!path)
    path = args[0];
  else
    path = path + 1;

  if (!path || *path == 0 || !dev)
    {
      if (dev)
	holy_device_close (dev);

      holy_free (device_name);

      return holy_error (holy_ERR_BAD_ARGUMENT, "invalid argument");
    }

  err = holy_mac_bless_file (dev, path, cmd->name[3] == 't');

  holy_device_close (dev);
  holy_free (device_name);
  return err;
}

static holy_command_t cmd, cmd_ppc;

holy_MOD_INIT(macbless)
{
  cmd = holy_register_command ("mactelbless", holy_cmd_macbless,
			       N_("FILE"),
			       N_
			       ("Bless FILE of HFS or HFS+ partition for intel macs."));
  cmd_ppc =
    holy_register_command ("macppcbless", holy_cmd_macbless, N_("DIR"),
			   N_
			   ("Bless DIR of HFS or HFS+ partition for PPC macs."));
}

holy_MOD_FINI(macbless)
{
  holy_unregister_command (cmd);
  holy_unregister_command (cmd_ppc);
}
