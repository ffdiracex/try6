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

#define DVH_MAGIC 0x0be5a941

struct holy_dvh_partition_descriptor
{
  holy_uint32_t length;
  holy_uint32_t start;
  holy_uint32_t type;
} holy_PACKED;

struct holy_dvh_block
{
  holy_uint32_t magic;
  holy_uint8_t unused[308];
  struct holy_dvh_partition_descriptor parts[16];
  holy_uint32_t checksum;
  holy_uint32_t unused2;
} holy_PACKED;

static struct holy_partition_map holy_dvh_partition_map;

/* Verify checksum (true=ok).  */
static int
holy_dvh_is_valid (holy_uint32_t *label)
{
  holy_uint32_t *pos;
  holy_uint32_t sum = 0;

  for (pos = label;
       pos < (label + sizeof (struct holy_dvh_block) / 4);
       pos++)
    sum += holy_be_to_cpu32 (*pos);

  return ! sum;
}

static holy_err_t
dvh_partition_map_iterate (holy_disk_t disk,
			   holy_partition_iterate_hook_t hook, void *hook_data)
{
  struct holy_partition p;
  union
  {
    struct holy_dvh_block dvh;
    holy_uint32_t raw[0];
  } block;
  unsigned partnum;
  holy_err_t err;

  p.partmap = &holy_dvh_partition_map;
  err = holy_disk_read (disk, 0, 0, sizeof (struct holy_dvh_block),
			&block);
  if (err)
    return err;

  if (DVH_MAGIC != holy_be_to_cpu32 (block.dvh.magic))
    return holy_error (holy_ERR_BAD_PART_TABLE, "not a dvh partition table");

  if (! holy_dvh_is_valid (block.raw))
      return holy_error (holy_ERR_BAD_PART_TABLE, "invalid checksum");
  
  /* Maybe another error value would be better, because partition
     table _is_ recognized but invalid.  */
  for (partnum = 0; partnum < ARRAY_SIZE (block.dvh.parts); partnum++)
    {
      if (block.dvh.parts[partnum].length == 0)
	continue;

      if (partnum == 10)
	continue;

      p.start = holy_be_to_cpu32 (block.dvh.parts[partnum].start);
      p.len = holy_be_to_cpu32 (block.dvh.parts[partnum].length);
      p.number = p.index = partnum;
      if (hook (disk, &p, hook_data))
	break;
    }

  return holy_errno;
}


/* Partition map type.  */
static struct holy_partition_map holy_dvh_partition_map =
  {
    .name = "dvh",
    .iterate = dvh_partition_map_iterate,
  };

holy_MOD_INIT(part_dvh)
{
  holy_partition_map_register (&holy_dvh_partition_map);
}

holy_MOD_FINI(part_dvh)
{
  holy_partition_map_unregister (&holy_dvh_partition_map);
}

