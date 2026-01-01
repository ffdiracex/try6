/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_GPT_PARTITION_HEADER
#define holy_GPT_PARTITION_HEADER	1

#include <holy/types.h>
#include <holy/partition.h>
#include <holy/msdos_partition.h>

struct holy_gpt_guid
{
  holy_uint32_t data1;
  holy_uint16_t data2;
  holy_uint16_t data3;
  holy_uint8_t data4[8];
} holy_PACKED;
typedef struct holy_gpt_guid holy_gpt_guid_t;
typedef struct holy_gpt_guid holy_gpt_part_type_t;

/* Format the raw little-endian GUID as a newly allocated string.  */
char * holy_gpt_guid_to_str (holy_gpt_guid_t *guid);


#define holy_GPT_GUID_INIT(a, b, c, d1, d2, d3, d4, d5, d6, d7, d8)  \
  {					\
    holy_cpu_to_le32_compile_time (a),	\
    holy_cpu_to_le16_compile_time (b),	\
    holy_cpu_to_le16_compile_time (c),	\
    { d1, d2, d3, d4, d5, d6, d7, d8 }	\
  }

#define holy_GPT_PARTITION_TYPE_EMPTY \
  holy_GPT_GUID_INIT (0x0, 0x0, 0x0,  \
      0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0)

#define holy_GPT_PARTITION_TYPE_EFI_SYSTEM \
  holy_GPT_GUID_INIT (0xc12a7328, 0xf81f, 0x11d2, \
      0xba, 0x4b, 0x00, 0xa0, 0xc9, 0x3e, 0xc9, 0x3b)

#define holy_GPT_PARTITION_TYPE_BIOS_BOOT \
  holy_GPT_GUID_INIT (0x21686148, 0x6449, 0x6e6f, \
      0x74, 0x4e, 0x65, 0x65, 0x64, 0x45, 0x46, 0x49)

#define holy_GPT_PARTITION_TYPE_LDM \
  holy_GPT_GUID_INIT (0x5808c8aa, 0x7e8f, 0x42e0, \
      0x85, 0xd2, 0xe1, 0xe9, 0x04, 0x34, 0xcf, 0xb3)

#define holy_GPT_PARTITION_TYPE_USR_X86_64 \
  holy_GPT_GUID_INIT (0x5dfbf5f4, 0x2848, 0x4bac, \
      0xaa, 0x5e, 0x0d, 0x9a, 0x20, 0xb7, 0x45, 0xa6)

#define holy_GPT_HEADER_MAGIC \
  { 0x45, 0x46, 0x49, 0x20, 0x50, 0x41, 0x52, 0x54 }

#define holy_GPT_HEADER_VERSION	\
  holy_cpu_to_le32_compile_time (0x00010000U)

struct holy_gpt_header
{
  holy_uint8_t magic[8];
  holy_uint32_t version;
  holy_uint32_t headersize;
  holy_uint32_t crc32;
  holy_uint32_t unused1;
  holy_uint64_t header_lba;
  holy_uint64_t alternate_lba;
  holy_uint64_t start;
  holy_uint64_t end;
  holy_gpt_guid_t guid;
  holy_uint64_t partitions;
  holy_uint32_t maxpart;
  holy_uint32_t partentry_size;
  holy_uint32_t partentry_crc32;
} holy_PACKED;

struct holy_gpt_partentry
{
  holy_gpt_part_type_t type;
  holy_gpt_guid_t guid;
  holy_uint64_t start;
  holy_uint64_t end;
  holy_uint64_t attrib;
  holy_uint16_t name[36];
} holy_PACKED;

enum holy_gpt_part_attr_offset
{
  /* Standard partition attribute bits defined by UEFI.  */
  holy_GPT_PART_ATTR_OFFSET_REQUIRED			= 0,
  holy_GPT_PART_ATTR_OFFSET_NO_BLOCK_IO_PROTOCOL	= 1,
  holy_GPT_PART_ATTR_OFFSET_LEGACY_BIOS_BOOTABLE	= 2,

  /* De facto standard attribute bits defined by Microsoft and reused by
   * http://www.freedesktop.org/wiki/Specifications/DiscoverablePartitionsSpec */
  holy_GPT_PART_ATTR_OFFSET_READ_ONLY			= 60,
  holy_GPT_PART_ATTR_OFFSET_NO_AUTO			= 63,

  /* Partition attributes for priority based selection,
   * Currently only valid for PARTITION_TYPE_USR_X86_64.
   * TRIES_LEFT and PRIORITY are 4 bit wide fields.  */
  holy_GPT_PART_ATTR_OFFSET_GPTPRIO_PRIORITY		= 48,
  holy_GPT_PART_ATTR_OFFSET_GPTPRIO_TRIES_LEFT		= 52,
  holy_GPT_PART_ATTR_OFFSET_GPTPRIO_SUCCESSFUL		= 56,
};

/* Helpers for reading/writing partition attributes.  */
static inline holy_uint64_t
holy_gpt_entry_attribute (struct holy_gpt_partentry *entry,
			  enum holy_gpt_part_attr_offset offset,
			  unsigned int bits)
{
  holy_uint64_t attrib = holy_le_to_cpu64 (entry->attrib);

  return (attrib >> offset) & ((1ULL << bits) - 1);
}

static inline void
holy_gpt_entry_set_attribute (struct holy_gpt_partentry *entry,
			      holy_uint64_t value,
			      enum holy_gpt_part_attr_offset offset,
			      unsigned int bits)
{
  holy_uint64_t attrib, mask;

  mask = (((1ULL << bits) - 1) << offset);
  attrib = holy_le_to_cpu64 (entry->attrib) & ~mask;
  attrib |= ((value << offset) & mask);
  entry->attrib = holy_cpu_to_le64 (attrib);
}

/* Basic GPT partmap module.  */
holy_err_t
holy_gpt_partition_map_iterate (holy_disk_t disk,
				holy_partition_iterate_hook_t hook,
				void *hook_data);

/* Advanced GPT library.  */

/* Status bits for the holy_gpt.status field.  */
#define holy_GPT_PROTECTIVE_MBR		0x01
#define holy_GPT_HYBRID_MBR		0x02
#define holy_GPT_PRIMARY_HEADER_VALID	0x04
#define holy_GPT_PRIMARY_ENTRIES_VALID	0x08
#define holy_GPT_BACKUP_HEADER_VALID	0x10
#define holy_GPT_BACKUP_ENTRIES_VALID	0x20

/* UEFI requires the entries table to be at least 16384 bytes for a
 * total of 128 entries given the standard 128 byte entry size.  */
#define holy_GPT_DEFAULT_ENTRIES_SIZE	16384
#define holy_GPT_DEFAULT_ENTRIES_LENGTH	\
  (holy_GPT_DEFAULT_ENTRIES_SIZE / sizeof (struct holy_gpt_partentry))

struct holy_gpt
{
  /* Bit field indicating which structures on disk are valid.  */
  unsigned status;

  /* Protective or hybrid MBR.  */
  struct holy_msdos_partition_mbr mbr;

  /* Each of the two GPT headers.  */
  struct holy_gpt_header primary;
  struct holy_gpt_header backup;

  /* Only need one entries table, on disk both copies are identical.
   * The on disk entry size may be larger than our partentry struct so
   * the table cannot be indexed directly.  */
  void *entries;
  holy_size_t entries_size;

  /* Logarithm of sector size, in case GPT and disk driver disagree.  */
  unsigned int log_sector_size;
};
typedef struct holy_gpt *holy_gpt_t;

/* Helpers for checking the gpt status field.  */
static inline int
holy_gpt_mbr_valid (holy_gpt_t gpt)
{
  return ((gpt->status & holy_GPT_PROTECTIVE_MBR) ||
	  (gpt->status & holy_GPT_HYBRID_MBR));
}

static inline int
holy_gpt_primary_valid (holy_gpt_t gpt)
{
  return ((gpt->status & holy_GPT_PRIMARY_HEADER_VALID) &&
	  (gpt->status & holy_GPT_PRIMARY_ENTRIES_VALID));
}

static inline int
holy_gpt_backup_valid (holy_gpt_t gpt)
{
  return ((gpt->status & holy_GPT_BACKUP_HEADER_VALID) &&
	  (gpt->status & holy_GPT_BACKUP_ENTRIES_VALID));
}

static inline int
holy_gpt_both_valid (holy_gpt_t gpt)
{
  return holy_gpt_primary_valid (gpt) && holy_gpt_backup_valid (gpt);
}

/* Translate GPT sectors to holy's 512 byte block addresses.  */
static inline holy_disk_addr_t
holy_gpt_sector_to_addr (holy_gpt_t gpt, holy_uint64_t sector)
{
  return (sector << (gpt->log_sector_size - holy_DISK_SECTOR_BITS));
}

/* Allocates and fills new holy_gpt structure, free with holy_gpt_free.  */
holy_gpt_t holy_gpt_read (holy_disk_t disk);

/* Helper for indexing into the entries table.
 * Returns NULL when the end of the table has been reached.  */
struct holy_gpt_partentry * holy_gpt_get_partentry (holy_gpt_t gpt,
						    holy_uint32_t n);

/* Sync and update primary and backup headers if either are invalid.  */
holy_err_t holy_gpt_repair (holy_disk_t disk, holy_gpt_t gpt);

/* Recompute checksums and revalidate everything, must be called after
 * modifying any GPT data.  */
holy_err_t holy_gpt_update (holy_gpt_t gpt);

/* Write headers and entry tables back to disk.  */
holy_err_t holy_gpt_write (holy_disk_t disk, holy_gpt_t gpt);

void holy_gpt_free (holy_gpt_t gpt);

holy_err_t holy_gpt_pmbr_check (struct holy_msdos_partition_mbr *mbr);
holy_err_t holy_gpt_header_check (struct holy_gpt_header *gpt,
				  unsigned int log_sector_size);


/* Utilities for simple partition data lookups, usage is intended to
 * be similar to fs->label and fs->uuid functions.  */

/* Return the partition label of the device DEVICE in LABEL.
 * The label is in a new buffer and should be freed by the caller.  */
holy_err_t holy_gpt_part_label (holy_device_t device, char **label);

/* Return the partition uuid of the device DEVICE in UUID.
 * The uuid is in a new buffer and should be freed by the caller.  */
holy_err_t holy_gpt_part_uuid (holy_device_t device, char **uuid);

/* Return the disk uuid of the device DEVICE in UUID.
 * The uuid is in a new buffer and should be freed by the caller.  */
holy_err_t holy_gpt_disk_uuid (holy_device_t device, char **uuid);

#endif /* ! holy_GPT_PARTITION_HEADER */
