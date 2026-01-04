/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config-util.h>
#include <config.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <holy/types.h>

#include <holy/util/misc.h>

#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/emu/misc.h>
#include <holy/emu/hostdisk.h>
#include <holy/emu/getroot.h>

#include <sys/wait.h>

# include <sys/types.h>
# include <sys/mkdev.h>
# include <sys/dkio.h>


char *
holy_util_part_to_disk (const char *os_dev,
			struct stat *st,
			int *is_part)
{
  char *colon = holy_strrchr (os_dev, ':');

  if (! S_ISCHR (st->st_mode))
    {
      *is_part = 0;
      return xstrdup (os_dev);
    }

  if (holy_memcmp (os_dev, "/devices", sizeof ("/devices") - 1) == 0
      && colon)
    {
      char *ret = xmalloc (colon - os_dev + sizeof (":q,raw"));
      if (holy_strcmp (colon, ":q,raw") != 0)
	*is_part = 1;
      holy_memcpy (ret, os_dev, colon - os_dev);
      holy_memcpy (ret + (colon - os_dev), ":q,raw", sizeof (":q,raw"));
      return ret;
    }
  else
    return xstrdup (os_dev);
}

enum holy_dev_abstraction_types
holy_util_get_dev_abstraction_os (const char *os_dev __attribute__((unused)))
{
  return holy_DEV_ABSTRACTION_NONE;
}

int
holy_util_pull_device_os (const char *os_dev __attribute__ ((unused)),
			  enum holy_dev_abstraction_types ab __attribute__ ((unused)))
{
  return 0;
}

char *
holy_util_get_holy_dev_os (const char *os_dev __attribute__ ((unused)))
{
  return NULL;
}

holy_disk_addr_t
holy_util_find_partition_start_os (const char *dev)
{
  int fd;
  struct extpart_info pinfo;

  fd = open (dev, O_RDONLY);
  if (fd == -1)
    {
      holy_error (holy_ERR_BAD_DEVICE, N_("cannot open `%s': %s"),
		  dev, strerror (errno));
      return 0;
    }

  if (ioctl (fd, DKIOCEXTPARTINFO, &pinfo))
    {
      holy_error (holy_ERR_BAD_DEVICE,
		  "cannot get disk geometry of `%s'", dev);
      close (fd);
      return 0;
    }

  close (fd);

  return pinfo.p_start;
}
