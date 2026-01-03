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

#define holy_PARTMAP_SUN_MAGIC 0xDABE
#define holy_PARTMAP_SUN_MAX_PARTS 8
#define holy_PARTMAP_SUN_WHOLE_DISK_ID 0x05

struct holy_sun_partition_info
{
  holy_uint8_t spare1;
  holy_uint8_t id;
  holy_uint8_t spare2;
  holy_uint8_t flags;
} holy_PACKED;

struct holy_sun_partition_descriptor
{
  holy_uint32_t start_cylinder;
  holy_uint32_t num_sectors;
} holy_PACKED;

struct holy_sun_block
{
  holy_uint8_t  info[128];      /* Informative text string.  */
  holy_uint8_t  spare0[14];
  struct holy_sun_partition_info infos[8];
  holy_uint8_t  spare1[246];    /* Boot information etc.  */
  holy_uint16_t  rspeed;        /* Disk rotational speed.  */
  holy_uint16_t  pcylcount;     /* Physical cylinder count.  */
  holy_uint16_t  sparecyl;      /* extra sects per cylinder.  */
  holy_uint8_t  spare2[4];      /* More magic...  */
  holy_uint16_t  ilfact;        /* Interleave factor.  */
  holy_uint16_t  ncyl;          /* Data cylinder count.  */
  holy_uint16_t  nacyl;         /* Alt. cylinder count.  */
  holy_uint16_t  ntrks;         /* Tracks per cylinder.  */
  holy_uint16_t  nsect;         /* Sectors per track.  */
  holy_uint8_t  spare3[4];      /* Even more magic...  */
  struct holy_sun_partition_descriptor partitions[8];
  holy_uint16_t  magic;         /* Magic number.  */
  holy_uint16_t  csum;          /* Label xor'd checksum.  */
} holy_PACKED;

static struct holy_partition_map holy_sun_partition_map;

/* Verify checksum (true=ok).  */
static int
holy_sun_is_valid (holy_uint16_t *label)
{
  holy_uint16_t *pos;
  holy_uint16_t sum = 0;

  for (pos = label;
       pos < (label + sizeof (struct holy_sun_block) / 2);
       pos++)
    sum ^= *pos;

  return ! sum;
}

static holy_err_t
sun_partition_map_iterate (holy_disk_t disk,
			   holy_partition_iterate_hook_t hook, void *hook_data)
{
  struct holy_partition p;
  union
  {
    struct holy_sun_block sun_block;
    holy_uint16_t raw[0];
  } block;
  int partnum;
  holy_err_t err;

  p.partmap = &holy_sun_partition_map;
  err = holy_disk_read (disk, 0, 0, sizeof (struct holy_sun_block),
			&block);
  if (err)
    return err;

  if (holy_PARTMAP_SUN_MAGIC != holy_be_to_cpu16 (block.sun_block.magic))
    return holy_error (holy_ERR_BAD_PART_TABLE, "not a sun partition table");

  if (! holy_sun_is_valid (block.raw))
      return holy_error (holy_ERR_BAD_PART_TABLE, "invalid checksum");
  
  /* Maybe another error value would be better, because partition
     table _is_ recognized but invalid.  */
  for (partnum = 0; partnum < holy_PARTMAP_SUN_MAX_PARTS; partnum++)
    {
      struct holy_sun_partition_descriptor *desc;

      if (block.sun_block.infos[partnum].id == 0
	  || block.sun_block.infos[partnum].id == holy_PARTMAP_SUN_WHOLE_DISK_ID)
	continue;

      desc = &block.sun_block.partitions[partnum];
      p.start = ((holy_uint64_t) holy_be_to_cpu32 (desc->start_cylinder)
		  * holy_be_to_cpu16 (block.sun_block.ntrks)
		  * holy_be_to_cpu16 (block.sun_block.nsect));
      p.len = holy_be_to_cpu32 (desc->num_sectors);
      p.number = p.index = partnum;
      if (p.len)
	{
	  if (hook (disk, &p, hook_data))
	    partnum = holy_PARTMAP_SUN_MAX_PARTS;
	}
    }

  return holy_errno;
}

/* Partition map type.  */
static struct holy_partition_map holy_sun_partition_map =
  {
    .name = "sun",
    .iterate = sun_partition_map_iterate,
  };

holy_MOD_INIT(part_sun)
{
  holy_partition_map_register (&holy_sun_partition_map);
}

holy_MOD_FINI(part_sun)
{
  holy_partition_map_unregister (&holy_sun_partition_map);
}

