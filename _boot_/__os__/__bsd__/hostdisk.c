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

# include <sys/disk.h> /* DIOCGMEDIASIZE */
# include <sys/param.h>
# include <sys/sysctl.h>
# include <sys/mount.h>
# include <libgeom.h>

holy_int64_t
holy_util_get_fd_size_os (holy_util_fd_t fd, const char *name, unsigned *log_secsize)
{
  unsigned long long nr;
  unsigned sector_size, log_sector_size;

  if (ioctl (fd, DIOCGMEDIASIZE, &nr))
    return -1;

  if (ioctl (fd, DIOCGSECTORSIZE, &sector_size))
    return -1;
  if (sector_size & (sector_size - 1) || !sector_size)
    return -1;
  for (log_sector_size = 0;
       (1 << log_sector_size) < sector_size;
       log_sector_size++);

  if (log_secsize)
    *log_secsize = log_sector_size;

  if (nr & (sector_size - 1))
    holy_util_error ("%s", _("unaligned device size"));

  return nr;
}

void
holy_hostdisk_flush_initial_buffer (const char *os_dev __attribute__ ((unused)))
{
}

holy_util_fd_t
holy_util_fd_open (const char *os_dev, int flags)
{
  holy_util_fd_t ret;
  int sysctl_flags, sysctl_oldflags;
  size_t sysctl_size = sizeof (sysctl_flags);

#ifdef O_LARGEFILE
  flags |= O_LARGEFILE;
#endif
#ifdef O_BINARY
  flags |= O_BINARY;
#endif

  if (sysctlbyname ("kern.geom.debugflags", &sysctl_oldflags, &sysctl_size, NULL, 0))
    {
      holy_error (holy_ERR_BAD_DEVICE, "cannot get current flags of sysctl kern.geom.debugflags");
      return holy_UTIL_FD_INVALID;
    }
  sysctl_flags = sysctl_oldflags | 0x10;
  if (! (sysctl_oldflags & 0x10)
      && sysctlbyname ("kern.geom.debugflags", NULL , 0, &sysctl_flags, sysctl_size))
    {
      if (errno == EPERM)
	/* Running as an unprivileged user; don't worry about restoring
	   flags, although if we try to write to anything interesting such
	   as the MBR then we may fail later.  */
	sysctl_oldflags = 0x10;
      else
	{
	  holy_error (holy_ERR_BAD_DEVICE, "cannot set flags of sysctl kern.geom.debugflags");
	  return holy_UTIL_FD_INVALID;
	}
    }

  ret = open (os_dev, flags, S_IROTH | S_IRGRP | S_IRUSR | S_IWUSR);

  if (! (sysctl_oldflags & 0x10)
      && sysctlbyname ("kern.geom.debugflags", NULL , 0, &sysctl_oldflags, sysctl_size))
    {
      holy_error (holy_ERR_BAD_DEVICE, "cannot set flags back to the old value for sysctl kern.geom.debugflags");
      close (ret);
      return holy_UTIL_FD_INVALID;
    }

  return ret;
}
