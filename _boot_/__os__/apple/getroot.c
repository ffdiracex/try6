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

# include <sys/disk.h>
# include <sys/param.h>
# include <sys/sysctl.h>
# include <sys/mount.h>

char *
holy_util_part_to_disk (const char *os_dev, struct stat *st,
			int *is_part)
{
  char *path = xstrdup (os_dev);

  if (! S_ISCHR (st->st_mode))
    {
      *is_part = 0;
      return path;
    }

  if (strncmp ("/dev/", path, 5) == 0)
    {
      char *p;
      for (p = path + 5; *p; ++p)
        if (holy_isdigit(*p))
          {
            p = strpbrk (p, "sp");
            if (p)
	      {
		*is_part = 1;
		*p = '\0';
	      }
            break;
          }
    }
  return path;
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
holy_util_find_partition_start_os (const char *dev __attribute__ ((unused)))
{
  return 0;
}
