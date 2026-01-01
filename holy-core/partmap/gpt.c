/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/disk.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/partition.h>
#include <holy/dl.h>
#include <holy/msdos_partition.h>
#include <holy/gpt_partition.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_uint8_t holy_gpt_magic[8] =
  {
    0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54
  };

static const holy_gpt_part_type_t holy_gpt_partition_type_empty = holy_GPT_PARTITION_TYPE_EMPTY;

#ifdef holy_UTIL
static const holy_gpt_part_type_t holy_gpt_partition_type_bios_boot = holy_GPT_PARTITION_TYPE_BIOS_BOOT;
#endif

/* 512 << 7 = 65536 byte sectors.  */
#define MAX_SECTOR_LOG 7

static struct holy_partition_map holy_gpt_partition_map;



holy_err_t
holy_gpt_partition_map_iterate (holy_disk_t disk,
				holy_partition_iterate_hook_t hook,
				void *hook_data)
{
  struct holy_partition part;
  struct holy_gpt_header gpt;
  struct holy_gpt_partentry entry;
  struct holy_msdos_partition_mbr mbr;
  holy_uint64_t entries;
  unsigned int i;
  int last_offset = 0;
  int sector_log = 0;

  /* Read the protective MBR.  */
  if (holy_disk_read (disk, 0, 0, sizeof (mbr), &mbr))
    return holy_errno;

  /* Check if it is valid.  */
  if (mbr.signature != holy_cpu_to_le16_compile_time (holy_PC_PARTITION_SIGNATURE))
    return holy_error (holy_ERR_BAD_PART_TABLE, "no signature");

  /* Make sure the MBR is a protective MBR and not a normal MBR.  */
  for (i = 0; i < 4; i++)
    if (mbr.entries[i].type == holy_PC_PARTITION_TYPE_GPT_DISK)
      break;
  if (i == 4)
    return holy_error (holy_ERR_BAD_PART_TABLE, "no GPT partition map found");

  /* Read the GPT header.  */
  for (sector_log = 0; sector_log < MAX_SECTOR_LOG; sector_log++)
    {
      if (holy_disk_read (disk, 1 << sector_log, 0, sizeof (gpt), &gpt))
	return holy_errno;

      if (holy_memcmp (gpt.magic, holy_gpt_magic, sizeof (holy_gpt_magic)) == 0)
	break;
    }
  if (sector_log == MAX_SECTOR_LOG)
    return holy_error (holy_ERR_BAD_PART_TABLE, "no valid GPT header");

  holy_dprintf ("gpt", "Read a valid GPT header\n");

  entries = holy_le_to_cpu64 (gpt.partitions) << sector_log;
  for (i = 0; i < holy_le_to_cpu32 (gpt.maxpart); i++)
    {
      if (holy_disk_read (disk, entries, last_offset,
			  sizeof (entry), &entry))
	return holy_errno;

      if (holy_memcmp (&holy_gpt_partition_type_empty, &entry.type,
		       sizeof (holy_gpt_partition_type_empty)))
	{
	  /* Calculate the first block and the size of the partition.  */
	  part.start = holy_le_to_cpu64 (entry.start) << sector_log;
	  part.len = (holy_le_to_cpu64 (entry.end)
		      - holy_le_to_cpu64 (entry.start) + 1)  << sector_log;
	  part.offset = entries;
	  part.number = i;
	  part.index = last_offset;
	  part.partmap = &holy_gpt_partition_map;
	  part.parent = disk->partition;

	  holy_dprintf ("gpt", "GPT entry %d: start=%lld, length=%lld\n", i,
			(unsigned long long) part.start,
			(unsigned long long) part.len);

	  if (hook (disk, &part, hook_data))
	    return holy_errno;
	}

      last_offset += holy_le_to_cpu32 (gpt.partentry_size);
      if (last_offset == holy_DISK_SECTOR_SIZE)
	{
	  last_offset = 0;
	  entries++;
	}
    }

  return holy_ERR_NONE;
}

#ifdef holy_UTIL
/* Context for gpt_partition_map_embed.  */
struct gpt_partition_map_embed_ctx
{
  holy_disk_addr_t start, len;
};

/* Helper for gpt_partition_map_embed.  */
static int
find_usable_region (holy_disk_t disk __attribute__ ((unused)),
		    const holy_partition_t p, void *data)
{
  struct gpt_partition_map_embed_ctx *ctx = data;
  struct holy_gpt_partentry gptdata;
  holy_partition_t p2;

  p2 = disk->partition;
  disk->partition = p->parent;
  if (holy_disk_read (disk, p->offset, p->index,
		      sizeof (gptdata), &gptdata))
    {
      disk->partition = p2;
      return 0;
    }
  disk->partition = p2;

  /* If there's an embed region, it is in a dedicated partition.  */
  if (! holy_memcmp (&gptdata.type, &holy_gpt_partition_type_bios_boot, 16))
    {
      ctx->start = p->start;
      ctx->len = p->len;
      return 1;
    }

  return 0;
}

static holy_err_t
gpt_partition_map_embed (struct holy_disk *disk, unsigned int *nsectors,
			 unsigned int max_nsectors,
			 holy_embed_type_t embed_type,
			 holy_disk_addr_t **sectors)
{
  struct gpt_partition_map_embed_ctx ctx = {
    .start = 0,
    .len = 0
  };
  unsigned i;
  holy_err_t err;

  if (embed_type != holy_EMBED_PCBIOS)
    return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		       "GPT currently supports only PC-BIOS embedding");

  err = holy_gpt_partition_map_iterate (disk, find_usable_region, &ctx);
  if (err)
    return err;

  if (ctx.len == 0)
    return holy_error (holy_ERR_FILE_NOT_FOUND,
		       N_("this GPT partition label contains no BIOS Boot Partition;"
			  " embedding won't be possible"));

  if (ctx.len < *nsectors)
    return holy_error (holy_ERR_OUT_OF_RANGE,
		       N_("your BIOS Boot Partition is too small;"
			  " embedding won't be possible"));

  *nsectors = ctx.len;
  if (*nsectors > max_nsectors)
    *nsectors = max_nsectors;
  *sectors = holy_malloc (*nsectors * sizeof (**sectors));
  if (!*sectors)
    return holy_errno;
  for (i = 0; i < *nsectors; i++)
    (*sectors)[i] = ctx.start + i;

  return holy_ERR_NONE;
}
#endif


/* Partition map type.  */
static struct holy_partition_map holy_gpt_partition_map =
  {
    .name = "gpt",
    .iterate = holy_gpt_partition_map_iterate,
#ifdef holy_UTIL
    .embed = gpt_partition_map_embed
#endif
  };

holy_MOD_INIT(part_gpt)
{
  holy_partition_map_register (&holy_gpt_partition_map);
}

holy_MOD_FINI(part_gpt)
{
  holy_partition_map_unregister (&holy_gpt_partition_map);
}
