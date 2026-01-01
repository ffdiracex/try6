/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

static holy_err_t
holy_disk_adjust_range (holy_disk_t disk, holy_disk_addr_t *sector,
			holy_off_t *offset, holy_size_t size)
{
  holy_partition_t part;
  holy_disk_addr_t total_sectors;

  *sector += *offset >> holy_DISK_SECTOR_BITS;
  *offset &= holy_DISK_SECTOR_SIZE - 1;

  for (part = disk->partition; part; part = part->parent)
    {
      holy_disk_addr_t start;
      holy_uint64_t len;

      start = part->start;
      len = part->len;

      if (*sector >= len
	  || len - *sector < ((*offset + size + holy_DISK_SECTOR_SIZE - 1)
			      >> holy_DISK_SECTOR_BITS))
	return holy_error (holy_ERR_OUT_OF_RANGE,
			   N_("attempt to read or write outside of partition"));

      *sector += start;
    }

  /* Transform total_sectors to number of 512B blocks.  */
  total_sectors = disk->total_sectors << (disk->log_sector_size - holy_DISK_SECTOR_BITS);

  /* Some drivers have problems with disks above reasonable.
     Treat unknown as 1EiB disk. While on it, clamp the size to 1EiB.
     Just one condition is enough since holy_DISK_UNKNOWN_SIZE << ls is always
     above 9EiB.
  */
  if (total_sectors > (1ULL << 51))
    total_sectors = (1ULL << 51);

  if ((total_sectors <= *sector
       || ((*offset + size + holy_DISK_SECTOR_SIZE - 1)
	   >> holy_DISK_SECTOR_BITS) > total_sectors - *sector))
    return holy_error (holy_ERR_OUT_OF_RANGE,
		       N_("attempt to read or write outside of disk `%s'"), disk->name);

  return holy_ERR_NONE;
}

static inline holy_disk_addr_t
transform_sector (holy_disk_t disk, holy_disk_addr_t sector)
{
  return sector >> (disk->log_sector_size - holy_DISK_SECTOR_BITS);
}

static unsigned
holy_disk_cache_get_index (unsigned long dev_id, unsigned long disk_id,
			   holy_disk_addr_t sector)
{
  return ((dev_id * 524287UL + disk_id * 2606459UL
	   + ((unsigned) (sector >> holy_DISK_CACHE_BITS)))
	  % holy_DISK_CACHE_NUM);
}
