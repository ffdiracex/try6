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

#if defined(MAJOR_IN_MKDEV)
#include <sys/mkdev.h>
#elif defined(MAJOR_IN_SYSMACROS)
#include <sys/sysmacros.h>
#endif

#ifdef HAVE_DEVICE_MAPPER
# include <libdevmapper.h>

static void device_mapper_null_log (int level __attribute__ ((unused)),
				    const char *file __attribute__ ((unused)),
				    int line __attribute__ ((unused)),
				    int dm_errno __attribute__ ((unused)),
				    const char *f __attribute__ ((unused)),
				    ...)
{
}

int
holy_device_mapper_supported (void)
{
  static int supported = -1;

  if (supported == -1)
    {
      struct dm_task *dmt;

      /* Suppress annoying log messages.  */
      dm_log_with_errno_init (&device_mapper_null_log);

      dmt = dm_task_create (DM_DEVICE_VERSION);
      supported = (dmt != NULL);
      if (dmt)
	dm_task_destroy (dmt);

      /* Restore the original logger.  */
      dm_log_with_errno_init (NULL);
    }

  return supported;
}

int
holy_util_device_is_mapped (const char *dev)
{
  struct stat st;

  if (!holy_device_mapper_supported ())
    return 0;

  if (stat (dev, &st) < 0)
    return 0;

#if holy_DISK_DEVS_ARE_CHAR
  if (! S_ISCHR (st.st_mode))
#else
  if (! S_ISBLK (st.st_mode))
#endif
    return 0;

  return dm_is_dm_major (major (st.st_rdev));
}

int
holy_util_device_is_mapped_stat (struct stat *st)
{
#if holy_DISK_DEVS_ARE_CHAR
  if (! S_ISCHR (st->st_mode))
#else
  if (! S_ISBLK (st->st_mode))
#endif
    return 0;

  if (!holy_device_mapper_supported ())
    return 0;

  return dm_is_dm_major (major (st->st_rdev));
}


int
holy_util_get_dm_node_linear_info (dev_t dev,
				   int *maj, int *min,
				   holy_disk_addr_t *st)
{
  struct dm_task *dmt;
  void *next = NULL;
  uint64_t length, start;
  char *target, *params;
  char *ptr;
  int major = 0, minor = 0;
  int first = 1;
  holy_disk_addr_t partstart = 0;
  const char *node_uuid;

  major = major (dev);
  minor = minor (dev);

  while (1)
    {
      dmt = dm_task_create(DM_DEVICE_TABLE);
      if (!dmt)
	break;
      
      if (! (dm_task_set_major_minor (dmt, major, minor, 0)))
	{
	  dm_task_destroy (dmt);
	  break;
	}
      dm_task_no_open_count(dmt);
      if (!dm_task_run(dmt))
	{
	  dm_task_destroy (dmt);
	  break;
	}
      node_uuid = dm_task_get_uuid (dmt);
      if (node_uuid && (strncmp (node_uuid, "LVM-", 4) == 0
			|| strncmp (node_uuid, "mpath-", 6) == 0))
	{
	  dm_task_destroy (dmt);
	  break;
	}

      next = dm_get_next_target(dmt, next, &start, &length,
				&target, &params);
      if (holy_strcmp (target, "linear") != 0)
	{
	  dm_task_destroy (dmt);
	  break;
	}
      major = holy_strtoul (params, &ptr, 10);
      if (holy_errno)
	{
	  dm_task_destroy (dmt);
	  holy_errno = holy_ERR_NONE;
	  return 0;
	}
      if (*ptr != ':')
	{
	  dm_task_destroy (dmt);
	  return 0;
	}
      ptr++;
      minor = holy_strtoul (ptr, &ptr, 10);
      if (holy_errno)
	{
	  holy_errno = holy_ERR_NONE;
	  dm_task_destroy (dmt);
	  return 0;
	}

      if (*ptr != ' ')
	{
	  dm_task_destroy (dmt);
	  return 0;
	}
      ptr++;
      partstart += holy_strtoull (ptr, &ptr, 10);
      if (holy_errno)
	{
	  holy_errno = holy_ERR_NONE;
	  dm_task_destroy (dmt);
	  return 0;
	}

      dm_task_destroy (dmt);
      first = 0;
      if (!dm_is_dm_major (major))
	break;
    }
  if (first)
    return 0;
  if (maj)
    *maj = major;
  if (min)
    *min = minor;
  if (st)
    *st = partstart;
  return 1;
}
#else

int
holy_util_device_is_mapped (const char *dev __attribute__ ((unused)))
{
  return 0;
}

int
holy_util_get_dm_node_linear_info (dev_t dev __attribute__ ((unused)),
				   int *maj __attribute__ ((unused)),
				   int *min __attribute__ ((unused)),
				   holy_disk_addr_t *st __attribute__ ((unused)))
{
  return 0;
}

int
holy_util_device_is_mapped_stat (struct stat *st __attribute__ ((unused)))
{
  return 0;
}

#endif
