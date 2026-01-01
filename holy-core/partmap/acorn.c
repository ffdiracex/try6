/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/disk.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/partition.h>
#include <holy/acorn_filecore.h>

holy_MOD_LICENSE ("GPLv2+");

#define LINUX_NATIVE_MAGIC holy_cpu_to_le32_compile_time (0xdeafa1de)
#define LINUX_SWAP_MAGIC   holy_cpu_to_le32_compile_time (0xdeafab1e)
#define LINUX_MAP_ENTRIES  (512 / 12)

#define NONADFS_PARTITION_TYPE_LINUX 9
#define NONADFS_PARTITION_TYPE_MASK 15

struct holy_acorn_boot_block
{
  union
  {
    struct
    {
      holy_uint8_t misc[0x1C0];
      struct holy_filecore_disc_record disc_record;
      holy_uint8_t flags;
      holy_uint16_t start_cylinder;
      holy_uint8_t checksum;
    } holy_PACKED;
    holy_uint8_t bin[0x200];
  };
} holy_PACKED;

struct linux_part
{
  holy_uint32_t magic;
  holy_uint32_t start;
  holy_uint32_t size;
};

static struct holy_partition_map holy_acorn_partition_map;

static holy_err_t
acorn_partition_map_find (holy_disk_t disk, struct linux_part *m,
			  holy_disk_addr_t *sector)
{
  struct holy_acorn_boot_block boot;
  holy_err_t err;
  unsigned int checksum = 0;
  unsigned int heads;
  unsigned int sectors_per_cylinder;
  int i;

  err = holy_disk_read (disk, 0xC00 / holy_DISK_SECTOR_SIZE, 0,
			sizeof (struct holy_acorn_boot_block),
			&boot);
  if (err)
    return err;

  if ((boot.flags & NONADFS_PARTITION_TYPE_MASK) != NONADFS_PARTITION_TYPE_LINUX)
    goto fail;

  for (i = 0; i != 0x1ff; ++i)
    checksum = ((checksum & 0xff) + (checksum >> 8) + boot.bin[i]);

  if ((holy_uint8_t) checksum != boot.checksum)
    goto fail;

  heads = (boot.disc_record.heads
		    + ((boot.disc_record.lowsector >> 6) & 1));
  sectors_per_cylinder = boot.disc_record.secspertrack * heads;
  *sector = holy_le_to_cpu16 (boot.start_cylinder) * sectors_per_cylinder;

  return holy_disk_read (disk, *sector, 0,
			 sizeof (struct linux_part) * LINUX_MAP_ENTRIES,
			 m);

fail:
  return holy_error (holy_ERR_BAD_PART_TABLE,
		     "Linux/ADFS partition map not found");

}


static holy_err_t
acorn_partition_map_iterate (holy_disk_t disk,
			     holy_partition_iterate_hook_t hook,
			     void *hook_data)
{
  struct holy_partition part;
  struct linux_part map[LINUX_MAP_ENTRIES];
  int i;
  holy_disk_addr_t sector = 0;
  holy_err_t err;

  err = acorn_partition_map_find (disk, map, &sector);
  if (err)
    return err;

  part.partmap = &holy_acorn_partition_map;

  for (i = 0; i != LINUX_MAP_ENTRIES; ++i)
    {
      if (map[i].magic != LINUX_NATIVE_MAGIC
	  && map[i].magic != LINUX_SWAP_MAGIC)
	return holy_ERR_NONE;

      part.start = sector + map[i].start;
      part.len = map[i].size;
      part.offset = 6;
      part.number = part.index = i;

      if (hook (disk, &part, hook_data))
	return holy_errno;
    }

  return holy_ERR_NONE;
}



/* Partition map type.  */
static struct holy_partition_map holy_acorn_partition_map =
{
  .name = "acorn",
  .iterate = acorn_partition_map_iterate,
};

holy_MOD_INIT(part_acorn)
{
  holy_partition_map_register (&holy_acorn_partition_map);
}

holy_MOD_FINI(part_acorn)
{
  holy_partition_map_unregister (&holy_acorn_partition_map);
}
