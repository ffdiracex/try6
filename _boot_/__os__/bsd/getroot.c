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

# include <sys/ioctl.h>
# include <sys/disklabel.h>    /* struct disklabel */
# include <sys/disk.h>    /* struct dkwedge_info */
# ifdef HAVE_GETRAWPARTITION
#  include <util.h>    /* getrawpartition */
# endif /* HAVE_GETRAWPARTITION */
#if defined(__NetBSD__)
# include <sys/fdio.h>
#endif
#if defined(__OpenBSD__)
# include <sys/dkio.h>
#endif

char *
holy_util_part_to_disk (const char *os_dev, struct stat *st,
			int *is_part)
{
  int rawpart = -1;

  if (! S_ISCHR (st->st_mode))
    {
      *is_part = 0;
      return xstrdup (os_dev);
    }

# ifdef HAVE_GETRAWPARTITION
  rawpart = getrawpartition();
# endif /* HAVE_GETRAWPARTITION */
  if (rawpart < 0)
    return xstrdup (os_dev);

#if defined(__NetBSD__)
  /* NetBSD disk wedges are of the form "/dev/rdk.*".  */
  if (strncmp ("/dev/rdk", os_dev, sizeof("/dev/rdk") - 1) == 0)
    {
      struct dkwedge_info dkw;
      int fd;

      fd = open (os_dev, O_RDONLY);
      if (fd == -1)
	{
	  holy_error (holy_ERR_BAD_DEVICE,
		      N_("cannot open `%s': %s"), os_dev,
		      strerror (errno));
	  return xstrdup (os_dev);
	}
      if (ioctl (fd, DIOCGWEDGEINFO, &dkw) == -1)
	{
	  holy_error (holy_ERR_BAD_DEVICE,
		      "cannot get disk wedge info of `%s'", os_dev);
	  close (fd);
	  return xstrdup (os_dev);
	}
      *is_part = (dkw.dkw_offset != 0);
      close (fd);
      return xasprintf ("/dev/r%s%c", dkw.dkw_parent, 'a' + rawpart);
    }
#endif

  /* NetBSD (disk label) partitions are of the form "/dev/r[a-z]+[0-9][a-z]".  */
  if (strncmp ("/dev/r", os_dev, sizeof("/dev/r") - 1) == 0 &&
      (os_dev[sizeof("/dev/r") - 1] >= 'a' && os_dev[sizeof("/dev/r") - 1] <= 'z') &&
      strncmp ("fd", os_dev + sizeof("/dev/r") - 1, sizeof("fd") - 1) != 0)    /* not a floppy device name */
    {
      char *path = xstrdup (os_dev);
      char *p;
      for (p = path + sizeof("/dev/r"); *p >= 'a' && *p <= 'z'; p++);
      if (holy_isdigit(*p))
	{
	  p++;
	  if ((*p >= 'a' && *p <= 'z') && (*(p+1) == '\0'))
	    {
	      if (*p != 'a' + rawpart)
		*is_part = 1;
	      /* path matches the required regular expression and
		 p points to its last character.  */
	      *p = 'a' + rawpart;
	    }
	}
      return path;
    }

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
#  if defined(__NetBSD__)
  struct dkwedge_info dkw;
#  endif /* defined(__NetBSD__) */
  struct disklabel label;
  int p_index;

  fd = holy_util_fd_open (dev, O_RDONLY);
  if (fd == -1)
    {
      holy_error (holy_ERR_BAD_DEVICE, N_("cannot open `%s': %s"),
		  dev, strerror (errno));
      return 0;
    }

#  if defined(__NetBSD__)
  /* First handle the case of disk wedges.  */
  if (ioctl (fd, DIOCGWEDGEINFO, &dkw) == 0)
    {
      close (fd);
      return (holy_disk_addr_t) dkw.dkw_offset;
    }
#  endif /* defined(__NetBSD__) */

  if (ioctl (fd, DIOCGDINFO, &label) == -1)
    {
      holy_error (holy_ERR_BAD_DEVICE,
		  "cannot get disk label of `%s'", dev);
      close (fd);
      return 0;
    }

  close (fd);

  if (dev[0])
    p_index = dev[strlen(dev) - 1] - 'a';
  else
    p_index = -1;
  
  if (p_index >= label.d_npartitions || p_index < 0)
    {
      holy_error (holy_ERR_BAD_DEVICE,
		  "no disk label entry for `%s'", dev);
      return 0;
    }
  return (holy_disk_addr_t) label.d_partitions[p_index].p_offset;
}
