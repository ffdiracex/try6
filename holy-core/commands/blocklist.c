/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/file.h>
#include <holy/mm.h>
#include <holy/disk.h>
#include <holy/partition.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

/* Context for holy_cmd_blocklist.  */
struct blocklist_ctx
{
  unsigned long start_sector;
  unsigned num_sectors;
  int num_entries;
  holy_disk_addr_t part_start;
};

/* Helper for holy_cmd_blocklist.  */
static void
print_blocklist (holy_disk_addr_t sector, unsigned num,
		 unsigned offset, unsigned length, struct blocklist_ctx *ctx)
{
  if (ctx->num_entries++)
    holy_printf (",");

  holy_printf ("%llu", (unsigned long long) (sector - ctx->part_start));
  if (num > 0)
    holy_printf ("+%u", num);
  if (offset != 0 || length != 0)
    holy_printf ("[%u-%u]", offset, offset + length);
}

/* Helper for holy_cmd_blocklist.  */
static void
read_blocklist (holy_disk_addr_t sector, unsigned offset, unsigned length,
		void *data)
{
  struct blocklist_ctx *ctx = data;

  if (ctx->num_sectors > 0)
    {
      if (ctx->start_sector + ctx->num_sectors == sector
	  && offset == 0 && length >= holy_DISK_SECTOR_SIZE)
	{
	  ctx->num_sectors += length >> holy_DISK_SECTOR_BITS;
	  sector += length >> holy_DISK_SECTOR_BITS;
	  length &= (holy_DISK_SECTOR_SIZE - 1);
	}

      if (!length)
	return;
      print_blocklist (ctx->start_sector, ctx->num_sectors, 0, 0, ctx);
      ctx->num_sectors = 0;
    }

  if (offset)
    {
      unsigned l = length + offset;
      l &= (holy_DISK_SECTOR_SIZE - 1);
      l -= offset;
      print_blocklist (sector, 0, offset, l, ctx);
      length -= l;
      sector++;
      offset = 0;
    }

  if (!length)
    return;

  if (length & (holy_DISK_SECTOR_SIZE - 1))
    {
      if (length >> holy_DISK_SECTOR_BITS)
	{
	  print_blocklist (sector, length >> holy_DISK_SECTOR_BITS, 0, 0, ctx);
	  sector += length >> holy_DISK_SECTOR_BITS;
	}
      print_blocklist (sector, 0, 0, length & (holy_DISK_SECTOR_SIZE - 1), ctx);
    }
  else
    {
      ctx->start_sector = sector;
      ctx->num_sectors = length >> holy_DISK_SECTOR_BITS;
    }
}

static holy_err_t
holy_cmd_blocklist (holy_command_t cmd __attribute__ ((unused)),
		    int argc, char **args)
{
  holy_file_t file;
  char buf[holy_DISK_SECTOR_SIZE];
  struct blocklist_ctx ctx = {
    .start_sector = 0,
    .num_sectors = 0,
    .num_entries = 0,
    .part_start = 0
  };

  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  holy_file_filter_disable_compression ();
  file = holy_file_open (args[0]);
  if (! file)
    return holy_errno;

  if (! file->device->disk)
    return holy_error (holy_ERR_BAD_DEVICE,
		       "this command is available only for disk devices");

  ctx.part_start = holy_partition_get_start (file->device->disk->partition);

  file->read_hook = read_blocklist;
  file->read_hook_data = &ctx;

  while (holy_file_read (file, buf, sizeof (buf)) > 0)
    ;

  if (ctx.num_sectors > 0)
    print_blocklist (ctx.start_sector, ctx.num_sectors, 0, 0, &ctx);

  holy_file_close (file);

  return holy_errno;
}

static holy_command_t cmd;

holy_MOD_INIT(blocklist)
{
  cmd = holy_register_command ("blocklist", holy_cmd_blocklist,
			       N_("FILE"), N_("Print a block list."));
}

holy_MOD_FINI(blocklist)
{
  holy_unregister_command (cmd);
}
