/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/disk.h>
#include <holy/err.h>
#include <holy/mm.h>
#include <holy/types.h>
#include <holy/partition.h>
#include <holy/misc.h>
#include <holy/time.h>
#include <holy/file.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

#include "../kern/disk_common.c"

static void
holy_disk_cache_invalidate (unsigned long dev_id, unsigned long disk_id,
			    holy_disk_addr_t sector)
{
  unsigned cache_index;
  struct holy_disk_cache *cache;

  sector &= ~((holy_disk_addr_t) holy_DISK_CACHE_SIZE - 1);
  cache_index = holy_disk_cache_get_index (dev_id, disk_id, sector);
  cache = holy_disk_cache_table + cache_index;

  if (cache->dev_id == dev_id && cache->disk_id == disk_id
      && cache->sector == sector && cache->data)
    {
      cache->lock = 1;
      holy_free (cache->data);
      cache->data = 0;
      cache->lock = 0;
    }
}

holy_err_t
holy_disk_write (holy_disk_t disk, holy_disk_addr_t sector,
		 holy_off_t offset, holy_size_t size, const void *buf)
{
  unsigned real_offset;
  holy_disk_addr_t aligned_sector;

  holy_dprintf ("disk", "Writing `%s'...\n", disk->name);

  if (holy_disk_adjust_range (disk, &sector, &offset, size) != holy_ERR_NONE)
    return -1;

  aligned_sector = (sector & ~((1ULL << (disk->log_sector_size
					 - holy_DISK_SECTOR_BITS)) - 1));
  real_offset = offset + ((sector - aligned_sector) << holy_DISK_SECTOR_BITS);
  sector = aligned_sector;

  while (size)
    {
      if (real_offset != 0 || (size < (1U << disk->log_sector_size)
			       && size != 0))
	{
	  char *tmp_buf;
	  holy_size_t len;
	  holy_partition_t part;

	  tmp_buf = holy_malloc (1U << disk->log_sector_size);
	  if (!tmp_buf)
	    return holy_errno;

	  part = disk->partition;
	  disk->partition = 0;
	  if (holy_disk_read (disk, sector,
			      0, (1U << disk->log_sector_size), tmp_buf)
	      != holy_ERR_NONE)
	    {
	      disk->partition = part;
	      holy_free (tmp_buf);
	      goto finish;
	    }
	  disk->partition = part;

	  len = (1U << disk->log_sector_size) - real_offset;
	  if (len > size)
	    len = size;

	  holy_memcpy (tmp_buf + real_offset, buf, len);

	  holy_disk_cache_invalidate (disk->dev->id, disk->id, sector);

	  if ((disk->dev->write) (disk, transform_sector (disk, sector),
				  1, tmp_buf) != holy_ERR_NONE)
	    {
	      holy_free (tmp_buf);
	      goto finish;
	    }

	  holy_free (tmp_buf);

	  sector += (1U << (disk->log_sector_size - holy_DISK_SECTOR_BITS));
	  buf = (const char *) buf + len;
	  size -= len;
	  real_offset = 0;
	}
      else
	{
	  holy_size_t len;
	  holy_size_t n;

	  len = size & ~((1ULL << disk->log_sector_size) - 1);
	  n = size >> disk->log_sector_size;

	  if (n > (disk->max_agglomerate
		   << (holy_DISK_CACHE_BITS + holy_DISK_SECTOR_BITS
		       - disk->log_sector_size)))
	    n = (disk->max_agglomerate
		 << (holy_DISK_CACHE_BITS + holy_DISK_SECTOR_BITS
		     - disk->log_sector_size));

	  if ((disk->dev->write) (disk, transform_sector (disk, sector),
				  n, buf) != holy_ERR_NONE)
	    goto finish;

	  while (n--)
	    {
	      holy_disk_cache_invalidate (disk->dev->id, disk->id, sector);
	      sector += (1U << (disk->log_sector_size - holy_DISK_SECTOR_BITS));
	    }

	  buf = (const char *) buf + len;
	  size -= len;
	}
    }

 finish:

  return holy_errno;
}

holy_MOD_INIT(disk)
{
  holy_disk_write_weak = holy_disk_write;
}

holy_MOD_FINI(disk)
{
  holy_disk_write_weak = NULL;
}
