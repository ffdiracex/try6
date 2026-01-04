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

/* Linux RAID on disk structures and constants,
   copied from include/linux/raid/md_p.h.  */

holy_MOD_LICENSE ("GPLv2+");

#ifdef MODE_BIGENDIAN
#define holy_md_to_cpu64 holy_be_to_cpu64
#define holy_md_to_cpu32 holy_be_to_cpu32
#define holy_md_to_cpu16 holy_be_to_cpu16
#define holy_cpu_to_md64_compile_time holy_cpu_to_be64_compile_time
#define holy_cpu_to_md32_compile_time holy_cpu_to_be32_compile_time
#define holy_cpu_to_md16_compile_time holy_cpu_to_be16_compile_time
#else
#define holy_md_to_cpu64 holy_le_to_cpu64
#define holy_md_to_cpu32 holy_le_to_cpu32
#define holy_md_to_cpu16 holy_le_to_cpu16
#define holy_cpu_to_md64_compile_time holy_cpu_to_le64_compile_time
#define holy_cpu_to_md32_compile_time holy_cpu_to_le32_compile_time
#define holy_cpu_to_md16_compile_time holy_cpu_to_le16_compile_time
#endif

#define RESERVED_BYTES			(64 * 1024)
#define RESERVED_SECTORS		(RESERVED_BYTES / 512)

#define NEW_SIZE_SECTORS(x)		((x & ~(RESERVED_SECTORS - 1)) \
					- RESERVED_SECTORS)

#define SB_BYTES			4096
#define SB_WORDS			(SB_BYTES / 4)
#define SB_SECTORS			(SB_BYTES / 512)

/*
 * The following are counted in 32-bit words
 */
#define	SB_GENERIC_OFFSET		0

#define SB_PERSONALITY_OFFSET		64
#define SB_DISKS_OFFSET			128
#define SB_DESCRIPTOR_OFFSET		992

#define SB_GENERIC_CONSTANT_WORDS	32
#define SB_GENERIC_STATE_WORDS		32
#define SB_GENERIC_WORDS		(SB_GENERIC_CONSTANT_WORDS + \
                                         SB_GENERIC_STATE_WORDS)

#define SB_PERSONALITY_WORDS		64
#define SB_DESCRIPTOR_WORDS		32
#define SB_DISKS			27
#define SB_DISKS_WORDS			(SB_DISKS * SB_DESCRIPTOR_WORDS)

#define SB_RESERVED_WORDS		(1024 \
                                         - SB_GENERIC_WORDS \
                                         - SB_PERSONALITY_WORDS \
                                         - SB_DISKS_WORDS \
                                         - SB_DESCRIPTOR_WORDS)

#define SB_EQUAL_WORDS			(SB_GENERIC_WORDS \
                                         + SB_PERSONALITY_WORDS \
                                         + SB_DISKS_WORDS)

/*
 * Device "operational" state bits
 */
#define DISK_FAULTY			0
#define DISK_ACTIVE			1
#define DISK_SYNC			2
#define DISK_REMOVED			3

#define	DISK_WRITEMOSTLY		9

#define SB_MAGIC			0xa92b4efc

/*
 * Superblock state bits
 */
#define SB_CLEAN			0
#define SB_ERRORS			1

#define	SB_BITMAP_PRESENT		8

struct holy_raid_disk_09
{
  holy_uint32_t number;		/* Device number in the entire set.  */
  holy_uint32_t major;		/* Device major number.  */
  holy_uint32_t minor;		/* Device minor number.  */
  holy_uint32_t raid_disk;	/* The role of the device in the raid set.  */
  holy_uint32_t state;		/* Operational state.  */
  holy_uint32_t reserved[SB_DESCRIPTOR_WORDS - 5];
};

struct holy_raid_super_09
{
  /*
   * Constant generic information
   */
  holy_uint32_t md_magic;	/* MD identifier.  */
  holy_uint32_t major_version;	/* Major version.  */
  holy_uint32_t minor_version;	/* Minor version.  */
  holy_uint32_t patch_version;	/* Patchlevel version.  */
  holy_uint32_t gvalid_words;	/* Number of used words in this section.  */
  holy_uint32_t set_uuid0;	/* Raid set identifier.  */
  holy_uint32_t ctime;		/* Creation time.  */
  holy_uint32_t level;		/* Raid personality.  */
  holy_uint32_t size;		/* Apparent size of each individual disk.  */
  holy_uint32_t nr_disks;	/* Total disks in the raid set.  */
  holy_uint32_t raid_disks;	/* Disks in a fully functional raid set.  */
  holy_uint32_t md_minor;	/* Preferred MD minor device number.  */
  holy_uint32_t not_persistent;	/* Does it have a persistent superblock.  */
  holy_uint32_t set_uuid1;	/* Raid set identifier #2.  */
  holy_uint32_t set_uuid2;	/* Raid set identifier #3.  */
  holy_uint32_t set_uuid3;	/* Raid set identifier #4.  */
  holy_uint32_t gstate_creserved[SB_GENERIC_CONSTANT_WORDS - 16];

  /*
   * Generic state information
   */
  holy_uint32_t utime;		/* Superblock update time.  */
  holy_uint32_t state;		/* State bits (clean, ...).  */
  holy_uint32_t active_disks;	/* Number of currently active disks.  */
  holy_uint32_t working_disks;	/* Number of working disks.  */
  holy_uint32_t failed_disks;	/* Number of failed disks.  */
  holy_uint32_t spare_disks;	/* Number of spare disks.  */
  holy_uint32_t sb_csum;	/* Checksum of the whole superblock.  */
  holy_uint64_t events;		/* Superblock update count.  */
  holy_uint64_t cp_events;	/* Checkpoint update count.  */
  holy_uint32_t recovery_cp;	/* Recovery checkpoint sector count.  */
  holy_uint32_t gstate_sreserved[SB_GENERIC_STATE_WORDS - 12];

  /*
   * Personality information
   */
  holy_uint32_t layout;		/* The array's physical layout.  */
  holy_uint32_t chunk_size;	/* Chunk size in bytes.  */
  holy_uint32_t root_pv;	/* LV root PV.  */
  holy_uint32_t root_block;	/* LV root block.  */
  holy_uint32_t pstate_reserved[SB_PERSONALITY_WORDS - 4];

  /*
   * Disks information
   */
  struct holy_raid_disk_09 disks[SB_DISKS];

  /*
   * Reserved
   */
  holy_uint32_t reserved[SB_RESERVED_WORDS];

  /*
   * Active descriptor
   */
  struct holy_raid_disk_09 this_disk;
} holy_PACKED;

static struct holy_diskfilter_vg *
holy_mdraid_detect (holy_disk_t disk,
		    struct holy_diskfilter_pv_id *id,
		    holy_disk_addr_t *start_sector)
{
  holy_disk_addr_t sector;
  holy_uint64_t size;
  struct holy_raid_super_09 *sb = NULL;
  holy_uint32_t *uuid;
  holy_uint32_t level;
  struct holy_diskfilter_vg *ret;

  /* The sector where the mdraid 0.90 superblock is stored, if available.  */
  size = holy_disk_get_size (disk);
  if (size == holy_DISK_SIZE_UNKNOWN)
    /* not 0.9x raid.  */
    return NULL;
  sector = NEW_SIZE_SECTORS (size);

  sb = holy_malloc (sizeof (*sb));
  if (!sb)
    return NULL;

  if (holy_disk_read (disk, sector, 0, SB_BYTES, sb))
    goto fail;

  /* Look whether there is a mdraid 0.90 superblock.  */
  if (sb->md_magic != holy_cpu_to_md32_compile_time (SB_MAGIC))
    /* not 0.9x raid.  */
    goto fail;

  if (sb->major_version != holy_cpu_to_md32_compile_time (0)
      || sb->minor_version != holy_cpu_to_md32_compile_time (90))
    /* Unsupported version.  */
    goto fail;

  /* No need for explicit check that sb->size is 0 (unspecified) since
     0 >= non-0 is false.  */
  if (((holy_disk_addr_t) holy_md_to_cpu32 (sb->size)) * 2 >= size)
    goto fail;

  /* FIXME: Check the checksum.  */

  level = holy_md_to_cpu32 (sb->level);
  /* Multipath.  */
  if ((int) level == -4)
    level = 1;

  if (level != 0 && level != 1 && level != 4 &&
      level != 5 && level != 6 && level != 10)
    {
      holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		  "unsupported RAID level: %d", level);
      goto fail;
    }
  if (holy_md_to_cpu32 (sb->this_disk.number) == 0xffff
      || holy_md_to_cpu32 (sb->this_disk.number) == 0xfffe)
    /* Spares aren't implemented.  */
    goto fail;

  uuid = holy_malloc (16);
  if (!uuid)
    goto fail;

  uuid[0] = holy_swap_bytes32 (sb->set_uuid0);
  uuid[1] = holy_swap_bytes32 (sb->set_uuid1);
  uuid[2] = holy_swap_bytes32 (sb->set_uuid2);
  uuid[3] = holy_swap_bytes32 (sb->set_uuid3);

  *start_sector = 0;

  id->uuidlen = 0;
  id->id = holy_md_to_cpu32 (sb->this_disk.number);

  char buf[32];
  holy_snprintf (buf, sizeof (buf), "md%d", holy_md_to_cpu32 (sb->md_minor));
  ret = holy_diskfilter_make_raid (16, (char *) uuid,
				   holy_md_to_cpu32 (sb->raid_disks), buf,
				   (sb->size) ? ((holy_disk_addr_t)
						 holy_md_to_cpu32 (sb->size)) * 2
				   : sector,
				   holy_md_to_cpu32 (sb->chunk_size) >> 9,
				   holy_md_to_cpu32 (sb->layout),
				   level);
  holy_free (sb);
  return ret;

 fail:
  holy_free (sb);
  return NULL;
}

static struct holy_diskfilter holy_mdraid_dev = {
#ifdef MODE_BIGENDIAN
  .name = "mdraid09_be",
#else
  .name = "mdraid09",
#endif
  .detect = holy_mdraid_detect,
  .next = 0
};

#ifdef MODE_BIGENDIAN
holy_MOD_INIT (mdraid09_be)
#else
holy_MOD_INIT (mdraid09)
#endif
{
  holy_diskfilter_register_front (&holy_mdraid_dev);
}

#ifdef MODE_BIGENDIAN
holy_MOD_FINI (mdraid09_be)
#else
holy_MOD_FINI (mdraid09)
#endif
{
  holy_diskfilter_unregister (&holy_mdraid_dev);
}
