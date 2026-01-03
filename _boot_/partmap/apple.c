/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/disk.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/partition.h>

holy_MOD_LICENSE ("GPLv2+");

#define holy_APPLE_HEADER_MAGIC	0x4552
#define holy_APPLE_PART_MAGIC	0x504D

struct holy_apple_header
{
  /* The magic number to identify the partition map, it should have
     the value `0x4552'.  */
  holy_uint16_t magic;
  holy_uint16_t blocksize;
};

struct holy_apple_part
{
  /* The magic number to identify this as a partition, it should have
     the value `0x504D'.  */
  holy_uint16_t magic;

  /* Reserved.  */
  holy_uint16_t reserved;

  /* The size of the partition map in blocks.  */
  holy_uint32_t partmap_size;

  /* The first physical block of the partition.  */
  holy_uint32_t first_phys_block;

  /* The amount of blocks.  */
  holy_uint32_t blockcnt;

  /* The partition name.  */
  char partname[32];

  /* The partition type.  */
  char parttype[32];

  /* The first datablock of the partition.  */
  holy_uint32_t datablocks_first;

  /* The amount datablocks.  */
  holy_uint32_t datablocks_count;

  /* The status of the partition. (???)  */
  holy_uint32_t status;

  /* The first block on which the bootcode can be found.  */
  holy_uint32_t bootcode_pos;

  /* The size of the bootcode in bytes.  */
  holy_uint32_t bootcode_size;

  /* The load address of the bootcode.  */
  holy_uint32_t bootcode_loadaddr;

  /* Reserved.  */
  holy_uint32_t reserved2;

  /* The entry point of the bootcode.  */
  holy_uint32_t bootcode_entrypoint;

  /* Reserved.  */
  holy_uint32_t reserved3;

  /* A checksum of the bootcode.  */
  holy_uint32_t bootcode_checksum;

  /* The processor type.  */
  char processor[16];

  /* Padding.  */
  holy_uint16_t pad[187];
};

static struct holy_partition_map holy_apple_partition_map;


static holy_err_t
apple_partition_map_iterate (holy_disk_t disk,
			     holy_partition_iterate_hook_t hook,
			     void *hook_data)
{
  struct holy_partition part;
  struct holy_apple_header aheader;
  struct holy_apple_part apart;
  int partno = 0, partnum = 0;
  unsigned pos;

  part.partmap = &holy_apple_partition_map;

  if (holy_disk_read (disk, 0, 0, sizeof (aheader), &aheader))
    return holy_errno;

  if (holy_be_to_cpu16 (aheader.magic) != holy_APPLE_HEADER_MAGIC)
    {
      holy_dprintf ("partition",
		    "bad magic (found 0x%x; wanted 0x%x)\n",
		    holy_be_to_cpu16 (aheader.magic),
		    holy_APPLE_HEADER_MAGIC);
      goto fail;
    }

  pos = holy_be_to_cpu16 (aheader.blocksize);

  do
    {
      part.offset = pos / holy_DISK_SECTOR_SIZE;
      part.index = pos % holy_DISK_SECTOR_SIZE;

      if (holy_disk_read (disk, part.offset, part.index,
			  sizeof (struct holy_apple_part),  &apart))
	return holy_errno;

      if (holy_be_to_cpu16 (apart.magic) != holy_APPLE_PART_MAGIC)
	{
	  holy_dprintf ("partition",
			"partition %d: bad magic (found 0x%x; wanted 0x%x)\n",
			partno, holy_be_to_cpu16 (apart.magic),
			holy_APPLE_PART_MAGIC);
	  break;
	}

      if (partnum == 0)
	partnum = holy_be_to_cpu32 (apart.partmap_size);

      part.start = ((holy_disk_addr_t) holy_be_to_cpu32 (apart.first_phys_block)
		    * holy_be_to_cpu16 (aheader.blocksize))
	/ holy_DISK_SECTOR_SIZE;
      part.len = ((holy_disk_addr_t) holy_be_to_cpu32 (apart.blockcnt)
		  * holy_be_to_cpu16 (aheader.blocksize))
	/ holy_DISK_SECTOR_SIZE;
      part.offset = pos;
      part.index = partno;
      part.number = partno;

      holy_dprintf ("partition",
		    "partition %d: name %s, type %s, start 0x%x, len 0x%x\n",
		    partno, apart.partname, apart.parttype,
		    holy_be_to_cpu32 (apart.first_phys_block),
		    holy_be_to_cpu32 (apart.blockcnt));

      if (hook (disk, &part, hook_data))
	return holy_errno;

      pos += holy_be_to_cpu16 (aheader.blocksize);
      partno++;
    }
  while (partno < partnum);

  if (partno != 0)
    return 0;

 fail:
  return holy_error (holy_ERR_BAD_PART_TABLE,
		     "Apple partition map not found");
}


/* Partition map type.  */
static struct holy_partition_map holy_apple_partition_map =
  {
    .name = "apple",
    .iterate = apple_partition_map_iterate,
  };

holy_MOD_INIT(part_apple)
{
  holy_partition_map_register (&holy_apple_partition_map);
}

holy_MOD_FINI(part_apple)
{
  holy_partition_map_unregister (&holy_apple_partition_map);
}

