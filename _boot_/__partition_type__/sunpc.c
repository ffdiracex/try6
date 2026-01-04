/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/partition.h>
#include <holy/disk.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/dl.h>
#include <holy/symbol.h>
#include <holy/types.h>
#include <holy/err.h>

holy_MOD_LICENSE ("GPLv2+");

#define holy_PARTMAP_SUN_PC_MAGIC 0xDABE
#define holy_PARTMAP_SUN_PC_MAX_PARTS 16
#define holy_PARTMAP_SUN_PC_WHOLE_DISK_ID 0x05

struct holy_sun_pc_partition_descriptor
{
  holy_uint16_t id;
  holy_uint16_t unused;
  holy_uint32_t start_sector;
  holy_uint32_t num_sectors;
} holy_PACKED;

struct holy_sun_pc_block
{
  holy_uint8_t unused[72];
  struct holy_sun_pc_partition_descriptor partitions[holy_PARTMAP_SUN_PC_MAX_PARTS];
  holy_uint8_t unused2[244];
  holy_uint16_t  magic;         /* Magic number.  */
  holy_uint16_t  csum;          /* Label xor'd checksum.  */
} holy_PACKED;

static struct holy_partition_map holy_sun_pc_partition_map;

/* Verify checksum (true=ok).  */
static int
holy_sun_is_valid (holy_uint16_t *label)
{
  holy_uint16_t *pos;
  holy_uint16_t sum = 0;

  for (pos = label;
       pos < (label + sizeof (struct holy_sun_pc_block) / 2);
       pos++)
    sum ^= *pos;

  return ! sum;
}

static holy_err_t
sun_pc_partition_map_iterate (holy_disk_t disk,
			      holy_partition_iterate_hook_t hook,
			      void *hook_data)
{
  holy_partition_t p;
  union
  {
    struct holy_sun_pc_block sun_block;
    holy_uint16_t raw[0];
  } block;
  int partnum;
  holy_err_t err;

  p = (holy_partition_t) holy_zalloc (sizeof (struct holy_partition));
  if (! p)
    return holy_errno;

  p->partmap = &holy_sun_pc_partition_map;
  err = holy_disk_read (disk, 1, 0, sizeof (struct holy_sun_pc_block), &block);
  if (err)
    {
      holy_free (p);
      return err;
    }
  
  if (holy_PARTMAP_SUN_PC_MAGIC != holy_le_to_cpu16 (block.sun_block.magic))
    {
      holy_free (p);
      return holy_error (holy_ERR_BAD_PART_TABLE,
			 "not a sun_pc partition table");
    }

  if (! holy_sun_is_valid (block.raw))
    {
      holy_free (p);
      return holy_error (holy_ERR_BAD_PART_TABLE, "invalid checksum");
    }

  /* Maybe another error value would be better, because partition
     table _is_ recognized but invalid.  */
  for (partnum = 0; partnum < holy_PARTMAP_SUN_PC_MAX_PARTS; partnum++)
    {
      struct holy_sun_pc_partition_descriptor *desc;

      if (block.sun_block.partitions[partnum].id == 0
	  || block.sun_block.partitions[partnum].id
	  == holy_PARTMAP_SUN_PC_WHOLE_DISK_ID)
	continue;

      desc = &block.sun_block.partitions[partnum];
      p->start = holy_le_to_cpu32 (desc->start_sector);
      p->len = holy_le_to_cpu32 (desc->num_sectors);
      p->number = partnum;
      if (p->len)
	{
	  if (hook (disk, p, hook_data))
	    partnum = holy_PARTMAP_SUN_PC_MAX_PARTS;
	}
    }

  holy_free (p);

  return holy_errno;
}

/* Partition map type.  */
static struct holy_partition_map holy_sun_pc_partition_map =
  {
    .name = "sunpc",
    .iterate = sun_pc_partition_map_iterate,
  };

holy_MOD_INIT(part_sunpc)
{
  holy_partition_map_register (&holy_sun_pc_partition_map);
}

holy_MOD_FINI(part_sunpc)
{
  holy_partition_map_unregister (&holy_sun_pc_partition_map);
}

