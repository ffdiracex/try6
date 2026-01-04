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

# include <sys/ioctl.h>
# include <sys/disklabel.h>    /* struct disklabel */
# include <sys/disk.h>    /* struct dkwedge_info */
# ifdef HAVE_GETRAWPARTITION
#  include <util.h>    /* getrawpartition */
# endif /* HAVE_GETRAWPARTITION */
# if defined(__NetBSD__)
# include <sys/fdio.h>
# endif
# if defined(__OpenBSD__)
# include <sys/dkio.h>
# endif

#if defined(__NetBSD__)
/* Adjust device driver parameters.  This function should be called just
   after successfully opening the device.  For now, it simply prevents the
   floppy driver from retrying operations on failure, as otherwise the
   driver takes a while to abort when there is no floppy in the drive.  */
static void
configure_device_driver (holy_util_fd_t fd)
{
  struct stat st;

  if (fstat (fd, &st) < 0 || ! S_ISCHR (st.st_mode))
    return;
  if (major(st.st_rdev) == RAW_FLOPPY_MAJOR)
    {
      int floppy_opts;

      if (ioctl (fd, FDIOCGETOPTS, &floppy_opts) == -1)
	return;
      floppy_opts |= FDOPT_NORETRY;
      if (ioctl (fd, FDIOCSETOPTS, &floppy_opts) == -1)
	return;
    }
}
holy_util_fd_t
holy_util_fd_open (const char *os_dev, int flags)
{
  holy_util_fd_t ret;

#ifdef O_LARGEFILE
  flags |= O_LARGEFILE;
#endif
#ifdef O_BINARY
  flags |= O_BINARY;
#endif

  ret = open (os_dev, flags, S_IROTH | S_IRGRP | S_IRUSR | S_IWUSR);
  if (ret >= 0)
    configure_device_driver (fd);
  return ret;
}

#endif

holy_int64_t
holy_util_get_fd_size_os (holy_util_fd_t fd, const char *name, unsigned *log_secsize)
{
  struct disklabel label;
  unsigned sector_size, log_sector_size;

#if defined(__NetBSD__)
  holy_hostdisk_configure_device_driver (fd);
#endif

  if (ioctl (fd, DIOCGDINFO, &label) == -1)
    return -1;

  sector_size = label.d_secsize;
  if (sector_size & (sector_size - 1) || !sector_size)
    return -1;
  for (log_sector_size = 0;
       (1 << log_sector_size) < sector_size;
       log_sector_size++);

  if (log_secsize)
    *log_secsize = log_sector_size;

  return (holy_uint64_t) label.d_secperunit << log_sector_size;
}

void
holy_hostdisk_flush_initial_buffer (const char *os_dev __attribute__ ((unused)))
{
}
