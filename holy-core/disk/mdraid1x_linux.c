/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/disk.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/misc.h>
#include <holy/diskfilter.h>

holy_MOD_LICENSE ("GPLv2+");

/* Linux RAID on disk structures and constants,
   copied from include/linux/raid/md_p.h.  */

#define SB_MAGIC			0xa92b4efc

/*
 * The version-1 superblock :
 * All numeric fields are little-endian.
 *
 * Total size: 256 bytes plus 2 per device.
 * 1K allows 384 devices.
 */

struct holy_raid_super_1x
{
  /* Constant array information - 128 bytes.  */
  holy_uint32_t magic;		/* MD_SB_MAGIC: 0xa92b4efc - little endian.  */
  holy_uint32_t major_version;	/* 1.  */
  holy_uint32_t feature_map;	/* Bit 0 set if 'bitmap_offset' is meaningful.   */
  holy_uint32_t pad0;		/* Always set to 0 when writing.  */

  holy_uint8_t set_uuid[16];	/* User-space generated.  */
  char set_name[32];		/* Set and interpreted by user-space.  */

  holy_uint64_t ctime;		/* Lo 40 bits are seconds, top 24 are microseconds or 0.  */
  holy_uint32_t level;		/* -4 (multipath), -1 (linear), 0,1,4,5.  */
  holy_uint32_t layout;		/* only for raid5 and raid10 currently.  */
  holy_uint64_t size;		/* Used size of component devices, in 512byte sectors.  */

  holy_uint32_t chunksize;	/* In 512byte sectors.  */
  holy_uint32_t raid_disks;
  holy_uint32_t bitmap_offset;	/* Sectors after start of superblock that bitmap starts
				 * NOTE: signed, so bitmap can be before superblock
				 * only meaningful of feature_map[0] is set.
				 */

  /* These are only valid with feature bit '4'.  */
  holy_uint32_t new_level;	/* New level we are reshaping to.  */
  holy_uint64_t reshape_position;	/* Next address in array-space for reshape.  */
  holy_uint32_t delta_disks;	/* Change in number of raid_disks.  */
  holy_uint32_t new_layout;	/* New layout.  */
  holy_uint32_t new_chunk;	/* New chunk size (512byte sectors).  */
  holy_uint8_t pad1[128 - 124];	/* Set to 0 when written.  */

  /* Constant this-device information - 64 bytes.  */
  holy_uint64_t data_offset;	/* Sector start of data, often 0.  */
  holy_uint64_t data_size;	/* Sectors in this device that can be used for data.  */
  holy_uint64_t super_offset;	/* Sector start of this superblock.  */
  holy_uint64_t recovery_offset;	/* Sectors before this offset (from data_offset) have been recovered.  */
  holy_uint32_t dev_number;	/* Permanent identifier of this  device - not role in raid.  */
  holy_uint32_t cnt_corrected_read;	/* Number of read errors that were corrected by re-writing.  */
  holy_uint8_t device_uuid[16];	/* User-space setable, ignored by kernel.  */
  holy_uint8_t devflags;	/* Per-device flags.  Only one defined...  */
  holy_uint8_t pad2[64 - 57];	/* Set to 0 when writing.  */

  /* Array state information - 64 bytes.  */
  holy_uint64_t utime;		/* 40 bits second, 24 btes microseconds.  */
  holy_uint64_t events;		/* Incremented when superblock updated.  */
  holy_uint64_t resync_offset;	/* Data before this offset (from data_offset) known to be in sync.  */
  holy_uint32_t sb_csum;	/* Checksum upto devs[max_dev].  */
  holy_uint32_t max_dev;	/* Size of devs[] array to consider.  */
  holy_uint8_t pad3[64 - 32];	/* Set to 0 when writing.  */

  /* Device state information. Indexed by dev_number.
   * 2 bytes per device.
   * Note there are no per-device state flags. State information is rolled
   * into the 'roles' value.  If a device is spare or faulty, then it doesn't
   * have a meaningful role.
   */
  holy_uint16_t dev_roles[0];	/* Role in array, or 0xffff for a spare, or 0xfffe for faulty.  */
};
/* Could be holy_PACKED, but since all members in this struct
   are already appropriately aligned, we can omit this and avoid suboptimal
   assembly in some cases.  */

#define WriteMostly1    1	/* Mask for writemostly flag in above devflags.  */

static struct holy_diskfilter_vg *
holy_mdraid_detect (holy_disk_t disk,
		    struct holy_diskfilter_pv_id *id,
		    holy_disk_addr_t *start_sector)
{
  holy_uint64_t size;
  holy_uint8_t minor_version;

  size = holy_disk_get_size (disk);

  /* Check for an 1.x superblock.
   * It's always aligned to a 4K boundary
   * and depending on the minor version it can be:
   * 0: At least 8K, but less than 12K, from end of device
   * 1: At start of device
   * 2: 4K from start of device.
   */

  for (minor_version = 0; minor_version < 3; ++minor_version)
    {
      holy_disk_addr_t sector = 0;
      struct holy_raid_super_1x sb;
      holy_uint16_t role;
      holy_uint32_t level;
      struct holy_diskfilter_vg *array;
      char *uuid;
	
      if (size == holy_DISK_SIZE_UNKNOWN && minor_version == 0)
	continue;
	
      switch (minor_version)
	{
	case 0:
	  sector = (size - 8 * 2) & ~(4 * 2 - 1);
	  break;
	case 1:
	  sector = 0;
	  break;
	case 2:
	  sector = 4 * 2;
	  break;
	}

      if (holy_disk_read (disk, sector, 0, sizeof (struct holy_raid_super_1x),
			  &sb))
	return NULL;

      if (sb.magic != holy_cpu_to_le32_compile_time (SB_MAGIC)
	  || holy_le_to_cpu64 (sb.super_offset) != sector)
	continue;

      if (sb.major_version != holy_cpu_to_le32_compile_time (1))
	/* Unsupported version.  */
	return NULL;

      level = holy_le_to_cpu32 (sb.level);

      /* Multipath.  */
      if ((int) level == -4)
	level = 1;

      if (level != 0 && level != 1 && level != 4 &&
	  level != 5 && level != 6 && level != 10)
	{
	  holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		      "Unsupported RAID level: %d", sb.level);
	  return NULL;
	}

      if (holy_le_to_cpu32 (sb.dev_number) >=
	  holy_le_to_cpu32 (sb.max_dev))
	/* Spares aren't implemented.  */
	return NULL;

      if (holy_disk_read (disk, sector,
			  (char *) &sb.dev_roles[holy_le_to_cpu32 (sb.dev_number)]
			  - (char *) &sb,
			  sizeof (role), &role))
	return NULL;

      if (holy_le_to_cpu16 (role)
	  >= holy_le_to_cpu32 (sb.raid_disks))
	/* Spares aren't implemented.  */
	return NULL;

      id->uuidlen = 0;
      id->id = holy_le_to_cpu16 (role);

      uuid = holy_malloc (16);
      if (!uuid)
	return NULL;

      holy_memcpy (uuid, sb.set_uuid, 16);

      *start_sector = holy_le_to_cpu64 (sb.data_offset);

      array = holy_diskfilter_make_raid (16, uuid,
					 holy_le_to_cpu32 (sb.raid_disks),
					 sb.set_name,
					 (sb.size)
					 ? holy_le_to_cpu64 (sb.size)
					 : holy_le_to_cpu64 (sb.data_size),
					 holy_le_to_cpu32 (sb.chunksize),
					 holy_le_to_cpu32 (sb.layout),
					 holy_le_to_cpu32 (sb.level));

      return array;
    }

  /* not 1.x raid.  */
  return NULL;
}

static struct holy_diskfilter holy_mdraid_dev = {
  .name = "mdraid1x",
  .detect = holy_mdraid_detect,
  .next = 0
};

holy_MOD_INIT (mdraid1x)
{
  holy_diskfilter_register_front (&holy_mdraid_dev);
}

holy_MOD_FINI (mdraid1x)
{
  holy_diskfilter_unregister (&holy_mdraid_dev);
}
