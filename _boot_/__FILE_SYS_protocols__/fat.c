/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/fs.h>
#include <holy/disk.h>
#include <holy/file.h>
#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/charset.h>
#ifndef MODE_EXFAT
#include <holy/fat.h>
#else
#include <holy/exfat.h>
#endif
#include <holy/fshelp.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

enum
  {
    holy_FAT_ATTR_READ_ONLY = 0x01,
    holy_FAT_ATTR_HIDDEN = 0x02,
    holy_FAT_ATTR_SYSTEM = 0x04,
#ifndef MODE_EXFAT
    holy_FAT_ATTR_VOLUME_ID = 0x08,
#endif
    holy_FAT_ATTR_DIRECTORY = 0x10,
    holy_FAT_ATTR_ARCHIVE = 0x20,

#ifndef MODE_EXFAT
    holy_FAT_ATTR_LONG_NAME = (holy_FAT_ATTR_READ_ONLY
			       | holy_FAT_ATTR_HIDDEN
			       | holy_FAT_ATTR_SYSTEM
			       | holy_FAT_ATTR_VOLUME_ID),
#endif
    holy_FAT_ATTR_VALID = (holy_FAT_ATTR_READ_ONLY
			   | holy_FAT_ATTR_HIDDEN
			   | holy_FAT_ATTR_SYSTEM
			   | holy_FAT_ATTR_DIRECTORY
			   | holy_FAT_ATTR_ARCHIVE
#ifndef MODE_EXFAT
			   | holy_FAT_ATTR_VOLUME_ID
#endif
			   )
  };

#ifdef MODE_EXFAT
typedef struct holy_exfat_bpb holy_current_fat_bpb_t;
#else
typedef struct holy_fat_bpb holy_current_fat_bpb_t;
#endif

#ifdef MODE_EXFAT
enum
  {
    FLAG_CONTIGUOUS = 2
  };
struct holy_fat_dir_entry
{
  holy_uint8_t entry_type;
  union
  {
    holy_uint8_t placeholder[31];
    struct {
      holy_uint8_t secondary_count;
      holy_uint16_t checksum;
      holy_uint16_t attr;
      holy_uint16_t reserved1;
      holy_uint32_t c_time;
      holy_uint32_t m_time;
      holy_uint32_t a_time;
      holy_uint8_t c_time_tenth;
      holy_uint8_t m_time_tenth;
      holy_uint8_t a_time_tenth;
      holy_uint8_t reserved2[9];
    }  holy_PACKED file;
    struct {
      holy_uint8_t flags;
      holy_uint8_t reserved1;
      holy_uint8_t name_length;
      holy_uint16_t name_hash;
      holy_uint16_t reserved2;
      holy_uint64_t valid_size;
      holy_uint32_t reserved3;
      holy_uint32_t first_cluster;
      holy_uint64_t file_size;
    }   holy_PACKED stream_extension;
    struct {
      holy_uint8_t flags;
      holy_uint16_t str[15];
    }  holy_PACKED  file_name;
    struct {
      holy_uint8_t character_count;
      holy_uint16_t str[15];
    }  holy_PACKED  volume_label;
  }  holy_PACKED type_specific;
} holy_PACKED;

struct holy_fat_dir_node
{
  holy_uint32_t attr;
  holy_uint32_t first_cluster;
  holy_uint64_t file_size;
  holy_uint64_t valid_size;
  int have_stream;
  int is_contiguous;
};

typedef struct holy_fat_dir_node holy_fat_dir_node_t;

#else
struct holy_fat_dir_entry
{
  holy_uint8_t name[11];
  holy_uint8_t attr;
  holy_uint8_t nt_reserved;
  holy_uint8_t c_time_tenth;
  holy_uint16_t c_time;
  holy_uint16_t c_date;
  holy_uint16_t a_date;
  holy_uint16_t first_cluster_high;
  holy_uint16_t w_time;
  holy_uint16_t w_date;
  holy_uint16_t first_cluster_low;
  holy_uint32_t file_size;
} holy_PACKED;

struct holy_fat_long_name_entry
{
  holy_uint8_t id;
  holy_uint16_t name1[5];
  holy_uint8_t attr;
  holy_uint8_t reserved;
  holy_uint8_t checksum;
  holy_uint16_t name2[6];
  holy_uint16_t first_cluster;
  holy_uint16_t name3[2];
} holy_PACKED;

typedef struct holy_fat_dir_entry holy_fat_dir_node_t;

#endif

struct holy_fat_data
{
  int logical_sector_bits;
  holy_uint32_t num_sectors;

  holy_uint32_t fat_sector;
  holy_uint32_t sectors_per_fat;
  int fat_size;

  holy_uint32_t root_cluster;
#ifndef MODE_EXFAT
  holy_uint32_t root_sector;
  holy_uint32_t num_root_sectors;
#endif

  int cluster_bits;
  holy_uint32_t cluster_eof_mark;
  holy_uint32_t cluster_sector;
  holy_uint32_t num_clusters;

  holy_uint32_t uuid;
};

struct holy_fshelp_node {
  holy_disk_t disk;
  struct holy_fat_data *data;

  holy_uint8_t attr;
#ifndef MODE_EXFAT
  holy_uint32_t file_size;
#else
  holy_uint64_t file_size;
#endif
  holy_uint32_t file_cluster;
  holy_uint32_t cur_cluster_num;
  holy_uint32_t cur_cluster;

#ifdef MODE_EXFAT
  int is_contiguous;
#endif
};

static holy_dl_t my_mod;

#ifndef MODE_EXFAT
static int
fat_log2 (unsigned x)
{
  int i;

  if (x == 0)
    return -1;

  for (i = 0; (x & 1) == 0; i++)
    x >>= 1;

  if (x != 1)
    return -1;

  return i;
}
#endif

static struct holy_fat_data *
holy_fat_mount (holy_disk_t disk)
{
  holy_current_fat_bpb_t bpb;
  struct holy_fat_data *data = 0;
  holy_uint32_t first_fat, magic;

  if (! disk)
    goto fail;

  data = (struct holy_fat_data *) holy_malloc (sizeof (*data));
  if (! data)
    goto fail;

  /* Read the BPB.  */
  if (holy_disk_read (disk, 0, 0, sizeof (bpb), &bpb))
    goto fail;

#ifdef MODE_EXFAT
  if (holy_memcmp ((const char *) bpb.oem_name, "EXFAT   ",
		   sizeof (bpb.oem_name)) != 0)
    goto fail;    
#endif

  /* Get the sizes of logical sectors and clusters.  */
#ifdef MODE_EXFAT
  data->logical_sector_bits = bpb.bytes_per_sector_shift;
#else
  data->logical_sector_bits =
    fat_log2 (holy_le_to_cpu16 (bpb.bytes_per_sector));
#endif
  if (data->logical_sector_bits < holy_DISK_SECTOR_BITS
      || data->logical_sector_bits >= 16)
    goto fail;
  data->logical_sector_bits -= holy_DISK_SECTOR_BITS;

#ifdef MODE_EXFAT
  data->cluster_bits = bpb.sectors_per_cluster_shift;
#else
  data->cluster_bits = fat_log2 (bpb.sectors_per_cluster);
#endif
  if (data->cluster_bits < 0 || data->cluster_bits > 25)
    goto fail;
  data->cluster_bits += data->logical_sector_bits;

  /* Get information about FATs.  */
#ifdef MODE_EXFAT
  data->fat_sector = (holy_le_to_cpu32 (bpb.num_reserved_sectors)
		      << data->logical_sector_bits);
#else
  data->fat_sector = (holy_le_to_cpu16 (bpb.num_reserved_sectors)
		      << data->logical_sector_bits);
#endif
  if (data->fat_sector == 0)
    goto fail;

#ifdef MODE_EXFAT
  data->sectors_per_fat = (holy_le_to_cpu32 (bpb.sectors_per_fat)
			   << data->logical_sector_bits);
#else
  data->sectors_per_fat = ((bpb.sectors_per_fat_16
			    ? holy_le_to_cpu16 (bpb.sectors_per_fat_16)
			    : holy_le_to_cpu32 (bpb.version_specific.fat32.sectors_per_fat_32))
			   << data->logical_sector_bits);
#endif
  if (data->sectors_per_fat == 0)
    goto fail;

  /* Get the number of sectors in this volume.  */
#ifdef MODE_EXFAT
  data->num_sectors = ((holy_le_to_cpu64 (bpb.num_total_sectors))
		       << data->logical_sector_bits);
#else
  data->num_sectors = ((bpb.num_total_sectors_16
			? holy_le_to_cpu16 (bpb.num_total_sectors_16)
			: holy_le_to_cpu32 (bpb.num_total_sectors_32))
		       << data->logical_sector_bits);
#endif
  if (data->num_sectors == 0)
    goto fail;

  /* Get information about the root directory.  */
  if (bpb.num_fats == 0)
    goto fail;

#ifndef MODE_EXFAT
  data->root_sector = data->fat_sector + bpb.num_fats * data->sectors_per_fat;
  data->num_root_sectors
    = ((((holy_uint32_t) holy_le_to_cpu16 (bpb.num_root_entries)
	 * sizeof (struct holy_fat_dir_entry)
	 + holy_le_to_cpu16 (bpb.bytes_per_sector) - 1)
	>> (data->logical_sector_bits + holy_DISK_SECTOR_BITS))
       << (data->logical_sector_bits));
#endif

#ifdef MODE_EXFAT
  data->cluster_sector = (holy_le_to_cpu32 (bpb.cluster_offset)
			  << data->logical_sector_bits);
  data->num_clusters = (holy_le_to_cpu32 (bpb.cluster_count)
			  << data->logical_sector_bits);
#else
  data->cluster_sector = data->root_sector + data->num_root_sectors;
  data->num_clusters = (((data->num_sectors - data->cluster_sector)
			 >> data->cluster_bits)
			+ 2);
#endif

  if (data->num_clusters <= 2)
    goto fail;

#ifdef MODE_EXFAT
  {
    /* exFAT.  */
    data->root_cluster = holy_le_to_cpu32 (bpb.root_cluster);
    data->fat_size = 32;
    data->cluster_eof_mark = 0xffffffff;

    if ((bpb.volume_flags & holy_cpu_to_le16_compile_time (0x1))
	&& bpb.num_fats > 1)
      data->fat_sector += data->sectors_per_fat;
  }
#else
  if (! bpb.sectors_per_fat_16)
    {
      /* FAT32.  */
      holy_uint16_t flags = holy_le_to_cpu16 (bpb.version_specific.fat32.extended_flags);

      data->root_cluster = holy_le_to_cpu32 (bpb.version_specific.fat32.root_cluster);
      data->fat_size = 32;
      data->cluster_eof_mark = 0x0ffffff8;

      if (flags & 0x80)
	{
	  /* Get an active FAT.  */
	  unsigned active_fat = flags & 0xf;

	  if (active_fat > bpb.num_fats)
	    goto fail;

	  data->fat_sector += active_fat * data->sectors_per_fat;
	}

      if (bpb.num_root_entries != 0 || bpb.version_specific.fat32.fs_version != 0)
	goto fail;
    }
  else
    {
      /* FAT12 or FAT16.  */
      data->root_cluster = ~0U;

      if (data->num_clusters <= 4085 + 2)
	{
	  /* FAT12.  */
	  data->fat_size = 12;
	  data->cluster_eof_mark = 0x0ff8;
	}
      else
	{
	  /* FAT16.  */
	  data->fat_size = 16;
	  data->cluster_eof_mark = 0xfff8;
	}
    }
#endif

  /* More sanity checks.  */
  if (data->num_sectors <= data->fat_sector)
    goto fail;

  if (holy_disk_read (disk,
		      data->fat_sector,
		      0,
		      sizeof (first_fat),
		      &first_fat))
    goto fail;

  first_fat = holy_le_to_cpu32 (first_fat);

  if (data->fat_size == 32)
    {
      first_fat &= 0x0fffffff;
      magic = 0x0fffff00;
    }
  else if (data->fat_size == 16)
    {
      first_fat &= 0x0000ffff;
      magic = 0xff00;
    }
  else
    {
      first_fat &= 0x00000fff;
      magic = 0x0f00;
    }

  /* Serial number.  */
#ifdef MODE_EXFAT
    data->uuid = holy_le_to_cpu32 (bpb.num_serial);
#else
  if (bpb.sectors_per_fat_16)
    data->uuid = holy_le_to_cpu32 (bpb.version_specific.fat12_or_fat16.num_serial);
  else
    data->uuid = holy_le_to_cpu32 (bpb.version_specific.fat32.num_serial);
#endif

#ifndef MODE_EXFAT
  /* Ignore the 3rd bit, because some BIOSes assigns 0xF0 to the media
     descriptor, even if it is a so-called superfloppy (e.g. an USB key).
     The check may be too strict for this kind of stupid BIOSes, as
     they overwrite the media descriptor.  */
  if ((first_fat | 0x8) != (magic | bpb.media | 0x8))
    goto fail;
#else
  (void) magic;
#endif

  return data;

 fail:

  holy_free (data);
  holy_error (holy_ERR_BAD_FS, "not a FAT filesystem");
  return 0;
}

static holy_ssize_t
holy_fat_read_data (holy_disk_t disk, holy_fshelp_node_t node,
		    holy_disk_read_hook_t read_hook, void *read_hook_data,
		    holy_off_t offset, holy_size_t len, char *buf)
{
  holy_size_t size;
  holy_uint32_t logical_cluster;
  unsigned logical_cluster_bits;
  holy_ssize_t ret = 0;
  unsigned long sector;

#ifndef MODE_EXFAT
  /* This is a special case. FAT12 and FAT16 doesn't have the root directory
     in clusters.  */
  if (node->file_cluster == ~0U)
    {
      size = (node->data->num_root_sectors << holy_DISK_SECTOR_BITS) - offset;
      if (size > len)
	size = len;

      if (holy_disk_read (disk, node->data->root_sector, offset, size, buf))
	return -1;

      return size;
    }
#endif

#ifdef MODE_EXFAT
  if (node->is_contiguous)
    {
      /* Read the data here.  */
      sector = (node->data->cluster_sector
		+ ((node->file_cluster - 2)
		   << node->data->cluster_bits));

      disk->read_hook = read_hook;
      disk->read_hook_data = read_hook_data;
      holy_disk_read (disk, sector + (offset >> holy_DISK_SECTOR_BITS),
		      offset & (holy_DISK_SECTOR_SIZE - 1), len, buf);
      disk->read_hook = 0;
      if (holy_errno)
	return -1;

      return len;
    }
#endif

  /* Calculate the logical cluster number and offset.  */
  logical_cluster_bits = (node->data->cluster_bits
			  + holy_DISK_SECTOR_BITS);
  logical_cluster = offset >> logical_cluster_bits;
  offset &= (1ULL << logical_cluster_bits) - 1;

  if (logical_cluster < node->cur_cluster_num)
    {
      node->cur_cluster_num = 0;
      node->cur_cluster = node->file_cluster;
    }

  while (len)
    {
      while (logical_cluster > node->cur_cluster_num)
	{
	  /* Find next cluster.  */
	  holy_uint32_t next_cluster;
	  holy_uint32_t fat_offset;

	  switch (node->data->fat_size)
	    {
	    case 32:
	      fat_offset = node->cur_cluster << 2;
	      break;
	    case 16:
	      fat_offset = node->cur_cluster << 1;
	      break;
	    default:
	      /* case 12: */
	      fat_offset = node->cur_cluster + (node->cur_cluster >> 1);
	      break;
	    }

	  /* Read the FAT.  */
	  if (holy_disk_read (disk, node->data->fat_sector, fat_offset,
			      (node->data->fat_size + 7) >> 3,
			      (char *) &next_cluster))
	    return -1;

	  next_cluster = holy_le_to_cpu32 (next_cluster);
	  switch (node->data->fat_size)
	    {
	    case 16:
	      next_cluster &= 0xFFFF;
	      break;
	    case 12:
	      if (node->cur_cluster & 1)
		next_cluster >>= 4;

	      next_cluster &= 0x0FFF;
	      break;
	    }

	  holy_dprintf ("fat", "fat_size=%d, next_cluster=%u\n",
			node->data->fat_size, next_cluster);

	  /* Check the end.  */
	  if (next_cluster >= node->data->cluster_eof_mark)
	    return ret;

	  if (next_cluster < 2 || next_cluster >= node->data->num_clusters)
	    {
	      holy_error (holy_ERR_BAD_FS, "invalid cluster %u",
			  next_cluster);
	      return -1;
	    }

	  node->cur_cluster = next_cluster;
	  node->cur_cluster_num++;
	}

      /* Read the data here.  */
      sector = (node->data->cluster_sector
		+ ((node->cur_cluster - 2)
		   << node->data->cluster_bits));
      size = (1 << logical_cluster_bits) - offset;
      if (size > len)
	size = len;

      disk->read_hook = read_hook;
      disk->read_hook_data = read_hook_data;
      holy_disk_read (disk, sector, offset, size, buf);
      disk->read_hook = 0;
      if (holy_errno)
	return -1;

      len -= size;
      buf += size;
      ret += size;
      logical_cluster++;
      offset = 0;
    }

  return ret;
}

struct holy_fat_iterate_context
{
#ifdef MODE_EXFAT
  struct holy_fat_dir_node dir;
#else
  struct holy_fat_dir_entry dir;
#endif
  char *filename;
  holy_uint16_t *unibuf;
  holy_ssize_t offset;
};

static holy_err_t
holy_fat_iterate_init (struct holy_fat_iterate_context *ctxt)
{
  ctxt->offset = -sizeof (struct holy_fat_dir_entry);

#ifndef MODE_EXFAT
  /* Allocate space enough to hold a long name.  */
  ctxt->filename = holy_malloc (0x40 * 13 * holy_MAX_UTF8_PER_UTF16 + 1);
  ctxt->unibuf = (holy_uint16_t *) holy_malloc (0x40 * 13 * 2);
#else
  ctxt->unibuf = holy_malloc (15 * 256 * 2);
  ctxt->filename = holy_malloc (15 * 256 * holy_MAX_UTF8_PER_UTF16 + 1);
#endif

  if (! ctxt->filename || ! ctxt->unibuf)
    {
      holy_free (ctxt->filename);
      holy_free (ctxt->unibuf);
      return holy_errno;
    }
  return holy_ERR_NONE;
}

static void
holy_fat_iterate_fini (struct holy_fat_iterate_context *ctxt)
{
  holy_free (ctxt->filename);
  holy_free (ctxt->unibuf);
}

#ifdef MODE_EXFAT
static holy_err_t
holy_fat_iterate_dir_next (holy_fshelp_node_t node,
			   struct holy_fat_iterate_context *ctxt)
{
  holy_memset (&ctxt->dir, 0, sizeof (ctxt->dir));
  while (1)
    {
      struct holy_fat_dir_entry dir;

      ctxt->offset += sizeof (dir);

      if (holy_fat_read_data (node->disk, node, 0, 0, ctxt->offset, sizeof (dir),
			      (char *) &dir)
	   != sizeof (dir))
	break;

      if (dir.entry_type == 0)
	break;
      if (!(dir.entry_type & 0x80))
	continue;

      if (dir.entry_type == 0x85)
	{
	  unsigned i, nsec, slots = 0;

	  nsec = dir.type_specific.file.secondary_count;

	  ctxt->dir.attr = holy_cpu_to_le16 (dir.type_specific.file.attr);
	  ctxt->dir.have_stream = 0;
	  for (i = 0; i < nsec; i++)
	    {
	      struct holy_fat_dir_entry sec;
	      ctxt->offset += sizeof (sec);
	      if (holy_fat_read_data (node->disk, node, 0, 0,
				      ctxt->offset, sizeof (sec), (char *) &sec)
		  != sizeof (sec))
		break;
	      if (!(sec.entry_type & 0x80))
		continue;
	      if (!(sec.entry_type & 0x40))
		break;
	      switch (sec.entry_type)
		{
		case 0xc0:
		  ctxt->dir.first_cluster = holy_cpu_to_le32 (sec.type_specific.stream_extension.first_cluster);
		  ctxt->dir.valid_size
		    = holy_cpu_to_le64 (sec.type_specific.stream_extension.valid_size);
		  ctxt->dir.file_size
		    = holy_cpu_to_le64 (sec.type_specific.stream_extension.file_size);
		  ctxt->dir.have_stream = 1;
		  ctxt->dir.is_contiguous = !!(sec.type_specific.stream_extension.flags
					       & holy_cpu_to_le16_compile_time (FLAG_CONTIGUOUS));
		  break;
		case 0xc1:
		  {
		    int j;
		    for (j = 0; j < 15; j++)
		      ctxt->unibuf[slots * 15 + j] 
			= holy_le_to_cpu16 (sec.type_specific.file_name.str[j]);
		    slots++;
		  }
		  break;
		default:
		  holy_dprintf ("exfat", "unknown secondary type 0x%02x\n",
				sec.entry_type);
		}
	    }

	  if (i != nsec)
	    {
	      ctxt->offset -= sizeof (dir);
	      continue;
	    }

	  *holy_utf16_to_utf8 ((holy_uint8_t *) ctxt->filename, ctxt->unibuf,
			       slots * 15) = '\0';

	  return 0;
	}
      /* Allocation bitmap. */
      if (dir.entry_type == 0x81)
	continue;
      /* Upcase table. */
      if (dir.entry_type == 0x82)
	continue;
      /* Volume label. */
      if (dir.entry_type == 0x83)
	continue;
      holy_dprintf ("exfat", "unknown primary type 0x%02x\n",
		    dir.entry_type);
    }
  return holy_errno ? : holy_ERR_EOF;
}

#else

static holy_err_t
holy_fat_iterate_dir_next (holy_fshelp_node_t node,
			   struct holy_fat_iterate_context *ctxt)
{
  char *filep = 0;
  int checksum = -1;
  int slot = -1, slots = -1;

  while (1)
    {
      unsigned i;

      /* Adjust the offset.  */
      ctxt->offset += sizeof (ctxt->dir);

      /* Read a directory entry.  */
      if (holy_fat_read_data (node->disk, node, 0, 0,
			      ctxt->offset, sizeof (ctxt->dir),
			      (char *) &ctxt->dir)
	   != sizeof (ctxt->dir) || ctxt->dir.name[0] == 0)
	break;

      /* Handle long name entries.  */
      if (ctxt->dir.attr == holy_FAT_ATTR_LONG_NAME)
	{
	  struct holy_fat_long_name_entry *long_name
	    = (struct holy_fat_long_name_entry *) &ctxt->dir;
	  holy_uint8_t id = long_name->id;

	  if (id & 0x40)
	    {
	      id &= 0x3f;
	      slots = slot = id;
	      checksum = long_name->checksum;
	    }

	  if (id != slot || slot == 0 || checksum != long_name->checksum)
	    {
	      checksum = -1;
	      continue;
	    }

	  slot--;
	  holy_memcpy (ctxt->unibuf + slot * 13, long_name->name1, 5 * 2);
	  holy_memcpy (ctxt->unibuf + slot * 13 + 5, long_name->name2, 6 * 2);
	  holy_memcpy (ctxt->unibuf + slot * 13 + 11, long_name->name3, 2 * 2);
	  continue;
	}

      /* Check if this entry is valid.  */
      if (ctxt->dir.name[0] == 0xe5 || (ctxt->dir.attr & ~holy_FAT_ATTR_VALID))
	continue;

      /* This is a workaround for Japanese.  */
      if (ctxt->dir.name[0] == 0x05)
	ctxt->dir.name[0] = 0xe5;

      if (checksum != -1 && slot == 0)
	{
	  holy_uint8_t sum;

	  for (sum = 0, i = 0; i < sizeof (ctxt->dir.name); i++)
	    sum = ((sum >> 1) | (sum << 7)) + ctxt->dir.name[i];

	  if (sum == checksum)
	    {
	      int u;

	      for (u = 0; u < slots * 13; u++)
		ctxt->unibuf[u] = holy_le_to_cpu16 (ctxt->unibuf[u]);

	      *holy_utf16_to_utf8 ((holy_uint8_t *) ctxt->filename,
				   ctxt->unibuf,
				   slots * 13) = '\0';

	      return holy_ERR_NONE;
	    }

	  checksum = -1;
	}

      /* Convert the 8.3 file name.  */
      filep = ctxt->filename;
      if (ctxt->dir.attr & holy_FAT_ATTR_VOLUME_ID)
	{
	  for (i = 0; i < sizeof (ctxt->dir.name) && ctxt->dir.name[i]; i++)
	    *filep++ = ctxt->dir.name[i];
	  while (i > 0 && ctxt->dir.name[i - 1] == ' ')
	    {
	      filep--;
	      i--;
	    }
	}
      else
	{
	  for (i = 0; i < 8 && ctxt->dir.name[i]; i++)
	    *filep++ = holy_tolower (ctxt->dir.name[i]);
	  while (i > 0 && ctxt->dir.name[i - 1] == ' ')
	    {
	      filep--;
	      i--;
	    }

	  /* XXX should we check that dir position is 0 or 1? */
	  if (i > 2 || filep[0] != '.' || (i == 2 && filep[1] != '.'))
	    *filep++ = '.';

	  for (i = 8; i < 11 && ctxt->dir.name[i]; i++)
	    *filep++ = holy_tolower (ctxt->dir.name[i]);
	  while (i > 8 && ctxt->dir.name[i - 1] == ' ')
	    {
	      filep--;
	      i--;
	    }

	  if (i == 8)
	    filep--;
	}
      *filep = '\0';
      return holy_ERR_NONE;
    }

  return holy_errno ? : holy_ERR_EOF;
}

#endif

static holy_err_t lookup_file (holy_fshelp_node_t node,
			       const char *name,
			       holy_fshelp_node_t *foundnode,
			       enum holy_fshelp_filetype *foundtype)
{
  holy_err_t err;
  struct holy_fat_iterate_context ctxt;

  err = holy_fat_iterate_init (&ctxt);
  if (err)
    return err;

  while (!(err = holy_fat_iterate_dir_next (node, &ctxt)))
    {

#ifdef MODE_EXFAT
      if (!ctxt.dir.have_stream)
	continue;
#else
      if (ctxt.dir.attr & holy_FAT_ATTR_VOLUME_ID)
	continue;
#endif

      if (holy_strcasecmp (name, ctxt.filename) == 0)
	{
	  *foundnode = holy_malloc (sizeof (struct holy_fshelp_node));
	  if (!*foundnode)
	    return holy_errno;
	  (*foundnode)->attr = ctxt.dir.attr;
#ifdef MODE_EXFAT
	  (*foundnode)->file_size = ctxt.dir.file_size;
	  (*foundnode)->file_cluster = ctxt.dir.first_cluster;
	  (*foundnode)->is_contiguous = ctxt.dir.is_contiguous;
#else
	  (*foundnode)->file_size = holy_le_to_cpu32 (ctxt.dir.file_size);
	  (*foundnode)->file_cluster = ((holy_le_to_cpu16 (ctxt.dir.first_cluster_high) << 16)
				| holy_le_to_cpu16 (ctxt.dir.first_cluster_low));
	  /* If directory points to root, starting cluster is 0 */
	  if (!(*foundnode)->file_cluster)
	    (*foundnode)->file_cluster = node->data->root_cluster;
#endif
	  (*foundnode)->cur_cluster_num = ~0U;
	  (*foundnode)->data = node->data;
	  (*foundnode)->disk = node->disk;

	  *foundtype = ((*foundnode)->attr & holy_FAT_ATTR_DIRECTORY) ? holy_FSHELP_DIR : holy_FSHELP_REG;

	  holy_fat_iterate_fini (&ctxt);
	  return holy_ERR_NONE;
	}
    }

  holy_fat_iterate_fini (&ctxt);
  if (err == holy_ERR_EOF)
    err = 0;

  return err;

}

static holy_err_t
holy_fat_dir (holy_device_t device, const char *path, holy_fs_dir_hook_t hook,
	      void *hook_data)
{
  struct holy_fat_data *data = 0;
  holy_disk_t disk = device->disk;
  holy_fshelp_node_t found = NULL;
  holy_err_t err;
  struct holy_fat_iterate_context ctxt;

  holy_dl_ref (my_mod);

  data = holy_fat_mount (disk);
  if (! data)
    goto fail;

  struct holy_fshelp_node root = {
    .data = data,
    .disk = disk,
    .attr = holy_FAT_ATTR_DIRECTORY,
    .file_size = 0,
    .file_cluster = data->root_cluster,
    .cur_cluster_num = ~0U,
    .cur_cluster = 0,
#ifdef MODE_EXFAT
    .is_contiguous = 0,
#endif
  };

  err = holy_fshelp_find_file_lookup (path, &root, &found, lookup_file, NULL, holy_FSHELP_DIR);
  if (err)
    goto fail;

  err = holy_fat_iterate_init (&ctxt);
  if (err)
    goto fail;

  while (!(err = holy_fat_iterate_dir_next (found, &ctxt)))
    {
      struct holy_dirhook_info info;
      holy_memset (&info, 0, sizeof (info));

      info.dir = !! (ctxt.dir.attr & holy_FAT_ATTR_DIRECTORY);
      info.case_insensitive = 1;
#ifdef MODE_EXFAT
      if (!ctxt.dir.have_stream)
	continue;
#else
      if (ctxt.dir.attr & holy_FAT_ATTR_VOLUME_ID)
	continue;
#endif

      if (hook (ctxt.filename, &info, hook_data))
	break;
    }
  holy_fat_iterate_fini (&ctxt);
  if (err == holy_ERR_EOF)
    err = 0;

 fail:
  if (found != &root)
    holy_free (found);

  holy_free (data);

  holy_dl_unref (my_mod);

  return holy_errno;
}

static holy_err_t
holy_fat_open (holy_file_t file, const char *name)
{
  struct holy_fat_data *data = 0;
  holy_fshelp_node_t found = NULL;
  holy_err_t err;
  holy_disk_t disk = file->device->disk;

  holy_dl_ref (my_mod);

  data = holy_fat_mount (disk);
  if (! data)
    goto fail;

  struct holy_fshelp_node root = {
    .data = data,
    .disk = disk,
    .attr = holy_FAT_ATTR_DIRECTORY,
    .file_size = 0,
    .file_cluster = data->root_cluster,
    .cur_cluster_num = ~0U,
    .cur_cluster = 0,
#ifdef MODE_EXFAT
    .is_contiguous = 0,
#endif
  };

  err = holy_fshelp_find_file_lookup (name, &root, &found, lookup_file, NULL, holy_FSHELP_REG);
  if (err)
    goto fail;

  file->data = found;
  file->size = found->file_size;

  return holy_ERR_NONE;

 fail:

  if (found != &root)
    holy_free (found);

  holy_free (data);

  holy_dl_unref (my_mod);

  return holy_errno;
}

static holy_ssize_t
holy_fat_read (holy_file_t file, char *buf, holy_size_t len)
{
  return holy_fat_read_data (file->device->disk, file->data,
			     file->read_hook, file->read_hook_data,
			     file->offset, len, buf);
}

static holy_err_t
holy_fat_close (holy_file_t file)
{
  holy_fshelp_node_t node = file->data;

  holy_free (node->data);
  holy_free (node);

  holy_dl_unref (my_mod);

  return holy_errno;
}

#ifdef MODE_EXFAT
static holy_err_t
holy_fat_label (holy_device_t device, char **label)
{
  struct holy_fat_dir_entry dir;
  holy_ssize_t offset = -sizeof(dir);
  holy_disk_t disk = device->disk;
  struct holy_fshelp_node root = {
    .disk = disk,
    .attr = holy_FAT_ATTR_DIRECTORY,
    .file_size = 0,
    .cur_cluster_num = ~0U,
    .cur_cluster = 0,
    .is_contiguous = 0,
  };

  root.data = holy_fat_mount (disk);
  if (! root.data)
    return holy_errno;

  root.file_cluster = root.data->root_cluster;

  *label = NULL;

  while (1)
    {
      offset += sizeof (dir);

      if (holy_fat_read_data (disk, &root, 0, 0,
			       offset, sizeof (dir), (char *) &dir)
	   != sizeof (dir))
	break;

      if (dir.entry_type == 0)
	break;
      if (!(dir.entry_type & 0x80))
	continue;

      /* Volume label. */
      if (dir.entry_type == 0x83)
	{
	  holy_size_t chc;
	  holy_uint16_t t[ARRAY_SIZE (dir.type_specific.volume_label.str)];
	  holy_size_t i;
	  *label = holy_malloc (ARRAY_SIZE (dir.type_specific.volume_label.str)
				* holy_MAX_UTF8_PER_UTF16 + 1);
	  if (!*label)
	    {
	      holy_free (root.data);
	      return holy_errno;
	    }
	  chc = dir.type_specific.volume_label.character_count;
	  if (chc > ARRAY_SIZE (dir.type_specific.volume_label.str))
	    chc = ARRAY_SIZE (dir.type_specific.volume_label.str);
	  for (i = 0; i < chc; i++)
	    t[i] = holy_le_to_cpu16 (dir.type_specific.volume_label.str[i]);
	  *holy_utf16_to_utf8 ((holy_uint8_t *) *label, t, chc) = '\0';
	}
    }

  holy_free (root.data);
  return holy_errno;
}

#else

static holy_err_t
holy_fat_label (holy_device_t device, char **label)
{
  holy_disk_t disk = device->disk;
  holy_err_t err;
  struct holy_fat_iterate_context ctxt;
  struct holy_fshelp_node root = {
    .disk = disk,
    .attr = holy_FAT_ATTR_DIRECTORY,
    .file_size = 0,
    .cur_cluster_num = ~0U,
    .cur_cluster = 0,
  };

  *label = 0;

  holy_dl_ref (my_mod);

  root.data = holy_fat_mount (disk);
  if (! root.data)
    goto fail;

  root.file_cluster = root.data->root_cluster;

  err = holy_fat_iterate_init (&ctxt);
  if (err)
    goto fail;

  while (!(err = holy_fat_iterate_dir_next (&root, &ctxt)))
    if ((ctxt.dir.attr & ~holy_FAT_ATTR_ARCHIVE) == holy_FAT_ATTR_VOLUME_ID)
      {
	*label = holy_strdup (ctxt.filename);
	break;
      }

  holy_fat_iterate_fini (&ctxt);

 fail:

  holy_dl_unref (my_mod);

  holy_free (root.data);

  return holy_errno;
}

#endif

static holy_err_t
holy_fat_uuid (holy_device_t device, char **uuid)
{
  struct holy_fat_data *data;
  holy_disk_t disk = device->disk;

  holy_dl_ref (my_mod);

  data = holy_fat_mount (disk);
  if (data)
    {
      char *ptr;
      *uuid = holy_xasprintf ("%04x-%04x",
			     (holy_uint16_t) (data->uuid >> 16),
			     (holy_uint16_t) data->uuid);
      for (ptr = *uuid; ptr && *ptr; ptr++)
	*ptr = holy_toupper (*ptr);
    }
  else
    *uuid = NULL;

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}

#ifdef holy_UTIL
#ifndef MODE_EXFAT
holy_disk_addr_t
holy_fat_get_cluster_sector (holy_disk_t disk, holy_uint64_t *sec_per_lcn)
#else
holy_disk_addr_t
  holy_exfat_get_cluster_sector (holy_disk_t disk, holy_uint64_t *sec_per_lcn)
#endif
{
  holy_disk_addr_t ret;
  struct holy_fat_data *data;
  data = holy_fat_mount (disk);
  if (!data)
    return 0;
  ret = data->cluster_sector;

  *sec_per_lcn = 1ULL << data->cluster_bits;

  holy_free (data);
  return ret;
}
#endif

static struct holy_fs holy_fat_fs =
  {
#ifdef MODE_EXFAT
    .name = "exfat",
#else
    .name = "fat",
#endif
    .dir = holy_fat_dir,
    .open = holy_fat_open,
    .read = holy_fat_read,
    .close = holy_fat_close,
    .label = holy_fat_label,
    .uuid = holy_fat_uuid,
#ifdef holy_UTIL
#ifdef MODE_EXFAT
    /* ExFAT BPB is 30 larger than FAT32 one.  */
    .reserved_first_sector = 0,
#else
    .reserved_first_sector = 1,
#endif
    .blocklist_install = 1,
#endif
    .next = 0
  };

#ifdef MODE_EXFAT
holy_MOD_INIT(exfat)
#else
holy_MOD_INIT(fat)
#endif
{
  COMPILE_TIME_ASSERT (sizeof (struct holy_fat_dir_entry) == 32);
  holy_fs_register (&holy_fat_fs);
  my_mod = mod;
}
#ifdef MODE_EXFAT
holy_MOD_FINI(exfat)
#else
holy_MOD_FINI(fat)
#endif
{
  holy_fs_unregister (&holy_fat_fs);
}

