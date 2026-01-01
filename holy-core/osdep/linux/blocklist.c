/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/fiemap.h>

#include <holy/disk.h>
#include <holy/partition.h>
#include <holy/util/misc.h>
#include <holy/util/install.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void
holy_install_get_blocklist (holy_device_t root_dev,
			    const char *core_path,
			    const char *core_img __attribute__ ((unused)),
			    size_t core_size,
			    void (*callback) (holy_disk_addr_t sector,
					      unsigned offset,
					      unsigned length,
					      void *data),
			    void *hook_data)
{
  holy_partition_t container = root_dev->disk->partition;
  holy_uint64_t container_start = holy_partition_get_start (container);
  struct fiemap fie1;
  int fd;

  /* Write the first two sectors of the core image onto the disk.  */
  holy_util_info ("opening the core image `%s'", core_path);
  fd = open (core_path, O_RDONLY);
  if (fd < 0)
    holy_util_error (_("cannot open `%s': %s"), core_path,
		     strerror (errno));

  holy_memset (&fie1, 0, sizeof (fie1));
  fie1.fm_length = core_size;
  fie1.fm_flags = FIEMAP_FLAG_SYNC;

  if (ioctl (fd, FS_IOC_FIEMAP, &fie1) < 0)
    {
      int nblocks, i;
      int bsize;
      int mul;

      holy_util_info ("FIEMAP failed. Reverting to FIBMAP");

      if (ioctl (fd, FIGETBSZ, &bsize) < 0)
	holy_util_error (_("can't retrieve blocklists: %s"),
			 strerror (errno));
      if (bsize & (holy_DISK_SECTOR_SIZE - 1))
	holy_util_error ("%s", _("blocksize is not divisible by 512"));
      if (!bsize)
	holy_util_error ("%s", _("invalid zero blocksize"));
      mul = bsize >> holy_DISK_SECTOR_BITS;
      nblocks = (core_size + bsize - 1) / bsize;
      if (mul == 0 || nblocks == 0)
	holy_util_error ("%s", _("can't retrieve blocklists"));
      for (i = 0; i < nblocks; i++)
	{
	  unsigned blk = i;
	  int rest;
	  if (ioctl (fd, FIBMAP, &blk) < 0)
	    holy_util_error (_("can't retrieve blocklists: %s"),
			     strerror (errno));
	    
	  rest = core_size - ((i * mul) << holy_DISK_SECTOR_BITS);
	  if (rest <= 0)
	    break;
	  if (rest > holy_DISK_SECTOR_SIZE * mul)
	    rest = holy_DISK_SECTOR_SIZE * mul;
	  callback (((holy_uint64_t) blk) * mul
		    + container_start,
		    0, rest, hook_data);
	}
    }
  else
    {
      struct fiemap *fie2;
      int i;
      fie2 = xmalloc (sizeof (*fie2)
		      + fie1.fm_mapped_extents
		      * sizeof (fie1.fm_extents[1]));
      memset (fie2, 0, sizeof (*fie2)
	      + fie1.fm_mapped_extents * sizeof (fie2->fm_extents[1]));
      fie2->fm_length = core_size;
      fie2->fm_flags = FIEMAP_FLAG_SYNC;
      fie2->fm_extent_count = fie1.fm_mapped_extents;
      if (ioctl (fd, FS_IOC_FIEMAP, fie2) < 0)
	holy_util_error (_("can't retrieve blocklists: %s"),
			 strerror (errno));
      for (i = 0; i < fie2->fm_mapped_extents; i++)
	{
	  callback ((fie2->fm_extents[i].fe_physical
		     >> holy_DISK_SECTOR_BITS)
		    + container_start,
		    fie2->fm_extents[i].fe_physical
		    & (holy_DISK_SECTOR_SIZE - 1),
		    fie2->fm_extents[i].fe_length, hook_data);
	}
      free (fie2);
    }
  close (fd);
}
