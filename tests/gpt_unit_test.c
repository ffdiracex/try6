/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/command.h>
#include <holy/device.h>
#include <holy/disk.h>
#include <holy/emu/hostdisk.h>
#include <holy/emu/misc.h>
#include <holy/env.h>
#include <holy/err.h>
#include <holy/gpt_partition.h>
#include <holy/msdos_partition.h>
#include <holy/lib/hexdump.h>
#include <holy/search.h>
#include <holy/test.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

/* from gnulib */
#include <verify.h>

/* Confirm that the GPT structures conform to the sizes in the spec:
 * The header size "must be greater than or equal to 92 and must be less
 * than or equal to the logical block size."
 * The partition entry size must be "a value of 128*(2^n) where n is an
 * integer greater than or equal to zero (e.g., 128, 256, 512, etc.)."  */
verify (sizeof (struct holy_gpt_header) == 92);
verify (sizeof (struct holy_gpt_partentry) == 128);

/* GPT section sizes.  */
#define HEADER_SIZE   (sizeof (struct holy_gpt_header))
#define HEADER_PAD    (holy_DISK_SECTOR_SIZE - HEADER_SIZE)
#define ENTRY_SIZE    (sizeof (struct holy_gpt_partentry))
#define TABLE_ENTRIES 0x80
#define TABLE_SIZE    (TABLE_ENTRIES * ENTRY_SIZE)
#define TABLE_SECTORS (TABLE_SIZE / holy_DISK_SECTOR_SIZE)

/* Double check that the table size calculation was valid.  */
verify (TABLE_SECTORS * holy_DISK_SECTOR_SIZE == TABLE_SIZE);

/* GPT section locations for a 1MiB disk.  */
#define DISK_SECTORS	      0x800
#define DISK_SIZE	      (holy_DISK_SECTOR_SIZE * DISK_SECTORS)
#define PRIMARY_HEADER_SECTOR 0x1
#define PRIMARY_TABLE_SECTOR  0x2
#define BACKUP_HEADER_SECTOR  (DISK_SECTORS - 0x1)
#define BACKUP_TABLE_SECTOR   (BACKUP_HEADER_SECTOR - TABLE_SECTORS)

#define DATA_START_SECTOR     (PRIMARY_TABLE_SECTOR + TABLE_SECTORS)
#define DATA_END_SECTOR	      (BACKUP_TABLE_SECTOR - 0x1)
#define DATA_SECTORS	      (BACKUP_TABLE_SECTOR - DATA_START_SECTOR)
#define DATA_SIZE	      (holy_DISK_SECTOR_SIZE * DATA_SECTORS)

struct test_disk
{
  struct holy_msdos_partition_mbr mbr;

  struct holy_gpt_header primary_header;
  holy_uint8_t primary_header_pad[HEADER_PAD];
  struct holy_gpt_partentry primary_entries[TABLE_ENTRIES];

  holy_uint8_t data[DATA_SIZE];

  struct holy_gpt_partentry backup_entries[TABLE_ENTRIES];
  struct holy_gpt_header backup_header;
  holy_uint8_t backup_header_pad[HEADER_PAD];
} holy_PACKED;

/* Sanity check that all the above ugly math was correct.  */
verify (sizeof (struct test_disk) == DISK_SIZE);

struct test_data
{
  int fd;
  holy_device_t dev;
  struct test_disk *raw;
};


/* Sample primary GPT header for a 1MB disk.  */
static const struct holy_gpt_header example_primary = {
  .magic = holy_GPT_HEADER_MAGIC,
  .version = holy_GPT_HEADER_VERSION,
  .headersize = sizeof (struct holy_gpt_header),
  .crc32 = holy_cpu_to_le32_compile_time (0xb985abe0),
  .header_lba = holy_cpu_to_le64_compile_time (PRIMARY_HEADER_SECTOR),
  .alternate_lba = holy_cpu_to_le64_compile_time (BACKUP_HEADER_SECTOR),
  .start = holy_cpu_to_le64_compile_time (DATA_START_SECTOR),
  .end = holy_cpu_to_le64_compile_time (DATA_END_SECTOR),
  .guid = holy_GPT_GUID_INIT(0x69c131ad, 0x67d6, 0x46c6,
			     0x93, 0xc4, 0x12, 0x4c, 0x75, 0x52, 0x56, 0xac),
  .partitions = holy_cpu_to_le64_compile_time (PRIMARY_TABLE_SECTOR),
  .maxpart = holy_cpu_to_le32_compile_time (TABLE_ENTRIES),
  .partentry_size = holy_cpu_to_le32_compile_time (ENTRY_SIZE),
  .partentry_crc32 = holy_cpu_to_le32_compile_time (0x074e052c),
};

static const struct holy_gpt_partentry example_entries[TABLE_ENTRIES] = {
  {
    .type = holy_GPT_PARTITION_TYPE_EFI_SYSTEM,
    .guid = holy_GPT_GUID_INIT (0xa0f1792e, 0xb4ce, 0x4136, 0xbc, 0xf2,
				0x1a, 0xfc, 0x13, 0x3c, 0x28, 0x28),
    .start = holy_cpu_to_le64_compile_time (DATA_START_SECTOR),
    .end = holy_cpu_to_le64_compile_time (0x3f),
    .attrib = 0x0,
    .name = {
      holy_cpu_to_le16_compile_time ('E'),
      holy_cpu_to_le16_compile_time ('F'),
      holy_cpu_to_le16_compile_time ('I'),
      holy_cpu_to_le16_compile_time (' '),
      holy_cpu_to_le16_compile_time ('S'),
      holy_cpu_to_le16_compile_time ('Y'),
      holy_cpu_to_le16_compile_time ('S'),
      holy_cpu_to_le16_compile_time ('T'),
      holy_cpu_to_le16_compile_time ('E'),
      holy_cpu_to_le16_compile_time ('M'),
      0x0,
    }
  },
  {
    .type = holy_GPT_PARTITION_TYPE_BIOS_BOOT,
    .guid = holy_GPT_GUID_INIT (0x876c898d, 0x1b40, 0x4727, 0xa1, 0x61,
				0xed, 0xf9, 0xb5, 0x48, 0x66, 0x74),
    .start = holy_cpu_to_le64_compile_time (0x40),
    .end = holy_cpu_to_le64_compile_time (0x7f),
    .attrib = holy_cpu_to_le64_compile_time (
	1ULL << holy_GPT_PART_ATTR_OFFSET_LEGACY_BIOS_BOOTABLE),
    .name = {
      holy_cpu_to_le16_compile_time ('B'),
      holy_cpu_to_le16_compile_time ('I'),
      holy_cpu_to_le16_compile_time ('O'),
      holy_cpu_to_le16_compile_time ('S'),
      holy_cpu_to_le16_compile_time (' '),
      holy_cpu_to_le16_compile_time ('B'),
      holy_cpu_to_le16_compile_time ('O'),
      holy_cpu_to_le16_compile_time ('O'),
      holy_cpu_to_le16_compile_time ('T'),
      0x0,
    }
  },
};

/* And the backup header.  */
static const struct holy_gpt_header example_backup = {
  .magic = holy_GPT_HEADER_MAGIC,
  .version = holy_GPT_HEADER_VERSION,
  .headersize = sizeof (struct holy_gpt_header),
  .crc32 = holy_cpu_to_le32_compile_time (0x0af785eb),
  .header_lba = holy_cpu_to_le64_compile_time (BACKUP_HEADER_SECTOR),
  .alternate_lba = holy_cpu_to_le64_compile_time (PRIMARY_HEADER_SECTOR),
  .start = holy_cpu_to_le64_compile_time (DATA_START_SECTOR),
  .end = holy_cpu_to_le64_compile_time (DATA_END_SECTOR),
  .guid = holy_GPT_GUID_INIT(0x69c131ad, 0x67d6, 0x46c6,
			     0x93, 0xc4, 0x12, 0x4c, 0x75, 0x52, 0x56, 0xac),
  .partitions = holy_cpu_to_le64_compile_time (BACKUP_TABLE_SECTOR),
  .maxpart = holy_cpu_to_le32_compile_time (TABLE_ENTRIES),
  .partentry_size = holy_cpu_to_le32_compile_time (ENTRY_SIZE),
  .partentry_crc32 = holy_cpu_to_le32_compile_time (0x074e052c),
};

/* Sample protective MBR for the same 1MB disk. Note, this matches
 * parted and fdisk behavior. The UEFI spec uses different values.  */
static const struct holy_msdos_partition_mbr example_pmbr = {
  .entries = {{.flag = 0x00,
	       .start_head = 0x00,
	       .start_sector = 0x01,
	       .start_cylinder = 0x00,
	       .type = 0xee,
	       .end_head = 0xfe,
	       .end_sector = 0xff,
	       .end_cylinder = 0xff,
	       .start = holy_cpu_to_le32_compile_time (0x1),
	       .length = holy_cpu_to_le32_compile_time (DISK_SECTORS - 0x1),
	       }},
  .signature = holy_cpu_to_le16_compile_time (holy_PC_PARTITION_SIGNATURE),
};

/* If errors are left in holy's error stack things can get confused.  */
static void
assert_error_stack_empty (void)
{
  do
    {
      holy_test_assert (holy_errno == holy_ERR_NONE,
			"error on stack: %s", holy_errmsg);
    }
  while (holy_error_pop ());
}

static holy_err_t
execute_command2 (const char *name, const char *arg1, const char *arg2)
{
  holy_command_t cmd;
  holy_err_t err;
  char *argv[2];

  cmd = holy_command_find (name);
  if (!cmd)
    holy_fatal ("can't find command %s", name);

  argv[0] = strdup (arg1);
  argv[1] = strdup (arg2);
  err = (cmd->func) (cmd, 2, argv);
  free (argv[0]);
  free (argv[1]);

  return err;
}

static void
sync_disk (struct test_data *data)
{
  if (msync (data->raw, DISK_SIZE, MS_SYNC | MS_INVALIDATE) < 0)
    holy_fatal ("Syncing disk failed: %s", strerror (errno));

  holy_disk_cache_invalidate_all ();
}

static void
reset_disk (struct test_data *data)
{
  memset (data->raw, 0, DISK_SIZE);

  /* Initialize image with valid example tables.  */
  memcpy (&data->raw->mbr, &example_pmbr, sizeof (data->raw->mbr));
  memcpy (&data->raw->primary_header, &example_primary,
	  sizeof (data->raw->primary_header));
  memcpy (&data->raw->primary_entries, &example_entries,
	  sizeof (data->raw->primary_entries));
  memcpy (&data->raw->backup_entries, &example_entries,
	  sizeof (data->raw->backup_entries));
  memcpy (&data->raw->backup_header, &example_backup,
	  sizeof (data->raw->backup_header));

  sync_disk (data);
}

static void
open_disk (struct test_data *data)
{
  const char *loop = "loop0";
  char template[] = "/tmp/holy_gpt_test.XXXXXX";
  char host[sizeof ("(host)") + sizeof (template)];

  data->fd = mkstemp (template);
  if (data->fd < 0)
    holy_fatal ("Creating %s failed: %s", template, strerror (errno));

  if (ftruncate (data->fd, DISK_SIZE) < 0)
    {
      int err = errno;
      unlink (template);
      holy_fatal ("Resizing %s failed: %s", template, strerror (err));
    }

  data->raw = mmap (NULL, DISK_SIZE, PROT_READ | PROT_WRITE,
		    MAP_SHARED, data->fd, 0);
  if (data->raw == MAP_FAILED)
    {
      int err = errno;
      unlink (template);
      holy_fatal ("Maping %s failed: %s", template, strerror (err));
    }

  snprintf (host, sizeof (host), "(host)%s", template);
  if (execute_command2 ("loopback", loop, host) != holy_ERR_NONE)
    {
      unlink (template);
      holy_fatal ("loopback %s %s failed: %s", loop, host, holy_errmsg);
    }

  if (unlink (template) < 0)
    holy_fatal ("Unlinking %s failed: %s", template, strerror (errno));

  reset_disk (data);

  data->dev = holy_device_open (loop);
  if (!data->dev)
    holy_fatal ("Opening %s failed: %s", loop, holy_errmsg);
}

static void
close_disk (struct test_data *data)
{
  char *loop;

  assert_error_stack_empty ();

  if (munmap (data->raw, DISK_SIZE) || close (data->fd))
    holy_fatal ("Closing disk image failed: %s", strerror (errno));

  loop = strdup (data->dev->disk->name);
  holy_test_assert (holy_device_close (data->dev) == holy_ERR_NONE,
		    "Closing disk device failed: %s", holy_errmsg);

  holy_test_assert (execute_command2 ("loopback", "-d", loop) ==
		    holy_ERR_NONE, "loopback -d %s failed: %s", loop,
		    holy_errmsg);

  free (loop);
}

static holy_gpt_t
read_disk (struct test_data *data)
{
  holy_gpt_t gpt;

  gpt = holy_gpt_read (data->dev->disk);
  if (gpt == NULL)
    holy_fatal ("holy_gpt_read failed: %s", holy_errmsg);

  return gpt;
}

static void
pmbr_test (void)
{
  struct holy_msdos_partition_mbr mbr;

  memset (&mbr, 0, sizeof (mbr));

  /* Empty is invalid.  */
  holy_gpt_pmbr_check (&mbr);
  holy_test_assert (holy_errno == holy_ERR_BAD_PART_TABLE,
		    "unexpected error: %s", holy_errmsg);
  holy_errno = holy_ERR_NONE;

  /* A table without a protective partition is invalid.  */
  mbr.signature = holy_cpu_to_le16_compile_time (holy_PC_PARTITION_SIGNATURE);
  holy_gpt_pmbr_check (&mbr);
  holy_test_assert (holy_errno == holy_ERR_BAD_PART_TABLE,
		    "unexpected error: %s", holy_errmsg);
  holy_errno = holy_ERR_NONE;

  /* A table with a protective type is ok.  */
  memcpy (&mbr, &example_pmbr, sizeof (mbr));
  holy_gpt_pmbr_check (&mbr);
  holy_test_assert (holy_errno == holy_ERR_NONE,
		    "unexpected error: %s", holy_errmsg);
  holy_errno = holy_ERR_NONE;
}

static void
header_test (void)
{
  struct holy_gpt_header primary, backup;

  /* Example headers should be valid.  */
  memcpy (&primary, &example_primary, sizeof (primary));
  holy_gpt_header_check (&primary, holy_DISK_SECTOR_BITS);
  holy_test_assert (holy_errno == holy_ERR_NONE,
		    "unexpected error: %s", holy_errmsg);
  holy_errno = holy_ERR_NONE;

  memcpy (&backup, &example_backup, sizeof (backup));
  holy_gpt_header_check (&backup, holy_DISK_SECTOR_BITS);
  holy_test_assert (holy_errno == holy_ERR_NONE,
		    "unexpected error: %s", holy_errmsg);
  holy_errno = holy_ERR_NONE;

  /* Twiddle the GUID to invalidate the CRC. */
  primary.guid.data1 = 0;
  holy_gpt_header_check (&primary, holy_DISK_SECTOR_BITS);
  holy_test_assert (holy_errno == holy_ERR_BAD_PART_TABLE,
		    "unexpected error: %s", holy_errmsg);
  holy_errno = holy_ERR_NONE;

  backup.guid.data1 = 0;
  holy_gpt_header_check (&backup, holy_DISK_SECTOR_BITS);
  holy_test_assert (holy_errno == holy_ERR_BAD_PART_TABLE,
		    "unexpected error: %s", holy_errmsg);
  holy_errno = holy_ERR_NONE;
}

static void
read_valid_test (void)
{
  struct test_data data;
  holy_gpt_t gpt;

  open_disk (&data);
  gpt = read_disk (&data);
  holy_test_assert (gpt->status == (holy_GPT_PROTECTIVE_MBR |
				    holy_GPT_PRIMARY_HEADER_VALID |
				    holy_GPT_PRIMARY_ENTRIES_VALID |
				    holy_GPT_BACKUP_HEADER_VALID |
				    holy_GPT_BACKUP_ENTRIES_VALID),
		    "unexpected status: 0x%02x", gpt->status);
  holy_gpt_free (gpt);
  close_disk (&data);
}

static void
read_invalid_entries_test (void)
{
  struct test_data data;
  holy_gpt_t gpt;

  open_disk (&data);

  /* Corrupt the first entry in both tables.  */
  memset (&data.raw->primary_entries[0], 0x55,
	  sizeof (data.raw->primary_entries[0]));
  memset (&data.raw->backup_entries[0], 0x55,
	  sizeof (data.raw->backup_entries[0]));
  sync_disk (&data);

  gpt = holy_gpt_read (data.dev->disk);
  holy_test_assert (gpt == NULL, "no error reported for corrupt entries");
  holy_test_assert (holy_errno == holy_ERR_BAD_PART_TABLE,
		    "unexpected error: %s", holy_errmsg);
  holy_errno = holy_ERR_NONE;

  close_disk (&data);
}

static void
read_fallback_test (void)
{
  struct test_data data;
  holy_gpt_t gpt;

  open_disk (&data);

  /* Corrupt the primary header.  */
  memset (&data.raw->primary_header.guid, 0x55,
	  sizeof (data.raw->primary_header.guid));
  sync_disk (&data);
  gpt = read_disk (&data);
  holy_test_assert ((gpt->status & holy_GPT_PRIMARY_HEADER_VALID) == 0,
		    "unreported corrupt primary header");
  holy_gpt_free (gpt);
  reset_disk (&data);

  /* Corrupt the backup header.  */
  memset (&data.raw->backup_header.guid, 0x55,
	  sizeof (data.raw->backup_header.guid));
  sync_disk (&data);
  gpt = read_disk (&data);
  holy_test_assert ((gpt->status & holy_GPT_BACKUP_HEADER_VALID) == 0,
		    "unreported corrupt backup header");
  holy_gpt_free (gpt);
  reset_disk (&data);

  /* Corrupt the primary entry table.  */
  memset (&data.raw->primary_entries[0], 0x55,
	  sizeof (data.raw->primary_entries[0]));
  sync_disk (&data);
  gpt = read_disk (&data);
  holy_test_assert ((gpt->status & holy_GPT_PRIMARY_ENTRIES_VALID) == 0,
		    "unreported corrupt primary entries table");
  holy_gpt_free (gpt);
  reset_disk (&data);

  /* Corrupt the backup entry table.  */
  memset (&data.raw->backup_entries[0], 0x55,
	  sizeof (data.raw->backup_entries[0]));
  sync_disk (&data);
  gpt = read_disk (&data);
  holy_test_assert ((gpt->status & holy_GPT_BACKUP_ENTRIES_VALID) == 0,
		    "unreported corrupt backup entries table");
  holy_gpt_free (gpt);
  reset_disk (&data);

  /* If primary is corrupt and disk size is unknown fallback fails.  */
  memset (&data.raw->primary_header.guid, 0x55,
	  sizeof (data.raw->primary_header.guid));
  sync_disk (&data);
  data.dev->disk->total_sectors = holy_DISK_SIZE_UNKNOWN;
  gpt = holy_gpt_read (data.dev->disk);
  holy_test_assert (gpt == NULL, "no error reported");
  holy_test_assert (holy_errno == holy_ERR_BAD_PART_TABLE,
		    "unexpected error: %s", holy_errmsg);
  holy_errno = holy_ERR_NONE;

  close_disk (&data);
}

static void
repair_test (void)
{
  struct test_data data;
  holy_gpt_t gpt;

  open_disk (&data);

  /* Erase/Repair primary.  */
  memset (&data.raw->primary_header, 0, sizeof (data.raw->primary_header));
  sync_disk (&data);
  gpt = read_disk (&data);
  holy_gpt_repair (data.dev->disk, gpt);
  holy_test_assert (holy_errno == holy_ERR_NONE,
		    "repair failed: %s", holy_errmsg);
  if (memcmp (&gpt->primary, &example_primary, sizeof (gpt->primary)))
    {
      printf ("Invalid restored primary header:\n");
      hexdump (16, (char*)&gpt->primary, sizeof (gpt->primary));
      printf ("Expected primary header:\n");
      hexdump (16, (char*)&example_primary, sizeof (example_primary));
      holy_test_assert (0, "repair did not restore primary header");
    }
  holy_gpt_free (gpt);
  reset_disk (&data);

  /* Erase/Repair backup.  */
  memset (&data.raw->backup_header, 0, sizeof (data.raw->backup_header));
  sync_disk (&data);
  gpt = read_disk (&data);
  holy_gpt_repair (data.dev->disk, gpt);
  holy_test_assert (holy_errno == holy_ERR_NONE,
		    "repair failed: %s", holy_errmsg);
  if (memcmp (&gpt->backup, &example_backup, sizeof (gpt->backup)))
    {
      printf ("Invalid restored backup header:\n");
      hexdump (16, (char*)&gpt->backup, sizeof (gpt->backup));
      printf ("Expected backup header:\n");
      hexdump (16, (char*)&example_backup, sizeof (example_backup));
      holy_test_assert (0, "repair did not restore backup header");
    }
  holy_gpt_free (gpt);
  reset_disk (&data);

  close_disk (&data);
}

/* Finding/reading/writing the backup GPT may be difficult if the OS and
 * BIOS report different sizes for the same disk.  We need to gracefully
 * recognize this and avoid causing trouble for the OS.  */
static void
weird_disk_size_test (void)
{
  struct test_data data;
  holy_gpt_t gpt;

  open_disk (&data);

  /* Chop off 65536 bytes (128 512B sectors) which may happen when the
   * BIOS thinks you are using a software RAID system that reserves that
   * area for metadata when in fact you are not and using the bare disk.  */
  holy_test_assert(data.dev->disk->total_sectors == DISK_SECTORS,
		   "unexpected disk size: 0x%llx",
		   (unsigned long long) data.dev->disk->total_sectors);
  data.dev->disk->total_sectors -= 128;

  gpt = read_disk (&data);
  assert_error_stack_empty ();
  /* Reading the alternate_lba should have been blocked and reading
   * the (new) end of disk should have found no useful data.  */
  holy_test_assert ((gpt->status & holy_GPT_BACKUP_HEADER_VALID) == 0,
		    "unreported missing backup header");

  /* We should be able to reconstruct the backup header and the location
   * of the backup should remain unchanged, trusting the GPT data over
   * what the BIOS is telling us.  Further changes are left to the OS.  */
  holy_gpt_repair (data.dev->disk, gpt);
  holy_test_assert (holy_errno == holy_ERR_NONE,
		    "repair failed: %s", holy_errmsg);
  holy_test_assert (memcmp (&gpt->primary, &example_primary,
	                    sizeof (gpt->primary)) == 0,
		    "repair corrupted primary header");

  holy_gpt_free (gpt);
  close_disk (&data);
}

static void
iterate_partitions_test (void)
{
  struct test_data data;
  struct holy_gpt_partentry *p;
  holy_gpt_t gpt;
  holy_uint32_t n;

  open_disk (&data);
  gpt = read_disk (&data);

  for (n = 0; (p = holy_gpt_get_partentry (gpt, n)) != NULL; n++)
    holy_test_assert (memcmp (p, &example_entries[n], sizeof (*p)) == 0,
		      "unexpected partition %d data", n);

  holy_test_assert (n == TABLE_ENTRIES, "unexpected partition limit: %d", n);

  holy_gpt_free (gpt);
  close_disk (&data);
}

static void
large_partitions_test (void)
{
  struct test_data data;
  struct holy_gpt_partentry *p;
  holy_gpt_t gpt;
  holy_uint32_t n;

  open_disk (&data);

  /* Double the entry size, cut the number of entries in half.  */
  data.raw->primary_header.maxpart =
    data.raw->backup_header.maxpart =
    holy_cpu_to_le32_compile_time (TABLE_ENTRIES/2);
  data.raw->primary_header.partentry_size =
    data.raw->backup_header.partentry_size =
    holy_cpu_to_le32_compile_time (ENTRY_SIZE*2);
  data.raw->primary_header.partentry_crc32 =
    data.raw->backup_header.partentry_crc32 =
    holy_cpu_to_le32_compile_time (0xf2c45af8);
  data.raw->primary_header.crc32 = holy_cpu_to_le32_compile_time (0xde00cc8f);
  data.raw->backup_header.crc32 = holy_cpu_to_le32_compile_time (0x6d72e284);

  memset (&data.raw->primary_entries, 0,
	  sizeof (data.raw->primary_entries));
  for (n = 0; n < TABLE_ENTRIES/2; n++)
    memcpy (&data.raw->primary_entries[n*2], &example_entries[n],
	    sizeof (data.raw->primary_entries[0]));
  memcpy (&data.raw->backup_entries, &data.raw->primary_entries,
	  sizeof (data.raw->backup_entries));

  sync_disk(&data);
  gpt = read_disk (&data);

  for (n = 0; (p = holy_gpt_get_partentry (gpt, n)) != NULL; n++)
    holy_test_assert (memcmp (p, &example_entries[n], sizeof (*p)) == 0,
		      "unexpected partition %d data", n);

  holy_test_assert (n == TABLE_ENTRIES/2, "unexpected partition limit: %d", n);

  holy_gpt_free (gpt);

  /* Editing memory beyond the entry structure should still change the crc.  */
  data.raw->primary_entries[1].attrib = 0xff;

  sync_disk(&data);
  gpt = read_disk (&data);
  holy_test_assert (gpt->status == (holy_GPT_PROTECTIVE_MBR |
				    holy_GPT_PRIMARY_HEADER_VALID |
				    holy_GPT_BACKUP_HEADER_VALID |
				    holy_GPT_BACKUP_ENTRIES_VALID),
		    "unexpected status: 0x%02x", gpt->status);
  holy_gpt_free (gpt);

  close_disk (&data);
}

static void
invalid_partsize_test (void)
{
  struct holy_gpt_header header = {
    .magic = holy_GPT_HEADER_MAGIC,
    .version = holy_GPT_HEADER_VERSION,
    .headersize = sizeof (struct holy_gpt_header),
    .crc32 = holy_cpu_to_le32_compile_time (0x1ff2a054),
    .header_lba = holy_cpu_to_le64_compile_time (PRIMARY_HEADER_SECTOR),
    .alternate_lba = holy_cpu_to_le64_compile_time (BACKUP_HEADER_SECTOR),
    .start = holy_cpu_to_le64_compile_time (DATA_START_SECTOR),
    .end = holy_cpu_to_le64_compile_time (DATA_END_SECTOR),
    .guid = holy_GPT_GUID_INIT(0x69c131ad, 0x67d6, 0x46c6,
			       0x93, 0xc4, 0x12, 0x4c, 0x75, 0x52, 0x56, 0xac),
    .partitions = holy_cpu_to_le64_compile_time (PRIMARY_TABLE_SECTOR),
    .maxpart = holy_cpu_to_le32_compile_time (TABLE_ENTRIES),
    /* Triple the entry size, which is not valid.  */
    .partentry_size = holy_cpu_to_le32_compile_time (ENTRY_SIZE*3),
    .partentry_crc32 = holy_cpu_to_le32_compile_time (0x074e052c),
  };

  holy_gpt_header_check(&header, holy_DISK_SECTOR_BITS);
  holy_test_assert (holy_errno == holy_ERR_BAD_PART_TABLE,
		    "unexpected error: %s", holy_errmsg);
  holy_test_assert (strcmp(holy_errmsg, "invalid GPT entry size") == 0,
		    "unexpected error: %s", holy_errmsg);
  holy_errno = holy_ERR_NONE;
}

static void
search_part_label_test (void)
{
  struct test_data data;
  const char *test_result;
  char *expected_result;

  open_disk (&data);

  expected_result = holy_xasprintf ("%s,gpt1", data.dev->disk->name);
  holy_env_unset ("test_result");
  holy_search_part_label ("EFI SYSTEM", "test_result", 0, NULL, 0);
  test_result = holy_env_get ("test_result");
  holy_test_assert (test_result && strcmp (test_result, expected_result) == 0,
		    "wrong device: %s (%s)", test_result, expected_result);
  holy_free (expected_result);

  expected_result = holy_xasprintf ("%s,gpt2", data.dev->disk->name);
  holy_env_unset ("test_result");
  holy_search_part_label ("BIOS BOOT", "test_result", 0, NULL, 0);
  test_result = holy_env_get ("test_result");
  holy_test_assert (test_result && strcmp (test_result, expected_result) == 0,
		    "wrong device: %s (%s)", test_result, expected_result);
  holy_free (expected_result);

  holy_env_unset ("test_result");
  holy_search_part_label ("bogus name", "test_result", 0, NULL, 0);
  test_result = holy_env_get ("test_result");
  holy_test_assert (test_result == NULL,
		    "unexpected device: %s", test_result);
  holy_test_assert (holy_errno == holy_ERR_FILE_NOT_FOUND,
		    "unexpected error: %s", holy_errmsg);
  holy_errno = holy_ERR_NONE;

  close_disk (&data);
}

static void
search_part_uuid_test (void)
{
  struct test_data data;
  const char gpt1_uuid[] = "A0F1792E-B4CE-4136-BCF2-1AFC133C2828";
  const char gpt2_uuid[] = "876c898d-1b40-4727-a161-edf9b5486674";
  const char bogus_uuid[] = "1534c928-c50e-4866-9daf-6a9fd7918a76";
  const char *test_result;
  char *expected_result;

  open_disk (&data);

  expected_result = holy_xasprintf ("%s,gpt1", data.dev->disk->name);
  holy_env_unset ("test_result");
  holy_search_part_uuid (gpt1_uuid, "test_result", 0, NULL, 0);
  test_result = holy_env_get ("test_result");
  holy_test_assert (test_result && strcmp (test_result, expected_result) == 0,
		    "wrong device: %s (%s)", test_result, expected_result);
  holy_free (expected_result);

  expected_result = holy_xasprintf ("%s,gpt2", data.dev->disk->name);
  holy_env_unset ("test_result");
  holy_search_part_uuid (gpt2_uuid, "test_result", 0, NULL, 0);
  test_result = holy_env_get ("test_result");
  holy_test_assert (test_result && strcmp (test_result, expected_result) == 0,
		    "wrong device: %s (%s)", test_result, expected_result);
  holy_free (expected_result);

  holy_env_unset ("test_result");
  holy_search_part_uuid (bogus_uuid, "test_result", 0, NULL, 0);
  test_result = holy_env_get ("test_result");
  holy_test_assert (test_result == NULL,
		    "unexpected device: %s", test_result);
  holy_test_assert (holy_errno == holy_ERR_FILE_NOT_FOUND,
		    "unexpected error: %s", holy_errmsg);
  holy_errno = holy_ERR_NONE;

  close_disk (&data);
}

static void
search_disk_uuid_test (void)
{
  struct test_data data;
  const char disk_uuid[] = "69c131ad-67d6-46c6-93c4-124c755256ac";
  const char bogus_uuid[] = "1534c928-c50e-4866-9daf-6a9fd7918a76";
  const char *test_result;
  char *expected_result;

  open_disk (&data);

  expected_result = holy_xasprintf ("%s", data.dev->disk->name);
  holy_env_unset ("test_result");
  holy_search_disk_uuid (disk_uuid, "test_result", 0, NULL, 0);
  test_result = holy_env_get ("test_result");
  holy_test_assert (test_result && strcmp (test_result, expected_result) == 0,
		    "wrong device: %s (%s)", test_result, expected_result);
  holy_free (expected_result);

  holy_env_unset ("test_result");
  holy_search_disk_uuid (bogus_uuid, "test_result", 0, NULL, 0);
  test_result = holy_env_get ("test_result");
  holy_test_assert (test_result == NULL,
		    "unexpected device: %s", test_result);
  holy_test_assert (holy_errno == holy_ERR_FILE_NOT_FOUND,
		    "unexpected error: %s", holy_errmsg);
  holy_errno = holy_ERR_NONE;

  close_disk (&data);
}

void
holy_unit_test_init (void)
{
  holy_init_all ();
  holy_hostfs_init ();
  holy_host_init ();
  holy_test_register ("gpt_pmbr_test", pmbr_test);
  holy_test_register ("gpt_header_test", header_test);
  holy_test_register ("gpt_read_valid_test", read_valid_test);
  holy_test_register ("gpt_read_invalid_test", read_invalid_entries_test);
  holy_test_register ("gpt_read_fallback_test", read_fallback_test);
  holy_test_register ("gpt_repair_test", repair_test);
  holy_test_register ("gpt_iterate_partitions_test", iterate_partitions_test);
  holy_test_register ("gpt_large_partitions_test", large_partitions_test);
  holy_test_register ("gpt_invalid_partsize_test", invalid_partsize_test);
  holy_test_register ("gpt_weird_disk_size_test", weird_disk_size_test);
  holy_test_register ("gpt_search_part_label_test", search_part_label_test);
  holy_test_register ("gpt_search_uuid_test", search_part_uuid_test);
  holy_test_register ("gpt_search_disk_uuid_test", search_disk_uuid_test);
}

void
holy_unit_test_fini (void)
{
  holy_test_unregister ("gpt_pmbr_test");
  holy_test_unregister ("gpt_header_test");
  holy_test_unregister ("gpt_read_valid_test");
  holy_test_unregister ("gpt_read_invalid_test");
  holy_test_unregister ("gpt_read_fallback_test");
  holy_test_unregister ("gpt_repair_test");
  holy_test_unregister ("gpt_iterate_partitions_test");
  holy_test_unregister ("gpt_large_partitions_test");
  holy_test_unregister ("gpt_invalid_partsize_test");
  holy_test_unregister ("gpt_weird_disk_size_test");
  holy_test_unregister ("gpt_search_part_label_test");
  holy_test_unregister ("gpt_search_part_uuid_test");
  holy_test_unregister ("gpt_search_disk_uuid_test");
  holy_fini_all ();
}
