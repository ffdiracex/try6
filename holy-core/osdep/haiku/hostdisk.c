/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config-util.h>

#include <holy/disk.h>
#include <holy/partition.h>
#include <holy/msdos_partition.h>
#include <holy/types.h>
#include <holy/err.h>
#include <holy/emu/misc.h>
#include <holy/emu/hostdisk.h>
#include <holy/emu/getroot.h>
#include <holy/misc.h>
#include <holy/i18n.h>
#include <holy/list.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <Drivers.h>
#include <StorageDefs.h>

holy_int64_t
holy_util_get_fd_size_os (holy_util_fd_t fd,
			  const char *name __attribute__ ((unused)),
			  unsigned *log_secsize)
{
  device_geometry part;
  unsigned lg;
  if (ioctl (fd, B_GET_GEOMETRY, &part, sizeof (part)) < 0)  
    return -1;
  for (lg = 0; (1 << lg) < part.bytes_per_sector; lg++);
  if (log_secsize)
    *log_secsize= lg;
  return ((holy_uint64_t) part.cylinder_count
	  * (holy_uint64_t) part.head_count
	  * (holy_uint64_t) part.sectors_per_track
	  * (holy_uint64_t) part.bytes_per_sector);
}

void
holy_hostdisk_flush_initial_buffer (const char *os_dev __attribute__ ((unused)))
{
}
