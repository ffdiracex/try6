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
#include <holy/util/misc.h>

#include <holy/cryptodisk.h>
#include <holy/i18n.h>

#ifdef __linux__
#include <sys/ioctl.h>         /* ioctl */
#include <sys/mount.h>
#endif

#include <sys/types.h>

#if defined(HAVE_LIBZFS) && defined(HAVE_LIBNVPAIR)
# include <holy/util/libzfs.h>
# include <holy/util/libnvpair.h>
#endif

#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/emu/misc.h>
#include <holy/emu/hostdisk.h>
#include <holy/emu/getroot.h>

#if defined (__FreeBSD__) || defined (__FreeBSD_kernel__)
#include <sys/mount.h>
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__)
# include <sys/ioctl.h>
# include <sys/disklabel.h>    /* struct disklabel */
# include <sys/disk.h>    /* struct dkwedge_info */
#include <sys/param.h>
#include <sys/mount.h>
#endif /* defined(__NetBSD__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) */

#if defined(__NetBSD__)
# include <sys/fdio.h>
#endif

holy_disk_addr_t
holy_util_find_partition_start (const char *dev)
{
#if holy_UTIL_FD_STAT_IS_FUNCTIONAL
  struct stat st;
  holy_disk_addr_t partition_start;

  if (stat (dev, &st) >= 0
      && holy_util_device_is_mapped_stat (&st)
      && holy_util_get_dm_node_linear_info (st.st_rdev, 0, 0, &partition_start))
    return partition_start;
#endif

  return holy_util_find_partition_start_os (dev);
}

void
holy_util_pull_device (const char *os_dev)
{
  enum holy_dev_abstraction_types ab;
  ab = holy_util_get_dev_abstraction (os_dev);
  switch (ab)
    {
    case holy_DEV_ABSTRACTION_LVM:
      holy_util_pull_lvm_by_command (os_dev);
      /* Fallthrough - in case that lvm-tools are unavailable.  */
    case holy_DEV_ABSTRACTION_LUKS:
      holy_util_pull_devmapper (os_dev);
      return;

    default:
      if (holy_util_pull_device_os (os_dev, ab))
	return;
      /* Fallthrough.  */
    case holy_DEV_ABSTRACTION_NONE:
      free (holy_util_biosdisk_get_holy_dev (os_dev));
      return;
    }
}

char *
holy_util_get_holy_dev (const char *os_dev)
{
  char *ret;

  holy_util_pull_device (os_dev);

  ret = holy_util_get_devmapper_holy_dev (os_dev);
  if (ret)
    return ret;
  ret = holy_util_get_holy_dev_os (os_dev);
  if (ret)
    return ret;
  return holy_util_biosdisk_get_holy_dev (os_dev);
}

int
holy_util_get_dev_abstraction (const char *os_dev)
{
  enum holy_dev_abstraction_types ret;

  /* User explicitly claims that this drive is visible by BIOS.  */
  if (holy_util_biosdisk_is_present (os_dev))
    return holy_DEV_ABSTRACTION_NONE;

  /* Check for LVM and LUKS.  */
  ret = holy_util_get_dm_abstraction (os_dev);

  if (ret != holy_DEV_ABSTRACTION_NONE)
    return ret;

  return holy_util_get_dev_abstraction_os (os_dev);
}

static char *
convert_system_partition_to_system_disk (const char *os_dev, int *is_part)
{
#if holy_UTIL_FD_STAT_IS_FUNCTIONAL
  struct stat st;

  if (stat (os_dev, &st) < 0)
    {
      const char *errstr = strerror (errno); 
      holy_error (holy_ERR_BAD_DEVICE, N_("cannot stat `%s': %s"),
		  os_dev, errstr);
      holy_util_info (_("cannot stat `%s': %s"), os_dev, errstr);
      return 0;
    }

  *is_part = 0;

  if (holy_util_device_is_mapped_stat (&st))
    return holy_util_devmapper_part_to_disk (&st, is_part, os_dev);

  *is_part = 0;

  return holy_util_part_to_disk (os_dev, &st, is_part);
#else
  *is_part = 0;

  return holy_util_part_to_disk (os_dev, NULL, is_part);
#endif
}

static const char *
find_system_device (const char *os_dev)
{
  char *os_disk;
  const char *drive;
  int is_part;

  os_disk = convert_system_partition_to_system_disk (os_dev, &is_part);
  if (! os_disk)
    return NULL;

  drive = holy_hostdisk_os_dev_to_holy_drive (os_disk, 0);
  free (os_disk);
  return drive;
}

static char *
make_device_name (const char *drive)
{
  char *ret, *ptr;
  const char *iptr;

  ret = xmalloc (strlen (drive) * 2);
  ptr = ret;
  for (iptr = drive; *iptr; iptr++)
    {
      if (*iptr == ',' || *iptr == '\\')
	*ptr++ = '\\';
      *ptr++ = *iptr;
    }
  *ptr = 0;

  return ret;
}

char *
holy_util_get_os_disk (const char *os_dev)
{
  int is_part;

  holy_util_info ("Looking for %s", os_dev);

  return convert_system_partition_to_system_disk (os_dev, &is_part);
}

#if !defined(__APPLE__)
/* Context for holy_util_biosdisk_get_holy_dev.  */
struct holy_util_biosdisk_get_holy_dev_ctx
{
  char *partname;
  holy_disk_addr_t start;
};

/* Helper for holy_util_biosdisk_get_holy_dev.  */
static int
find_partition (holy_disk_t dsk __attribute__ ((unused)),
		const holy_partition_t partition, void *data)
{
  struct holy_util_biosdisk_get_holy_dev_ctx *ctx = data;
  holy_disk_addr_t part_start = 0;
  holy_util_info ("Partition %d starts from %" holy_HOST_PRIuLONG_LONG,
		  partition->number, (unsigned long long) partition->start);

  part_start = holy_partition_get_start (partition);

  if (ctx->start == part_start)
    {
      ctx->partname = holy_partition_get_name (partition);
      return 1;
    }

  return 0;
}
#endif

char *
holy_util_biosdisk_get_holy_dev (const char *os_dev)
{
  const char *drive;
  char *sys_disk;
  int is_part;

  holy_util_info ("Looking for %s", os_dev);

  sys_disk = convert_system_partition_to_system_disk (os_dev, &is_part);

  if (!sys_disk)
    return 0;

  drive = holy_hostdisk_os_dev_to_holy_drive (sys_disk, 1);

  holy_util_info ("%s is a parent of %s", sys_disk, os_dev);
  if (!is_part)
    {
      free (sys_disk);
      return make_device_name (drive);
    }
  free (sys_disk);

#if defined(__APPLE__)
  /* Apple uses "/dev/r?disk[0-9]+(s[0-9]+)?".  */
  /*
   * Note: we do not use the new partition naming scheme as dos_part does not
   * necessarily correspond to an msdos partition.
   */
  {
    const char *p;
    char *dri, *ret;
    int part;
    int disk = (holy_memcmp (os_dev, "/dev/disk", sizeof ("/dev/disk") - 1)
		 == 0);
    int rdisk = (holy_memcmp (os_dev, "/dev/rdisk", sizeof ("/dev/rdisk") - 1)
		 == 0);

    dri = make_device_name (drive);
 
    if (!disk && !rdisk)
      return dri;

    p = os_dev + sizeof ("/dev/disk") + rdisk - 1;
    while (*p >= '0' && *p <= '9')
      p++;
    if (*p != 's')
      return dri;
    p++;

    part = strtol (p, NULL, 10);
    if (part == 0)
      return dri;

    ret = xasprintf ("%s,%d", dri, part);
    free (dri);

    return ret;
  }

#else

  /* Linux counts partitions uniformly, whether a BSD partition or a DOS
     partition, so mapping them to holy devices is not trivial.
     Here, get the start sector of a partition by HDIO_GETGEO, and
     compare it with each partition holy recognizes.

     Cygwin /dev/sdXN emulation uses Windows partition mapping. It
     does not count the extended partition and missing primary
     partitions.  Use same method as on Linux here.

     For NetBSD and FreeBSD, proceed as for Linux, except that the start
     sector is obtained from the disk label.  */
  {
    char *name;
    holy_disk_t disk;
    struct holy_util_biosdisk_get_holy_dev_ctx ctx;

    name = make_device_name (drive);

    ctx.start = holy_util_find_partition_start (os_dev);
    if (holy_errno != holy_ERR_NONE)
      {
	free (name);
	return 0;
      }

#if defined(__GNU__)
    /* Some versions of Hurd use badly glued Linux code to handle partitions
       resulting in partitions being promoted to disks.  */
    /* GNU uses "/dev/[hs]d[0-9]+(s[0-9]+[a-z]?)?".  */
    /*
     * Note: we do not use the new partition naming scheme as dos_part does not
     * necessarily correspond to an msdos partition.
     */
    if (ctx.start == (holy_disk_addr_t) -1)
      {
	char *p;
	char *dri;

	dri = make_device_name (drive);

	p = strrchr (os_dev + sizeof ("/dev/hd") - 1, 's');
	if (p)
	  {
	    long int n;
	    char *q;

	    p++;
	    n = strtol (p, &q, 10);
	    if (p != q && n > 0 && n != holy_LONG_MAX)
	      {
		char *t;
		t = dri;
		if (*q >= 'a' && *q <= 'g')
		  dri = xasprintf ("%s,%ld,%d", t, n, *q - 'a' + 1);
		else
		  dri = xasprintf ("%s,%ld", t, n);
		free (t);
	      }
	  }

	return dri;
      }
#endif

    holy_util_info ("%s starts from %" holy_HOST_PRIuLONG_LONG,
		    os_dev, (unsigned long long) ctx.start);

    holy_util_info ("opening the device %s", name);
    disk = holy_disk_open (name);
    free (name);

    if (! disk)
      {
	/* We already know that the partition exists.  Given that we already
	   checked the device map above, we can only get
	   holy_ERR_UNKNOWN_DEVICE at this point if the disk does not exist.
	   This can happen on Xen, where disk images in the host can be
	   assigned to devices that have partition-like names in the guest
	   but are really more like disks.  */
	if (holy_errno == holy_ERR_UNKNOWN_DEVICE)
	  {
	    char *canon;
	    holy_util_warn
	      (_("disk does not exist, so falling back to partition device %s"),
	       os_dev);
	    holy_errno = holy_ERR_NONE;

	    canon = holy_canonicalize_file_name (os_dev);
	    drive = holy_hostdisk_os_dev_to_holy_drive (canon ? : os_dev, 1);
	    if (canon)
	      free (canon);
	    return make_device_name (drive);
	  }
	else
	  return 0;
      }

    name = holy_util_get_ldm (disk, ctx.start);
    if (name)
      {
	holy_disk_close (disk);
	return name;
      }

    ctx.partname = NULL;

    holy_partition_iterate (disk, find_partition, &ctx);
    if (holy_errno != holy_ERR_NONE)
      {
	holy_disk_close (disk);
	return 0;
      }

    if (ctx.partname == NULL)
      {
	holy_disk_close (disk);
	holy_util_info ("cannot find the partition of `%s'", os_dev);
	holy_error (holy_ERR_BAD_DEVICE,
		    "cannot find the partition of `%s'", os_dev);
	return 0;
      }

    name = holy_xasprintf ("%s,%s", disk->name, ctx.partname);
    free (ctx.partname);
    holy_disk_close (disk);
    return name;
  }

#endif
}

int
holy_util_biosdisk_is_present (const char *os_dev)
{
  int ret = (find_system_device (os_dev) != NULL);
  holy_util_info ((ret ? "%s is present" : "%s is not present"),
		  os_dev);
  return ret;
}

#ifdef HAVE_LIBZFS
static libzfs_handle_t *__libzfs_handle;

static void
fini_libzfs (void)
{
  libzfs_fini (__libzfs_handle);
}

libzfs_handle_t *
holy_get_libzfs_handle (void)
{
  if (! __libzfs_handle)
    {
      __libzfs_handle = libzfs_init ();

      if (__libzfs_handle)
	atexit (fini_libzfs);
    }

  return __libzfs_handle;
}
#endif /* HAVE_LIBZFS */

