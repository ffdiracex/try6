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

# include <sys/dkio.h>

holy_int64_t
holy_util_get_fd_size_os (holy_util_fd_t fd, const char *name, unsigned *log_secsize)
{
  struct dk_minfo minfo;
  unsigned sector_size, log_sector_size;

  if (!ioctl (fd, DKIOCGMEDIAINFO, &minfo))
    return -1;

  sector_size = minfo.dki_lbsize;

  if (sector_size & (sector_size - 1) || !sector_size)
    return -1;
  for (log_sector_size = 0;
       (1 << log_sector_size) < sector_size;
       log_sector_size++);

  if (log_secsize)
    *log_secsize = log_sector_size;

  return minfo.dki_capacity << log_sector_size;
}

void
holy_hostdisk_flush_initial_buffer (const char *os_dev __attribute__ ((unused)))
{
}
