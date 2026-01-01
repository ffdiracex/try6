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

#define	holy_CACHE_TIMEOUT	2

/* The last time the disk was used.  */
static holy_uint64_t holy_last_time = 0;

struct holy_disk_cache holy_disk_cache_table[holy_DISK_CACHE_NUM];

void (*holy_disk_firmware_fini) (void);
int holy_disk_firmware_is_tainted;

#if DISK_CACHE_STATS
static unsigned long holy_disk_cache_hits;
static unsigned long holy_disk_cache_misses;

void
holy_disk_cache_get_performance (unsigned long *hits, unsigned long *misses)
{
  *hits = holy_disk_cache_hits;
  *misses = holy_disk_cache_misses;
}
#endif

holy_err_t (*holy_disk_write_weak) (holy_disk_t disk,
				    holy_disk_addr_t sector,
				    holy_off_t offset,
				    holy_size_t size,
				    const void *buf);
#include "disk_common.c"

void
holy_disk_cache_invalidate_all (void)
{
  unsigned i;

  for (i = 0; i < holy_DISK_CACHE_NUM; i++)
    {
      struct holy_disk_cache *cache = holy_disk_cache_table + i;

      if (cache->data && ! cache->lock)
	{
	  holy_free (cache->data);
	  cache->data = 0;
	}
    }
}

static char *
holy_disk_cache_fetch (unsigned long dev_id, unsigned long disk_id,
		       holy_disk_addr_t sector)
{
  struct holy_disk_cache *cache;
  unsigned cache_index;

  cache_index = holy_disk_cache_get_index (dev_id, disk_id, sector);
  cache = holy_disk_cache_table + cache_index;

  if (cache->dev_id == dev_id && cache->disk_id == disk_id
      && cache->sector == sector)
    {
      cache->lock = 1;
#if DISK_CACHE_STATS
      holy_disk_cache_hits++;
#endif
      return cache->data;
    }

#if DISK_CACHE_STATS
  holy_disk_cache_misses++;
#endif

  return 0;
}

static void
holy_disk_cache_unlock (unsigned long dev_id, unsigned long disk_id,
			holy_disk_addr_t sector)
{
  struct holy_disk_cache *cache;
  unsigned cache_index;

  cache_index = holy_disk_cache_get_index (dev_id, disk_id, sector);
  cache = holy_disk_cache_table + cache_index;

  if (cache->dev_id == dev_id && cache->disk_id == disk_id
      && cache->sector == sector)
    cache->lock = 0;
}

static holy_err_t
holy_disk_cache_store (unsigned long dev_id, unsigned long disk_id,
		       holy_disk_addr_t sector, const char *data)
{
  unsigned cache_index;
  struct holy_disk_cache *cache;

  cache_index = holy_disk_cache_get_index (dev_id, disk_id, sector);
  cache = holy_disk_cache_table + cache_index;

  cache->lock = 1;
  holy_free (cache->data);
  cache->data = 0;
  cache->lock = 0;

  cache->data = holy_malloc (holy_DISK_SECTOR_SIZE << holy_DISK_CACHE_BITS);
  if (! cache->data)
    return holy_errno;

  holy_memcpy (cache->data, data,
	       holy_DISK_SECTOR_SIZE << holy_DISK_CACHE_BITS);
  cache->dev_id = dev_id;
  cache->disk_id = disk_id;
  cache->sector = sector;

  return holy_ERR_NONE;
}



holy_disk_dev_t holy_disk_dev_list;

void
holy_disk_dev_register (holy_disk_dev_t dev)
{
  dev->next = holy_disk_dev_list;
  holy_disk_dev_list = dev;
}

void
holy_disk_dev_unregister (holy_disk_dev_t dev)
{
  holy_disk_dev_t *p, q;

  for (p = &holy_disk_dev_list, q = *p; q; p = &(q->next), q = q->next)
    if (q == dev)
      {
        *p = q->next;
	break;
      }
}

/* Return the location of the first ',', if any, which is not
   escaped by a '\'.  */
static const char *
find_part_sep (const char *name)
{
  const char *p = name;
  char c;

  while ((c = *p++) != '\0')
    {
      if (c == '\\' && *p == ',')
	p++;
      else if (c == ',')
	return p - 1;
    }
  return NULL;
}

holy_disk_t
holy_disk_open (const char *name)
{
  const char *p;
  holy_disk_t disk;
  holy_disk_dev_t dev;
  char *raw = (char *) name;
  holy_uint64_t current_time;

  holy_dprintf ("disk", "Opening `%s'...\n", name);

  disk = (holy_disk_t) holy_zalloc (sizeof (*disk));
  if (! disk)
    return 0;
  disk->log_sector_size = holy_DISK_SECTOR_BITS;
  /* Default 1MiB of maximum agglomerate.  */
  disk->max_agglomerate = 1048576 >> (holy_DISK_SECTOR_BITS
				      + holy_DISK_CACHE_BITS);

  p = find_part_sep (name);
  if (p)
    {
      holy_size_t len = p - name;

      raw = holy_malloc (len + 1);
      if (! raw)
	goto fail;

      holy_memcpy (raw, name, len);
      raw[len] = '\0';
      disk->name = holy_strdup (raw);
    }
  else
    disk->name = holy_strdup (name);
  if (! disk->name)
    goto fail;

  for (dev = holy_disk_dev_list; dev; dev = dev->next)
    {
      if ((dev->open) (raw, disk) == holy_ERR_NONE)
	break;
      else if (holy_errno == holy_ERR_UNKNOWN_DEVICE)
	holy_errno = holy_ERR_NONE;
      else
	goto fail;
    }

  if (! dev)
    {
      holy_error (holy_ERR_UNKNOWN_DEVICE, N_("disk `%s' not found"),
		  name);
      goto fail;
    }
  if (disk->log_sector_size > holy_DISK_CACHE_BITS + holy_DISK_SECTOR_BITS
      || disk->log_sector_size < holy_DISK_SECTOR_BITS)
    {
      holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		  "sector sizes of %d bytes aren't supported yet",
		  (1 << disk->log_sector_size));
      goto fail;
    }

  disk->dev = dev;

  if (p)
    {
      disk->partition = holy_partition_probe (disk, p + 1);
      if (! disk->partition)
	{
	  /* TRANSLATORS: It means that the specified partition e.g.
	     hd0,msdos1=/dev/sda1 doesn't exist.  */
	  holy_error (holy_ERR_UNKNOWN_DEVICE, N_("no such partition"));
	  goto fail;
	}
    }

  /* The cache will be invalidated about 2 seconds after a device was
     closed.  */
  current_time = holy_get_time_ms ();

  if (current_time > (holy_last_time
		      + holy_CACHE_TIMEOUT * 1000))
    holy_disk_cache_invalidate_all ();

  holy_last_time = current_time;

 fail:

  if (raw && raw != name)
    holy_free (raw);

  if (holy_errno != holy_ERR_NONE)
    {
      holy_error_push ();
      holy_dprintf ("disk", "Opening `%s' failed.\n", name);
      holy_error_pop ();

      holy_disk_close (disk);
      return 0;
    }

  return disk;
}

void
holy_disk_close (holy_disk_t disk)
{
  holy_partition_t part;
  holy_dprintf ("disk", "Closing `%s'.\n", disk->name);

  if (disk->dev && disk->dev->close)
    (disk->dev->close) (disk);

  /* Reset the timer.  */
  holy_last_time = holy_get_time_ms ();

  while (disk->partition)
    {
      part = disk->partition->parent;
      holy_free (disk->partition);
      disk->partition = part;
    }
  holy_free ((void *) disk->name);
  holy_free (disk);
}

/* Small read (less than cache size and not pass across cache unit boundaries).
   sector is already adjusted and is divisible by cache unit size.
 */
static holy_err_t
holy_disk_read_small_real (holy_disk_t disk, holy_disk_addr_t sector,
			   holy_off_t offset, holy_size_t size, void *buf)
{
  char *data;
  char *tmp_buf;

  /* Fetch the cache.  */
  data = holy_disk_cache_fetch (disk->dev->id, disk->id, sector);
  if (data)
    {
      /* Just copy it!  */
      holy_memcpy (buf, data + offset, size);
      holy_disk_cache_unlock (disk->dev->id, disk->id, sector);
      return holy_ERR_NONE;
    }

  /* Allocate a temporary buffer.  */
  tmp_buf = holy_malloc (holy_DISK_SECTOR_SIZE << holy_DISK_CACHE_BITS);
  if (! tmp_buf)
    return holy_errno;

  /* Otherwise read data from the disk actually.  */
  if (disk->total_sectors == holy_DISK_SIZE_UNKNOWN
      || sector + holy_DISK_CACHE_SIZE
      < (disk->total_sectors << (disk->log_sector_size - holy_DISK_SECTOR_BITS)))
    {
      holy_err_t err;
      err = (disk->dev->read) (disk, transform_sector (disk, sector),
			       1U << (holy_DISK_CACHE_BITS
				      + holy_DISK_SECTOR_BITS
				      - disk->log_sector_size), tmp_buf);
      if (!err)
	{
	  /* Copy it and store it in the disk cache.  */
	  holy_memcpy (buf, tmp_buf + offset, size);
	  holy_disk_cache_store (disk->dev->id, disk->id,
				 sector, tmp_buf);
	  holy_free (tmp_buf);
	  return holy_ERR_NONE;
	}
    }

  holy_free (tmp_buf);
  holy_errno = holy_ERR_NONE;

  {
    /* Uggh... Failed. Instead, just read necessary data.  */
    unsigned num;
    holy_disk_addr_t aligned_sector;

    sector += (offset >> holy_DISK_SECTOR_BITS);
    offset &= ((1 << holy_DISK_SECTOR_BITS) - 1);
    aligned_sector = (sector & ~((1ULL << (disk->log_sector_size
					   - holy_DISK_SECTOR_BITS))
				 - 1));
    offset += ((sector - aligned_sector) << holy_DISK_SECTOR_BITS);
    num = ((size + offset + (1ULL << (disk->log_sector_size))
	    - 1) >> (disk->log_sector_size));

    tmp_buf = holy_malloc (num << disk->log_sector_size);
    if (!tmp_buf)
      return holy_errno;
    
    if ((disk->dev->read) (disk, transform_sector (disk, aligned_sector),
			   num, tmp_buf))
      {
	holy_error_push ();
	holy_dprintf ("disk", "%s read failed\n", disk->name);
	holy_error_pop ();
	holy_free (tmp_buf);
	return holy_errno;
      }
    holy_memcpy (buf, tmp_buf + offset, size);
    holy_free (tmp_buf);
    return holy_ERR_NONE;
  }
}

static holy_err_t
holy_disk_read_small (holy_disk_t disk, holy_disk_addr_t sector,
		      holy_off_t offset, holy_size_t size, void *buf)
{
  holy_err_t err;

  err = holy_disk_read_small_real (disk, sector, offset, size, buf);
  if (err)
    return err;
  if (disk->read_hook)
    (disk->read_hook) (sector + (offset >> holy_DISK_SECTOR_BITS),
		       offset & (holy_DISK_SECTOR_SIZE - 1),
		       size, disk->read_hook_data);
  return holy_ERR_NONE;
}

/* Read data from the disk.  */
holy_err_t
holy_disk_read (holy_disk_t disk, holy_disk_addr_t sector,
		holy_off_t offset, holy_size_t size, void *buf)
{
  /* First of all, check if the region is within the disk.  */
  if (holy_disk_adjust_range (disk, &sector, &offset, size) != holy_ERR_NONE)
    {
      holy_error_push ();
      holy_dprintf ("disk", "Read out of range: sector 0x%llx (%s).\n",
		    (unsigned long long) sector, holy_errmsg);
      holy_error_pop ();
      return holy_errno;
    }

  /* First read until first cache boundary.   */
  if (offset || (sector & (holy_DISK_CACHE_SIZE - 1)))
    {
      holy_disk_addr_t start_sector;
      holy_size_t pos;
      holy_err_t err;
      holy_size_t len;

      start_sector = sector & ~((holy_disk_addr_t) holy_DISK_CACHE_SIZE - 1);
      pos = (sector - start_sector) << holy_DISK_SECTOR_BITS;
      len = ((holy_DISK_SECTOR_SIZE << holy_DISK_CACHE_BITS)
	     - pos - offset);
      if (len > size)
	len = size;
      err = holy_disk_read_small (disk, start_sector,
				  offset + pos, len, buf);
      if (err)
	return err;
      buf = (char *) buf + len;
      size -= len;
      offset += len;
      sector += (offset >> holy_DISK_SECTOR_BITS);
      offset &= ((1 << holy_DISK_SECTOR_BITS) - 1);
    }

  /* Until SIZE is zero...  */
  while (size >= (holy_DISK_CACHE_SIZE << holy_DISK_SECTOR_BITS))
    {
      char *data = NULL;
      holy_disk_addr_t agglomerate;
      holy_err_t err;

      /* agglomerate read until we find a first cached entry.  */
      for (agglomerate = 0; agglomerate
	     < (size >> (holy_DISK_SECTOR_BITS + holy_DISK_CACHE_BITS))
	     && agglomerate < disk->max_agglomerate;
	   agglomerate++)
	{
	  data = holy_disk_cache_fetch (disk->dev->id, disk->id,
					sector + (agglomerate
						  << holy_DISK_CACHE_BITS));
	  if (data)
	    break;
	}

      if (data)
	{
	  holy_memcpy ((char *) buf
		       + (agglomerate << (holy_DISK_CACHE_BITS
					  + holy_DISK_SECTOR_BITS)),
		       data, holy_DISK_CACHE_SIZE << holy_DISK_SECTOR_BITS);
	  holy_disk_cache_unlock (disk->dev->id, disk->id,
				  sector + (agglomerate
					    << holy_DISK_CACHE_BITS));
	}

      if (agglomerate)
	{
	  holy_disk_addr_t i;

	  err = (disk->dev->read) (disk, transform_sector (disk, sector),
				   agglomerate << (holy_DISK_CACHE_BITS
						   + holy_DISK_SECTOR_BITS
						   - disk->log_sector_size),
				   buf);
	  if (err)
	    return err;
	  
	  for (i = 0; i < agglomerate; i ++)
	    holy_disk_cache_store (disk->dev->id, disk->id,
				   sector + (i << holy_DISK_CACHE_BITS),
				   (char *) buf
				   + (i << (holy_DISK_CACHE_BITS
					    + holy_DISK_SECTOR_BITS)));


	  if (disk->read_hook)
	    (disk->read_hook) (sector, 0, agglomerate << (holy_DISK_CACHE_BITS + holy_DISK_SECTOR_BITS),
			       disk->read_hook_data);

	  sector += agglomerate << holy_DISK_CACHE_BITS;
	  size -= agglomerate << (holy_DISK_CACHE_BITS + holy_DISK_SECTOR_BITS);
	  buf = (char *) buf 
	    + (agglomerate << (holy_DISK_CACHE_BITS + holy_DISK_SECTOR_BITS));
	}

      if (data)
	{
	  if (disk->read_hook)
	    (disk->read_hook) (sector, 0, (holy_DISK_CACHE_SIZE << holy_DISK_SECTOR_BITS),
			       disk->read_hook_data);
	  sector += holy_DISK_CACHE_SIZE;
	  buf = (char *) buf + (holy_DISK_CACHE_SIZE << holy_DISK_SECTOR_BITS);
	  size -= (holy_DISK_CACHE_SIZE << holy_DISK_SECTOR_BITS);
	}
    }

  /* And now read the last part.  */
  if (size)
    {
      holy_err_t err;
      err = holy_disk_read_small (disk, sector, 0, size, buf);
      if (err)
	return err;
    }

  return holy_errno;
}

holy_uint64_t
holy_disk_get_size (holy_disk_t disk)
{
  if (disk->partition)
    return holy_partition_get_len (disk->partition);
  else if (disk->total_sectors != holy_DISK_SIZE_UNKNOWN)
    return disk->total_sectors << (disk->log_sector_size - holy_DISK_SECTOR_BITS);
  else
    return holy_DISK_SIZE_UNKNOWN;
}
