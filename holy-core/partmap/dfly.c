/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/disk.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/partition.h>

holy_MOD_LICENSE ("GPLv2+");

static struct holy_partition_map holy_dfly_partition_map;

#define holy_PARTITION_DISKLABEL64_HEADER_SIZE 200

/* Full entry is 64 bytes however we really care only
   about offset and size which are in first 16 bytes.
   Avoid using too much stack.  */
#define holy_PARTITION_DISKLABEL64_ENTRY_SIZE 64
struct holy_partition_disklabel64_entry
{
  holy_uint64_t boffset;
  holy_uint64_t bsize;
};

/* Full entry is 200 bytes however we really care only
   about magic and number of partitions which are in first 16 bytes.
   Avoid using too much stack.  */
struct holy_partition_disklabel64
{
  holy_uint32_t   magic;
#define holy_DISKLABEL64_MAGIC		  ((holy_uint32_t)0xc4464c59)
  holy_uint32_t   crc;
  holy_uint32_t   unused;
  holy_uint32_t   npartitions;
};

static holy_err_t
dfly_partition_map_iterate (holy_disk_t disk,
			    holy_partition_iterate_hook_t hook,
			    void *hook_data)
{
  struct holy_partition part;
  unsigned partno, pos;
  struct holy_partition_disklabel64 label;

  part.partmap = &holy_dfly_partition_map;

  if (holy_disk_read (disk, 1, 0, sizeof (label), &label))
    return holy_errno;

  if (label.magic != holy_cpu_to_le32_compile_time (holy_DISKLABEL64_MAGIC))
    {
      holy_dprintf ("partition",
		    "bad magic (found 0x%x; wanted 0x%x)\n",
		    (unsigned int) holy_le_to_cpu32 (label.magic),
		    (unsigned int) holy_DISKLABEL64_MAGIC);
      return holy_error (holy_ERR_BAD_PART_TABLE, "disklabel64 not found");
    }

  pos = holy_PARTITION_DISKLABEL64_HEADER_SIZE + holy_DISK_SECTOR_SIZE;

  for (partno = 0;
       partno < holy_le_to_cpu32 (label.npartitions); ++partno)
    {
      holy_disk_addr_t sector = pos >> holy_DISK_SECTOR_BITS;
      holy_off_t       offset = pos & (holy_DISK_SECTOR_SIZE - 1);
      struct holy_partition_disklabel64_entry dpart;

      pos += holy_PARTITION_DISKLABEL64_ENTRY_SIZE;
      if (holy_disk_read (disk, sector, offset, sizeof (dpart), &dpart))
	return holy_errno;

      holy_dprintf ("partition",
		    "partition %2d: offset 0x%llx, "
		    "size 0x%llx\n",
		    partno,
		    (unsigned long long) holy_le_to_cpu64 (dpart.boffset),
		    (unsigned long long) holy_le_to_cpu64 (dpart.bsize));

      /* Is partition initialized? */
      if (dpart.bsize == 0)
	continue;

      part.number = partno;
      part.start = holy_le_to_cpu64 (dpart.boffset) >> holy_DISK_SECTOR_BITS;
      part.len = holy_le_to_cpu64 (dpart.bsize) >> holy_DISK_SECTOR_BITS;

      /* This is counter-intuitive, but part.offset and sector have
       * the same type, and offset (NOT part.offset) is guaranteed
       * to fit into part.index. */
      part.offset = sector;
      part.index = offset;

      if (hook (disk, &part, hook_data))
	return holy_errno;
    }

  return holy_ERR_NONE;
}

/* Partition map type.  */
static struct holy_partition_map holy_dfly_partition_map =
{
    .name = "dfly",
    .iterate = dfly_partition_map_iterate,
};

holy_MOD_INIT(part_dfly)
{
  holy_partition_map_register (&holy_dfly_partition_map);
}

holy_MOD_FINI(part_dfly)
{
  holy_partition_map_unregister (&holy_dfly_partition_map);
}
