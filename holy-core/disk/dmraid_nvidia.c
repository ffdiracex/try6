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

#define NV_SIGNATURES		4

#define NV_IDLE			0
#define NV_SCDB_INIT_RAID	2
#define NV_SCDB_REBUILD_RAID	3
#define NV_SCDB_UPGRADE_RAID	4
#define NV_SCDB_SYNC_RAID	5

#define NV_LEVEL_UNKNOWN	0x00
#define NV_LEVEL_JBOD		0xFF
#define NV_LEVEL_0		0x80
#define NV_LEVEL_1		0x81
#define NV_LEVEL_3		0x83
#define NV_LEVEL_5		0x85
#define NV_LEVEL_10		0x8a
#define NV_LEVEL_1_0		0x8180

#define NV_ARRAY_FLAG_BOOT		1 /* BIOS use only.  */
#define NV_ARRAY_FLAG_ERROR		2 /* Degraded or offline.  */
#define NV_ARRAY_FLAG_PARITY_VALID	4 /* RAID-3/5 parity valid.  */

struct holy_nv_array
{
  holy_uint32_t version;
  holy_uint32_t signature[NV_SIGNATURES];
  holy_uint8_t raid_job_code;
  holy_uint8_t stripe_width;
  holy_uint8_t total_volumes;
  holy_uint8_t original_width;
  holy_uint32_t raid_level;
  holy_uint32_t stripe_block_size;
  holy_uint32_t stripe_block_size_bytes;
  holy_uint32_t stripe_block_size_log2;
  holy_uint32_t stripe_mask;
  holy_uint32_t stripe_size;
  holy_uint32_t stripe_size_bytes;
  holy_uint32_t raid_job_mask;
  holy_uint32_t original_capacity;
  holy_uint32_t flags;
};

#define NV_ID_LEN		8
#define NV_ID_STRING		"NVIDIA"
#define NV_VERSION		100

#define	NV_PRODID_LEN		16
#define	NV_PRODREV_LEN		4

struct holy_nv_super
{
  char vendor[NV_ID_LEN];	/* 0x00 - 0x07 ID string.  */
  holy_uint32_t size;		/* 0x08 - 0x0B Size of metadata in dwords.  */
  holy_uint32_t chksum;		/* 0x0C - 0x0F Checksum of this struct.  */
  holy_uint16_t version;	/* 0x10 - 0x11 NV version.  */
  holy_uint8_t unit_number;	/* 0x12 Disk index in array.  */
  holy_uint8_t reserved;	/* 0x13.  */
  holy_uint32_t capacity;	/* 0x14 - 0x17 Array capacity in sectors.  */
  holy_uint32_t sector_size;	/* 0x18 - 0x1B Sector size.  */
  char prodid[NV_PRODID_LEN];	/* 0x1C - 0x2B Array product ID.  */
  char prodrev[NV_PRODREV_LEN];	/* 0x2C - 0x2F Array product revision */
  holy_uint32_t unit_flags;	/* 0x30 - 0x33 Flags for this disk */
  struct holy_nv_array array;	/* Array information */
} holy_PACKED;

static struct holy_diskfilter_vg *
holy_dmraid_nv_detect (holy_disk_t disk,
			struct holy_diskfilter_pv_id *id,
                        holy_disk_addr_t *start_sector)
{
  holy_disk_addr_t sector;
  struct holy_nv_super sb;
  int level;
  holy_uint64_t disk_size;
  holy_uint32_t capacity;
  holy_uint8_t total_volumes;
  char *uuid;

  if (disk->partition)
    /* Skip partition.  */
    return NULL;

  sector = holy_disk_get_size (disk);
  if (sector == holy_DISK_SIZE_UNKNOWN)
    /* Not raid.  */
    return NULL;
  sector -= 2;
  if (holy_disk_read (disk, sector, 0, sizeof (sb), &sb))
    return NULL;

  if (holy_memcmp (sb.vendor, NV_ID_STRING, 6))
    /* Not raid.  */
    return NULL;

  if (sb.version != NV_VERSION)
    {
      holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		  "unknown version: %d.%d", sb.version);
      return NULL;
    }

  capacity = holy_le_to_cpu32 (sb.capacity);
  total_volumes = sb.array.total_volumes;

  switch (sb.array.raid_level)
    {
    case NV_LEVEL_0:
      level = 0;
      if (total_volumes == 0)
	/* Not RAID.  */
	return NULL;
      disk_size = capacity / total_volumes;
      break;

    case NV_LEVEL_1:
      level = 1;
      disk_size = sb.capacity;
      break;

    case NV_LEVEL_5:
      level = 5;
      if (total_volumes == 0 || total_volumes == 1)
	/* Not RAID.  */
	return NULL;
      disk_size = capacity / (total_volumes - 1);
      break;

    default:
      holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		  "unsupported RAID level: %d", sb.array.raid_level);
      return NULL;
    }

  uuid = holy_malloc (sizeof (sb.array.signature));
  if (! uuid)
    return NULL;

  holy_memcpy (uuid, (char *) &sb.array.signature,
               sizeof (sb.array.signature));

  id->uuidlen = 0;
  id->id = sb.unit_number;

  *start_sector = 0;

  return holy_diskfilter_make_raid (sizeof (sb.array.signature),
				    uuid, sb.array.total_volumes,
				    "nv", disk_size,
				    sb.array.stripe_block_size,
				    holy_RAID_LAYOUT_LEFT_ASYMMETRIC,
				    level);
}

static struct holy_diskfilter holy_dmraid_nv_dev =
{
  .name = "dmraid_nv",
  .detect = holy_dmraid_nv_detect,
  .next = 0
};

holy_MOD_INIT(dm_nv)
{
  holy_diskfilter_register_front (&holy_dmraid_nv_dev);
}

holy_MOD_FINI(dm_nv)
{
  holy_diskfilter_unregister (&holy_dmraid_nv_dev);
}
