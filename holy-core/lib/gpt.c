/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/charset.h>
#include <holy/crypto.h>
#include <holy/device.h>
#include <holy/disk.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/dl.h>
#include <holy/msdos_partition.h>
#include <holy/gpt_partition.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_uint8_t holy_gpt_magic[] = holy_GPT_HEADER_MAGIC;

static holy_err_t
holy_gpt_read_entries (holy_disk_t disk, holy_gpt_t gpt,
		       struct holy_gpt_header *header,
		       void **ret_entries,
		       holy_size_t *ret_entries_size);

char *
holy_gpt_guid_to_str (holy_gpt_guid_t *guid)
{
  return holy_xasprintf ("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			 holy_le_to_cpu32 (guid->data1),
			 holy_le_to_cpu16 (guid->data2),
			 holy_le_to_cpu16 (guid->data3),
			 guid->data4[0], guid->data4[1],
			 guid->data4[2], guid->data4[3],
			 guid->data4[4], guid->data4[5],
			 guid->data4[6], guid->data4[7]);
}

static holy_err_t
holy_gpt_device_partentry (holy_device_t device,
			   struct holy_gpt_partentry *entry)
{
  holy_disk_t disk = device->disk;
  holy_partition_t p;
  holy_err_t err;

  if (!disk || !disk->partition)
    return holy_error (holy_ERR_BUG, "not a partition");

  if (holy_strcmp (disk->partition->partmap->name, "gpt"))
    return holy_error (holy_ERR_BAD_ARGUMENT, "not a GPT partition");

  p = disk->partition;
  disk->partition = p->parent;
  err = holy_disk_read (disk, p->offset, p->index, sizeof (*entry), entry);
  disk->partition = p;

  return err;
}

holy_err_t
holy_gpt_part_label (holy_device_t device, char **label)
{
  struct holy_gpt_partentry entry;
  const holy_size_t name_len = ARRAY_SIZE (entry.name);
  const holy_size_t label_len = name_len * holy_MAX_UTF8_PER_UTF16 + 1;
  holy_size_t i;
  holy_uint8_t *end;

  if (holy_gpt_device_partentry (device, &entry))
    return holy_errno;

  *label = holy_malloc (label_len);
  if (!*label)
    return holy_errno;

  for (i = 0; i < name_len; i++)
    entry.name[i] = holy_le_to_cpu16 (entry.name[i]);

  end = holy_utf16_to_utf8 ((holy_uint8_t *) *label, entry.name, name_len);
  *end = '\0';

  return holy_ERR_NONE;
}

holy_err_t
holy_gpt_part_uuid (holy_device_t device, char **uuid)
{
  struct holy_gpt_partentry entry;

  if (holy_gpt_device_partentry (device, &entry))
    return holy_errno;

  *uuid = holy_gpt_guid_to_str (&entry.guid);
  if (!*uuid)
    return holy_errno;

  return holy_ERR_NONE;
}

static struct holy_gpt_header *
holy_gpt_get_header (holy_gpt_t gpt)
{
  if (gpt->status & holy_GPT_PRIMARY_HEADER_VALID)
    return &gpt->primary;
  else if (gpt->status & holy_GPT_BACKUP_HEADER_VALID)
    return &gpt->backup;

  holy_error (holy_ERR_BUG, "No valid GPT header");
  return NULL;
}

holy_err_t
holy_gpt_disk_uuid (holy_device_t device, char **uuid)
{
  struct holy_gpt_header *header;

  holy_gpt_t gpt = holy_gpt_read (device->disk);
  if (!gpt)
    goto done;

  header = holy_gpt_get_header (gpt);
  if (!header)
    goto done;

  *uuid = holy_gpt_guid_to_str (&header->guid);

done:
  holy_gpt_free (gpt);
  return holy_errno;
}

static holy_uint64_t
holy_gpt_size_to_sectors (holy_gpt_t gpt, holy_size_t size)
{
  unsigned int sector_size;
  holy_uint64_t sectors;

  sector_size = 1U << gpt->log_sector_size;
  sectors = size / sector_size;
  if (size % sector_size)
    sectors++;

  return sectors;
}

/* Copied from holy-core/kern/disk_common.c holy_disk_adjust_range so we can
 * avoid attempting to use disk->total_sectors when holy won't let us.
 * TODO: Why is disk->total_sectors not set to holy_DISK_SIZE_UNKNOWN?  */
static int
holy_gpt_disk_size_valid (holy_disk_t disk)
{
  holy_disk_addr_t total_sectors;

  /* Transform total_sectors to number of 512B blocks.  */
  total_sectors = disk->total_sectors << (disk->log_sector_size - holy_DISK_SECTOR_BITS);

  /* Some drivers have problems with disks above reasonable.
     Treat unknown as 1EiB disk. While on it, clamp the size to 1EiB.
     Just one condition is enough since holy_DISK_UNKNOWN_SIZE << ls is always
     above 9EiB.
  */
  if (total_sectors > (1ULL << 51))
    return 0;

  return 1;
}

static void
holy_gpt_lecrc32 (holy_uint32_t *crc, const void *data, holy_size_t len)
{
  holy_uint32_t crc32_val;

  holy_crypto_hash (holy_MD_CRC32, &crc32_val, data, len);

  /* holy_MD_CRC32 always uses big endian, gpt is always little.  */
  *crc = holy_swap_bytes32 (crc32_val);
}

static void
holy_gpt_header_lecrc32 (holy_uint32_t *crc, struct holy_gpt_header *header)
{
  holy_uint32_t old, new;

  /* crc32 must be computed with the field cleared.  */
  old = header->crc32;
  header->crc32 = 0;
  holy_gpt_lecrc32 (&new, header, sizeof (*header));
  header->crc32 = old;

  *crc = new;
}

/* Make sure the MBR is a protective MBR and not a normal MBR.  */
holy_err_t
holy_gpt_pmbr_check (struct holy_msdos_partition_mbr *mbr)
{
  unsigned int i;

  if (mbr->signature !=
      holy_cpu_to_le16_compile_time (holy_PC_PARTITION_SIGNATURE))
    return holy_error (holy_ERR_BAD_PART_TABLE, "invalid MBR signature");

  for (i = 0; i < sizeof (mbr->entries); i++)
    if (mbr->entries[i].type == holy_PC_PARTITION_TYPE_GPT_DISK)
      return holy_ERR_NONE;

  return holy_error (holy_ERR_BAD_PART_TABLE, "invalid protective MBR");
}

static holy_uint64_t
holy_gpt_entries_size (struct holy_gpt_header *gpt)
{
  return (holy_uint64_t) holy_le_to_cpu32 (gpt->maxpart) *
         (holy_uint64_t) holy_le_to_cpu32 (gpt->partentry_size);
}

static holy_uint64_t
holy_gpt_entries_sectors (struct holy_gpt_header *gpt,
			  unsigned int log_sector_size)
{
  holy_uint64_t sector_bytes, entries_bytes;

  sector_bytes = 1ULL << log_sector_size;
  entries_bytes = holy_gpt_entries_size (gpt);
  return holy_divmod64(entries_bytes + sector_bytes - 1, sector_bytes, NULL);
}

static int
is_pow2 (holy_uint32_t n)
{
  return (n & (n - 1)) == 0;
}

holy_err_t
holy_gpt_header_check (struct holy_gpt_header *gpt,
		       unsigned int log_sector_size)
{
  holy_uint32_t crc = 0, size;
  holy_uint64_t start, end;

  if (holy_memcmp (gpt->magic, holy_gpt_magic, sizeof (holy_gpt_magic)) != 0)
    return holy_error (holy_ERR_BAD_PART_TABLE, "invalid GPT signature");

  if (gpt->version != holy_GPT_HEADER_VERSION)
    return holy_error (holy_ERR_BAD_PART_TABLE, "unknown GPT version");

  holy_gpt_header_lecrc32 (&crc, gpt);
  if (gpt->crc32 != crc)
    return holy_error (holy_ERR_BAD_PART_TABLE, "invalid GPT header crc32");

  /* The header size "must be greater than or equal to 92 and must be less
   * than or equal to the logical block size."  */
  size = holy_le_to_cpu32 (gpt->headersize);
  if (size < 92U || size > (1U << log_sector_size))
    return holy_error (holy_ERR_BAD_PART_TABLE, "invalid GPT header size");

  /* The partition entry size must be "a value of 128*(2^n) where n is an
   * integer greater than or equal to zero (e.g., 128, 256, 512, etc.)."  */
  size = holy_le_to_cpu32 (gpt->partentry_size);
  if (size < 128U || size % 128U || !is_pow2 (size / 128U))
    return holy_error (holy_ERR_BAD_PART_TABLE, "invalid GPT entry size");

  /* The minimum entries table size is specified in terms of bytes,
   * regardless of how large the individual entry size is.  */
  if (holy_gpt_entries_size (gpt) < holy_GPT_DEFAULT_ENTRIES_SIZE)
    return holy_error (holy_ERR_BAD_PART_TABLE, "invalid GPT entry table size");

  /* And of course there better be some space for partitions!  */
  start = holy_le_to_cpu64 (gpt->start);
  end = holy_le_to_cpu64 (gpt->end);
  if (start > end)
    return holy_error (holy_ERR_BAD_PART_TABLE, "invalid usable sectors");

  return holy_ERR_NONE;
}

static int
holy_gpt_headers_equal (holy_gpt_t gpt)
{
  /* Assume headers passed holy_gpt_header_check so skip magic and version.
   * Individual fields must be checked instead of just using memcmp because
   * crc32, header, alternate, and partitions will all normally differ.  */

  if (gpt->primary.headersize != gpt->backup.headersize ||
      gpt->primary.header_lba != gpt->backup.alternate_lba ||
      gpt->primary.alternate_lba != gpt->backup.header_lba ||
      gpt->primary.start != gpt->backup.start ||
      gpt->primary.end != gpt->backup.end ||
      gpt->primary.maxpart != gpt->backup.maxpart ||
      gpt->primary.partentry_size != gpt->backup.partentry_size ||
      gpt->primary.partentry_crc32 != gpt->backup.partentry_crc32)
    return 0;

  return holy_memcmp(&gpt->primary.guid, &gpt->backup.guid,
                     sizeof(holy_gpt_guid_t)) == 0;
}

static holy_err_t
holy_gpt_check_primary (holy_gpt_t gpt)
{
  holy_uint64_t backup, primary, entries, entries_len, start, end;

  primary = holy_le_to_cpu64 (gpt->primary.header_lba);
  backup = holy_le_to_cpu64 (gpt->primary.alternate_lba);
  entries = holy_le_to_cpu64 (gpt->primary.partitions);
  entries_len = holy_gpt_entries_sectors(&gpt->primary, gpt->log_sector_size);
  start = holy_le_to_cpu64 (gpt->primary.start);
  end = holy_le_to_cpu64 (gpt->primary.end);

  holy_dprintf ("gpt", "Primary GPT layout:\n"
		"primary header = 0x%llx backup header = 0x%llx\n"
		"entries location = 0x%llx length = 0x%llx\n"
		"first usable = 0x%llx last usable = 0x%llx\n",
		(unsigned long long) primary,
		(unsigned long long) backup,
		(unsigned long long) entries,
		(unsigned long long) entries_len,
		(unsigned long long) start,
		(unsigned long long) end);

  if (holy_gpt_header_check (&gpt->primary, gpt->log_sector_size))
    return holy_errno;
  if (primary != 1)
    return holy_error (holy_ERR_BAD_PART_TABLE, "invalid primary GPT LBA");
  if (entries <= 1 || entries+entries_len > start)
    return holy_error (holy_ERR_BAD_PART_TABLE, "invalid entries location");
  if (backup <= end)
    return holy_error (holy_ERR_BAD_PART_TABLE, "invalid backup GPT LBA");

  return holy_ERR_NONE;
}

static holy_err_t
holy_gpt_check_backup (holy_gpt_t gpt)
{
  holy_uint64_t backup, primary, entries, entries_len, start, end;

  backup = holy_le_to_cpu64 (gpt->backup.header_lba);
  primary = holy_le_to_cpu64 (gpt->backup.alternate_lba);
  entries = holy_le_to_cpu64 (gpt->backup.partitions);
  entries_len = holy_gpt_entries_sectors(&gpt->backup, gpt->log_sector_size);
  start = holy_le_to_cpu64 (gpt->backup.start);
  end = holy_le_to_cpu64 (gpt->backup.end);

  holy_dprintf ("gpt", "Backup GPT layout:\n"
		"primary header = 0x%llx backup header = 0x%llx\n"
		"entries location = 0x%llx length = 0x%llx\n"
		"first usable = 0x%llx last usable = 0x%llx\n",
		(unsigned long long) primary,
		(unsigned long long) backup,
		(unsigned long long) entries,
		(unsigned long long) entries_len,
		(unsigned long long) start,
		(unsigned long long) end);

  if (holy_gpt_header_check (&gpt->backup, gpt->log_sector_size))
    return holy_errno;
  if (primary != 1)
    return holy_error (holy_ERR_BAD_PART_TABLE, "invalid primary GPT LBA");
  if (entries <= end || entries+entries_len > backup)
    return holy_error (holy_ERR_BAD_PART_TABLE, "invalid entries location");
  if (backup <= end)
    return holy_error (holy_ERR_BAD_PART_TABLE, "invalid backup GPT LBA");

  /* If both primary and backup are valid but differ prefer the primary.  */
  if ((gpt->status & holy_GPT_PRIMARY_HEADER_VALID) &&
      !holy_gpt_headers_equal (gpt))
    return holy_error (holy_ERR_BAD_PART_TABLE, "backup GPT out of sync");

  return holy_ERR_NONE;
}

static holy_err_t
holy_gpt_read_primary (holy_disk_t disk, holy_gpt_t gpt)
{
  holy_disk_addr_t addr;

  /* TODO: The gpt partmap module searches for the primary header instead
   * of relying on the disk's sector size. For now trust the disk driver
   * but eventually this code should match the existing behavior.  */
  gpt->log_sector_size = disk->log_sector_size;

  holy_dprintf ("gpt", "reading primary GPT from sector 0x1\n");

  addr = holy_gpt_sector_to_addr (gpt, 1);
  if (holy_disk_read (disk, addr, 0, sizeof (gpt->primary), &gpt->primary))
    return holy_errno;

  if (holy_gpt_check_primary (gpt))
    return holy_errno;

  gpt->status |= holy_GPT_PRIMARY_HEADER_VALID;

  if (holy_gpt_read_entries (disk, gpt, &gpt->primary,
			     &gpt->entries, &gpt->entries_size))
    return holy_errno;

  gpt->status |= holy_GPT_PRIMARY_ENTRIES_VALID;

  return holy_ERR_NONE;
}

static holy_err_t
holy_gpt_read_backup (holy_disk_t disk, holy_gpt_t gpt)
{
  void *entries = NULL;
  holy_size_t entries_size;
  holy_uint64_t sector;
  holy_disk_addr_t addr;

  /* Assumes gpt->log_sector_size == disk->log_sector_size  */
  if (gpt->status & holy_GPT_PRIMARY_HEADER_VALID)
    {
      sector = holy_le_to_cpu64 (gpt->primary.alternate_lba);
      if (holy_gpt_disk_size_valid (disk) && sector >= disk->total_sectors)
	return holy_error (holy_ERR_OUT_OF_RANGE,
			   "backup GPT located at 0x%llx, "
			   "beyond last disk sector at 0x%llx",
			   (unsigned long long) sector,
			   (unsigned long long) disk->total_sectors - 1);
    }
  else if (holy_gpt_disk_size_valid (disk))
    sector = disk->total_sectors - 1;
  else
    return holy_error (holy_ERR_OUT_OF_RANGE,
		       "size of disk unknown, cannot locate backup GPT");

  holy_dprintf ("gpt", "reading backup GPT from sector 0x%llx\n",
		(unsigned long long) sector);

  addr = holy_gpt_sector_to_addr (gpt, sector);
  if (holy_disk_read (disk, addr, 0, sizeof (gpt->backup), &gpt->backup))
    return holy_errno;

  if (holy_gpt_check_backup (gpt))
    return holy_errno;

  /* Ensure the backup header thinks it is located where we found it.  */
  if (holy_le_to_cpu64 (gpt->backup.header_lba) != sector)
    return holy_error (holy_ERR_BAD_PART_TABLE, "invalid backup GPT LBA");

  gpt->status |= holy_GPT_BACKUP_HEADER_VALID;

  if (holy_gpt_read_entries (disk, gpt, &gpt->backup,
			     &entries, &entries_size))
    return holy_errno;

  if (gpt->status & holy_GPT_PRIMARY_ENTRIES_VALID)
    {
      if (entries_size != gpt->entries_size ||
	  holy_memcmp (entries, gpt->entries, entries_size) != 0)
	return holy_error (holy_ERR_BAD_PART_TABLE, "backup GPT out of sync");

      holy_free (entries);
    }
  else
    {
      gpt->entries = entries;
      gpt->entries_size = entries_size;
    }

  gpt->status |= holy_GPT_BACKUP_ENTRIES_VALID;

  return holy_ERR_NONE;
}

static holy_err_t
holy_gpt_read_entries (holy_disk_t disk, holy_gpt_t gpt,
		       struct holy_gpt_header *header,
		       void **ret_entries,
		       holy_size_t *ret_entries_size)
{
  void *entries = NULL;
  holy_uint32_t count, size, crc;
  holy_uint64_t sector;
  holy_disk_addr_t addr;
  holy_size_t entries_size;

  /* holy doesn't include calloc, hence the manual overflow check.  */
  count = holy_le_to_cpu32 (header->maxpart);
  size = holy_le_to_cpu32 (header->partentry_size);
  entries_size = count *size;
  if (size && entries_size / size != count)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, N_("out of memory"));
      goto fail;
    }

  /* Double check that the header was validated properly.  */
  if (entries_size < holy_GPT_DEFAULT_ENTRIES_SIZE)
    return holy_error (holy_ERR_BUG, "invalid GPT entries table size");

  entries = holy_malloc (entries_size);
  if (!entries)
    goto fail;

  sector = holy_le_to_cpu64 (header->partitions);
  holy_dprintf ("gpt", "reading GPT %lu entries from sector 0x%llx\n",
		(unsigned long) count,
		(unsigned long long) sector);

  addr = holy_gpt_sector_to_addr (gpt, sector);
  if (holy_disk_read (disk, addr, 0, entries_size, entries))
    goto fail;

  holy_gpt_lecrc32 (&crc, entries, entries_size);
  if (crc != header->partentry_crc32)
    {
      holy_error (holy_ERR_BAD_PART_TABLE, "invalid GPT entry crc32");
      goto fail;
    }

  *ret_entries = entries;
  *ret_entries_size = entries_size;
  return holy_ERR_NONE;

fail:
  holy_free (entries);
  return holy_errno;
}

holy_gpt_t
holy_gpt_read (holy_disk_t disk)
{
  holy_gpt_t gpt;

  holy_dprintf ("gpt", "reading GPT from %s\n", disk->name);

  gpt = holy_zalloc (sizeof (*gpt));
  if (!gpt)
    goto fail;

  if (holy_disk_read (disk, 0, 0, sizeof (gpt->mbr), &gpt->mbr))
    goto fail;

  /* Check the MBR but errors aren't reported beyond the status bit.  */
  if (holy_gpt_pmbr_check (&gpt->mbr))
    holy_errno = holy_ERR_NONE;
  else
    gpt->status |= holy_GPT_PROTECTIVE_MBR;

  /* If both the primary and backup fail report the primary's error.  */
  if (holy_gpt_read_primary (disk, gpt))
    {
      holy_error_push ();
      holy_gpt_read_backup (disk, gpt);
      holy_error_pop ();
    }
  else
    holy_gpt_read_backup (disk, gpt);

  /* If either succeeded clear any possible error from the other.  */
  if (holy_gpt_primary_valid (gpt) || holy_gpt_backup_valid (gpt))
    holy_errno = holy_ERR_NONE;
  else
    goto fail;

  return gpt;

fail:
  holy_gpt_free (gpt);
  return NULL;
}

struct holy_gpt_partentry *
holy_gpt_get_partentry (holy_gpt_t gpt, holy_uint32_t n)
{
  struct holy_gpt_header *header;
  holy_size_t offset;

  header = holy_gpt_get_header (gpt);
  if (!header)
    return NULL;

  if (n >= holy_le_to_cpu32 (header->maxpart))
    return NULL;

  offset = (holy_size_t) holy_le_to_cpu32 (header->partentry_size) * n;
  return (struct holy_gpt_partentry *) ((char *) gpt->entries + offset);
}

holy_err_t
holy_gpt_repair (holy_disk_t disk, holy_gpt_t gpt)
{
  /* Skip if there is nothing to do.  */
  if (holy_gpt_both_valid (gpt))
    return holy_ERR_NONE;

  holy_dprintf ("gpt", "repairing GPT for %s\n", disk->name);

  if (disk->log_sector_size != gpt->log_sector_size)
    return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		       "GPT sector size must match disk sector size");

  if (holy_gpt_primary_valid (gpt))
    {
      holy_uint64_t backup_header;

      holy_dprintf ("gpt", "primary GPT is valid\n");

      /* Relocate backup to end if disk if the disk has grown.  */
      backup_header = holy_le_to_cpu64 (gpt->primary.alternate_lba);
      if (holy_gpt_disk_size_valid (disk) &&
	  disk->total_sectors - 1 > backup_header)
	{
	  backup_header = disk->total_sectors - 1;
	  holy_dprintf ("gpt", "backup GPT header relocated to 0x%llx\n",
			(unsigned long long) backup_header);

	  gpt->primary.alternate_lba = holy_cpu_to_le64 (backup_header);
	}

      holy_memcpy (&gpt->backup, &gpt->primary, sizeof (gpt->backup));
      gpt->backup.header_lba = gpt->primary.alternate_lba;
      gpt->backup.alternate_lba = gpt->primary.header_lba;
      gpt->backup.partitions = holy_cpu_to_le64 (backup_header -
	  holy_gpt_size_to_sectors (gpt, gpt->entries_size));
    }
  else if (holy_gpt_backup_valid (gpt))
    {
      holy_dprintf ("gpt", "backup GPT is valid\n");

      holy_memcpy (&gpt->primary, &gpt->backup, sizeof (gpt->primary));
      gpt->primary.header_lba = gpt->backup.alternate_lba;
      gpt->primary.alternate_lba = gpt->backup.header_lba;
      gpt->primary.partitions = holy_cpu_to_le64_compile_time (2);
    }
  else
    return holy_error (holy_ERR_BUG, "No valid GPT");

  if (holy_gpt_update (gpt))
    return holy_errno;

  holy_dprintf ("gpt", "repairing GPT for %s successful\n", disk->name);

  return holy_ERR_NONE;
}

holy_err_t
holy_gpt_update (holy_gpt_t gpt)
{
  holy_uint32_t crc;

  /* Clear status bits, require revalidation of everything.  */
  gpt->status &= ~(holy_GPT_PRIMARY_HEADER_VALID |
		   holy_GPT_PRIMARY_ENTRIES_VALID |
		   holy_GPT_BACKUP_HEADER_VALID |
		   holy_GPT_BACKUP_ENTRIES_VALID);

  /* Writing headers larger than our header structure are unsupported.  */
  gpt->primary.headersize =
    holy_cpu_to_le32_compile_time (sizeof (gpt->primary));
  gpt->backup.headersize =
    holy_cpu_to_le32_compile_time (sizeof (gpt->backup));

  holy_gpt_lecrc32 (&crc, gpt->entries, gpt->entries_size);
  gpt->primary.partentry_crc32 = crc;
  gpt->backup.partentry_crc32 = crc;

  holy_gpt_header_lecrc32 (&gpt->primary.crc32, &gpt->primary);
  holy_gpt_header_lecrc32 (&gpt->backup.crc32, &gpt->backup);

  if (holy_gpt_check_primary (gpt))
    {
      holy_error_push ();
      return holy_error (holy_ERR_BUG, "Generated invalid GPT primary header");
    }

  gpt->status |= (holy_GPT_PRIMARY_HEADER_VALID |
		  holy_GPT_PRIMARY_ENTRIES_VALID);

  if (holy_gpt_check_backup (gpt))
    {
      holy_error_push ();
      return holy_error (holy_ERR_BUG, "Generated invalid GPT backup header");
    }

  gpt->status |= (holy_GPT_BACKUP_HEADER_VALID |
		  holy_GPT_BACKUP_ENTRIES_VALID);

  return holy_ERR_NONE;
}

static holy_err_t
holy_gpt_write_table (holy_disk_t disk, holy_gpt_t gpt,
		      struct holy_gpt_header *header)
{
  holy_disk_addr_t addr;

  if (holy_le_to_cpu32 (header->headersize) != sizeof (*header))
    return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		       "Header size is %u, must be %u",
		       holy_le_to_cpu32 (header->headersize),
		       sizeof (*header));

  addr = holy_gpt_sector_to_addr (gpt, holy_le_to_cpu64 (header->header_lba));
  if (addr == 0)
    return holy_error (holy_ERR_BUG,
		       "Refusing to write GPT header to address 0x0");
  if (holy_disk_write (disk, addr, 0, sizeof (*header), header))
    return holy_errno;

  addr = holy_gpt_sector_to_addr (gpt, holy_le_to_cpu64 (header->partitions));
  if (addr < 2)
    return holy_error (holy_ERR_BUG,
		       "Refusing to write GPT entries to address 0x%llx",
		       (unsigned long long) addr);
  if (holy_disk_write (disk, addr, 0, gpt->entries_size, gpt->entries))
    return holy_errno;

  return holy_ERR_NONE;
}

holy_err_t
holy_gpt_write (holy_disk_t disk, holy_gpt_t gpt)
{
  holy_uint64_t backup_header;

  /* TODO: update/repair protective MBRs too.  */

  if (!holy_gpt_both_valid (gpt))
    return holy_error (holy_ERR_BAD_PART_TABLE, "Invalid GPT data");

  /* Write the backup GPT first so if writing fails the update is aborted
   * and the primary is left intact.  However if the backup location is
   * inaccessible we have to just skip and hope for the best, the backup
   * will need to be repaired in the OS.  */
  backup_header = holy_le_to_cpu64 (gpt->backup.header_lba);
  if (holy_gpt_disk_size_valid (disk) &&
      backup_header >= disk->total_sectors)
    {
      holy_printf ("warning: backup GPT located at 0x%llx, "
		   "beyond last disk sector at 0x%llx\n",
		   (unsigned long long) backup_header,
		   (unsigned long long) disk->total_sectors - 1);
      holy_printf ("warning: only writing primary GPT, "
	           "the backup GPT must be repaired from the OS\n");
    }
  else
    {
      holy_dprintf ("gpt", "writing backup GPT to %s\n", disk->name);
      if (holy_gpt_write_table (disk, gpt, &gpt->backup))
	return holy_errno;
    }

  holy_dprintf ("gpt", "writing primary GPT to %s\n", disk->name);
  if (holy_gpt_write_table (disk, gpt, &gpt->primary))
    return holy_errno;

  return holy_ERR_NONE;
}

void
holy_gpt_free (holy_gpt_t gpt)
{
  if (!gpt)
    return;

  holy_free (gpt->entries);
  holy_free (gpt);
}
