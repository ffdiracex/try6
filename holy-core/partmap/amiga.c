/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/disk.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/partition.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

#define AMIGA_CHECKSUM_WORDS 128

struct holy_amiga_rdsk
{
  /* "RDSK".  */
  holy_uint8_t magic[4];
#define holy_AMIGA_RDSK_MAGIC "RDSK"
  holy_uint32_t size;
  holy_int32_t checksum;
  holy_uint32_t scsihost;
  holy_uint32_t blksz;
  holy_uint32_t flags;
  holy_uint32_t badblcklst;
  holy_uint32_t partitionlst;
  holy_uint32_t fslst;

  holy_uint32_t unused[AMIGA_CHECKSUM_WORDS - 9];
} holy_PACKED;

struct holy_amiga_partition
{
  /* "PART".  */
  holy_uint8_t magic[4];
#define holy_AMIGA_PART_MAGIC "PART"
  holy_uint32_t size;
  holy_int32_t checksum;
  holy_uint32_t scsihost;
  holy_uint32_t next;
  holy_uint32_t flags;
  holy_uint32_t unused1[2];
  holy_uint32_t devflags;
  holy_uint8_t namelen;
  holy_uint8_t name[31];
  holy_uint32_t unused2[15];

  holy_uint32_t unused3[3];
  holy_uint32_t heads;
  holy_uint32_t unused4;
  holy_uint32_t block_per_track;
  holy_uint32_t unused5[3];
  holy_uint32_t lowcyl;
  holy_uint32_t highcyl;

  holy_uint32_t firstcyl;
  holy_uint32_t unused[AMIGA_CHECKSUM_WORDS - 44];
} holy_PACKED;

static struct holy_partition_map holy_amiga_partition_map;



static holy_uint32_t
amiga_partition_map_checksum (void *buf)
{
  holy_uint32_t *ptr = buf;
  holy_uint32_t r = 0;
  holy_size_t sz;
  /* Fancy and quick way of checking sz >= 512 / 4 = 128.  */
  if (ptr[1] & ~holy_cpu_to_be32_compile_time (AMIGA_CHECKSUM_WORDS - 1))
    sz = AMIGA_CHECKSUM_WORDS;
  else
    sz = holy_be_to_cpu32 (ptr[1]);

  for (; sz; sz--, ptr++)
    r += holy_be_to_cpu32 (*ptr);

  return r;
}

static holy_err_t
amiga_partition_map_iterate (holy_disk_t disk,
			     holy_partition_iterate_hook_t hook,
			     void *hook_data)
{
  struct holy_partition part;
  struct holy_amiga_rdsk rdsk;
  int partno = 0;
  int next = -1;
  unsigned pos;

  /* The RDSK block is one of the first 15 blocks.  */
  for (pos = 0; pos < 15; pos++)
    {
      /* Read the RDSK block which is a descriptor for the entire disk.  */
      if (holy_disk_read (disk, pos, 0, sizeof (rdsk), &rdsk))
	return holy_errno;

      if (holy_memcmp (rdsk.magic, holy_AMIGA_RDSK_MAGIC,
		       sizeof (rdsk.magic)) == 0
	  && amiga_partition_map_checksum (&rdsk) == 0)
	{
	  /* Found the first PART block.  */
	  next = holy_be_to_cpu32 (rdsk.partitionlst);
	  break;
	}
    }

  if (next == -1)
    return holy_error (holy_ERR_BAD_PART_TABLE,
		       "Amiga partition map not found");

  /* The end of the partition list is marked using "-1".  */
  while (next != -1)
    {
      struct holy_amiga_partition apart;

      /* Read the RDSK block which is a descriptor for the entire disk.  */
      if (holy_disk_read (disk, next, 0, sizeof (apart), &apart))
	return holy_errno;

      if (holy_memcmp (apart.magic, holy_AMIGA_PART_MAGIC,
		       sizeof (apart.magic)) != 0
	  || amiga_partition_map_checksum (&apart) != 0)
	return holy_error (holy_ERR_BAD_PART_TABLE,
			   "invalid Amiga partition map");
      /* Calculate the first block and the size of the partition.  */
      part.start = (holy_be_to_cpu32 (apart.lowcyl)
		    * holy_be_to_cpu32 (apart.heads)
		    * holy_be_to_cpu32 (apart.block_per_track));
      part.len = ((holy_be_to_cpu32 (apart.highcyl)
		   - holy_be_to_cpu32 (apart.lowcyl) + 1)
		  * holy_be_to_cpu32 (apart.heads)
		  * holy_be_to_cpu32 (apart.block_per_track));

      part.offset = next;
      part.number = partno;
      part.index = 0;
      part.partmap = &holy_amiga_partition_map;

      if (hook (disk, &part, hook_data))
	return holy_errno;

      next = holy_be_to_cpu32 (apart.next);
      partno++;
    }

  return 0;
}


/* Partition map type.  */
static struct holy_partition_map holy_amiga_partition_map =
  {
    .name = "amiga",
    .iterate = amiga_partition_map_iterate,
  };

holy_MOD_INIT(part_amiga)
{
  holy_partition_map_register (&holy_amiga_partition_map);
}

holy_MOD_FINI(part_amiga)
{
  holy_partition_map_unregister (&holy_amiga_partition_map);
}
