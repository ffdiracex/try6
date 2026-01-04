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

#ifdef __linux__
# include <sys/ioctl.h>         /* ioctl */
# include <sys/mount.h>
# ifndef BLKFLSBUF
#  define BLKFLSBUF     _IO (0x12,97)   /* flush buffer cache */
# endif /* ! BLKFLSBUF */
#endif /* __linux__ */

static struct
{
  char *drive;
  char *device;
  int device_map;
} map[256];

static int
unescape_cmp (const char *a, const char *b_escaped)
{
  while (*a || *b_escaped)
    {
      if (*b_escaped == '\\' && b_escaped[1] != 0)
	b_escaped++;
      if (*a < *b_escaped)
	return -1;
      if (*a > *b_escaped)
	return +1;
      a++;
      b_escaped++;
    }
  if (*a)
    return +1;
  if (*b_escaped)
    return -1;
  return 0;
}

static int
find_holy_drive (const char *name)
{
  unsigned int i;

  if (name)
    {
      for (i = 0; i < ARRAY_SIZE (map); i++)
	if (map[i].drive && unescape_cmp (map[i].drive, name) == 0)
	  return i;
    }

  return -1;
}

static int
find_free_slot (void)
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE (map); i++)
    if (! map[i].drive)
      return i;

  return -1;
}

static int
holy_util_biosdisk_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data,
			    holy_disk_pull_t pull)
{
  unsigned i;

  if (pull != holy_DISK_PULL_NONE)
    return 0;

  for (i = 0; i < ARRAY_SIZE (map); i++)
    if (map[i].drive && hook (map[i].drive, hook_data))
      return 1;

  return 0;
}

static holy_err_t
holy_util_biosdisk_open (const char *name, holy_disk_t disk)
{
  int drive;
  struct holy_util_hostdisk_data *data;

  drive = find_holy_drive (name);
  holy_util_info ("drive = %d", drive);
  if (drive < 0)
    return holy_error (holy_ERR_UNKNOWN_DEVICE,
		       "no mapping exists for `%s'", name);

  disk->id = drive;
  disk->data = data = xmalloc (sizeof (struct holy_util_hostdisk_data));
  data->dev = NULL;
  data->access_mode = 0;
  data->fd = holy_UTIL_FD_INVALID;
  data->is_disk = 0;
  data->device_map = map[drive].device_map;

  /* Get the size.  */
  {
    holy_util_fd_t fd;

    fd = holy_util_fd_open (map[drive].device, holy_UTIL_FD_O_RDONLY);

    if (!holy_UTIL_FD_IS_VALID(fd))
      return holy_error (holy_ERR_UNKNOWN_DEVICE, N_("cannot open `%s': %s"),
			 map[drive].device, holy_util_fd_strerror ());

    disk->total_sectors = holy_util_get_fd_size (fd, map[drive].device,
						 &disk->log_sector_size);
    disk->total_sectors >>= disk->log_sector_size;
    disk->max_agglomerate = holy_DISK_MAX_MAX_AGGLOMERATE;

#if holy_UTIL_FD_STAT_IS_FUNCTIONAL
    {
      struct stat st;
# if holy_DISK_DEVS_ARE_CHAR
      if (fstat (fd, &st) >= 0 && S_ISCHR (st.st_mode))
# else
      if (fstat (fd, &st) >= 0 && S_ISBLK (st.st_mode))
# endif
	data->is_disk = 1;
    }
#endif

    holy_util_fd_close (fd);

    holy_util_info ("the size of %s is %" holy_HOST_PRIuLONG_LONG,
		    name, (unsigned long long) disk->total_sectors);

    return holy_ERR_NONE;
  }
}

const char *
holy_hostdisk_os_dev_to_holy_drive (const char *os_disk, int add)
{
  unsigned int i;
  char *canon;

  canon = holy_canonicalize_file_name (os_disk);
  if (!canon)
    canon = xstrdup (os_disk);

  for (i = 0; i < ARRAY_SIZE (map); i++)
    if (! map[i].device)
      break;
    else if (strcmp (map[i].device, canon) == 0)
      {
	free (canon);
	return map[i].drive;
      }

  if (!add)
    {
      free (canon);
      return NULL;
    }

  if (i == ARRAY_SIZE (map))
    /* TRANSLATORS: it refers to the lack of free slots.  */
    holy_util_error ("%s", _("device count exceeds limit"));

  map[i].device = canon;
  map[i].drive = xmalloc (sizeof ("hostdisk/") + strlen (os_disk));
  strcpy (map[i].drive, "hostdisk/");
  strcpy (map[i].drive + sizeof ("hostdisk/") - 1, os_disk);
  map[i].device_map = 0;

  holy_hostdisk_flush_initial_buffer (os_disk);

  return map[i].drive;
}

#ifndef __linux__
holy_util_fd_t
holy_util_fd_open_device (const holy_disk_t disk, holy_disk_addr_t sector, int flags,
			  holy_disk_addr_t *max)
{
  holy_util_fd_t fd;
  struct holy_util_hostdisk_data *data = disk->data;

  *max = ~0ULL;

  flags |= holy_UTIL_FD_O_SYNC;

  if (data->dev && strcmp (data->dev, map[disk->id].device) == 0 &&
      data->access_mode == (flags & O_ACCMODE))
    {
      holy_dprintf ("hostdisk", "reusing open device `%s'\n", data->dev);
      fd = data->fd;
    }
  else
    {
      free (data->dev);
      data->dev = 0;
      if (holy_UTIL_FD_IS_VALID(data->fd))
	{
	    if (data->access_mode == O_RDWR || data->access_mode == O_WRONLY)
	      holy_util_fd_sync (data->fd);
	    holy_util_fd_close (data->fd);
	    data->fd = holy_UTIL_FD_INVALID;
	}

      fd = holy_util_fd_open (map[disk->id].device, flags);
      if (holy_UTIL_FD_IS_VALID(fd))
	{
	  data->dev = xstrdup (map[disk->id].device);
	  data->access_mode = (flags & O_ACCMODE);
	  data->fd = fd;
	}
    }

  if (!holy_UTIL_FD_IS_VALID(data->fd))
    {
      holy_error (holy_ERR_BAD_DEVICE, N_("cannot open `%s': %s"),
		  map[disk->id].device, holy_util_fd_strerror ());
      return holy_UTIL_FD_INVALID;
    }

  if (holy_util_fd_seek (fd, sector << disk->log_sector_size))
    {
      holy_util_fd_close (fd);
      holy_error (holy_ERR_BAD_DEVICE, N_("cannot seek `%s': %s"),
		  map[disk->id].device, holy_util_fd_strerror ());

      return holy_UTIL_FD_INVALID;
    }

  return fd;
}
#endif


static holy_err_t
holy_util_biosdisk_read (holy_disk_t disk, holy_disk_addr_t sector,
			 holy_size_t size, char *buf)
{
  while (size)
    {
      holy_util_fd_t fd;
      holy_disk_addr_t max = ~0ULL;
      fd = holy_util_fd_open_device (disk, sector, holy_UTIL_FD_O_RDONLY, &max);
      if (!holy_UTIL_FD_IS_VALID (fd))
	return holy_errno;

#ifdef __linux__
      if (sector == 0)
	/* Work around a bug in Linux ez remapping.  Linux remaps all
	   sectors that are read together with the MBR in one read.  It
	   should only remap the MBR, so we split the read in two
	   parts. -jochen  */
	max = 1;
#endif /* __linux__ */

      if (max > size)
	max = size;

      if (holy_util_fd_read (fd, buf, max << disk->log_sector_size)
	  != (ssize_t) (max << disk->log_sector_size))
	return holy_error (holy_ERR_READ_ERROR, N_("cannot read `%s': %s"),
			   map[disk->id].device, holy_util_fd_strerror ());
      size -= max;
      buf += (max << disk->log_sector_size);
      sector += max;
    }
  return holy_ERR_NONE;
}

static holy_err_t
holy_util_biosdisk_write (holy_disk_t disk, holy_disk_addr_t sector,
			  holy_size_t size, const char *buf)
{
  while (size)
    {
      holy_util_fd_t fd;
      holy_disk_addr_t max = ~0ULL;
      fd = holy_util_fd_open_device (disk, sector, holy_UTIL_FD_O_WRONLY, &max);
      if (!holy_UTIL_FD_IS_VALID (fd))
	return holy_errno;

#ifdef __linux__
      if (sector == 0)
	/* Work around a bug in Linux ez remapping.  Linux remaps all
	   sectors that are write together with the MBR in one write.  It
	   should only remap the MBR, so we split the write in two
	   parts. -jochen  */
	max = 1;
#endif /* __linux__ */

      if (max > size)
	max = size;

      if (holy_util_fd_write (fd, buf, max << disk->log_sector_size)
	  != (ssize_t) (max << disk->log_sector_size))
	return holy_error (holy_ERR_WRITE_ERROR, N_("cannot write to `%s': %s"),
			   map[disk->id].device, holy_util_fd_strerror ());
      size -= max;
      buf += (max << disk->log_sector_size);
    }
  return holy_ERR_NONE;
}

holy_err_t
holy_util_biosdisk_flush (struct holy_disk *disk)
{
  struct holy_util_hostdisk_data *data = disk->data;

  if (disk->dev->id != holy_DISK_DEVICE_BIOSDISK_ID)
    return holy_ERR_NONE;
  if (!holy_UTIL_FD_IS_VALID (data->fd))
    {
      holy_disk_addr_t max;
      data->fd = holy_util_fd_open_device (disk, 0, holy_UTIL_FD_O_RDONLY, &max);
      if (!holy_UTIL_FD_IS_VALID (data->fd))
	return holy_errno;
    }
  holy_util_fd_sync (data->fd);
#ifdef __linux__
  if (data->is_disk)
    ioctl (data->fd, BLKFLSBUF, 0);
#endif
  return holy_ERR_NONE;
}

static void
holy_util_biosdisk_close (struct holy_disk *disk)
{
  struct holy_util_hostdisk_data *data = disk->data;

  free (data->dev);
  if (holy_UTIL_FD_IS_VALID (data->fd))
    {
      if (data->access_mode == O_RDWR || data->access_mode == O_WRONLY)
	holy_util_biosdisk_flush (disk);
      holy_util_fd_close (data->fd);
    }
  free (data);
}

static struct holy_disk_dev holy_util_biosdisk_dev =
  {
    .name = "hostdisk",
    .id = holy_DISK_DEVICE_HOSTDISK_ID,
    .iterate = holy_util_biosdisk_iterate,
    .open = holy_util_biosdisk_open,
    .close = holy_util_biosdisk_close,
    .read = holy_util_biosdisk_read,
    .write = holy_util_biosdisk_write,
    .next = 0
  };

static int
holy_util_check_file_presence (const char *p)
{
#if !holy_UTIL_FD_STAT_IS_FUNCTIONAL
  holy_util_fd_t h;
  h = holy_util_fd_open (p, holy_UTIL_FD_O_RDONLY);
  if (!holy_UTIL_FD_IS_VALID(h))
    return 0;
  holy_util_fd_close (h);
  return 1;
#else
  struct stat st;

  if (stat (p, &st) == -1)
    return 0;
  return 1;
#endif
}

static void
read_device_map (const char *dev_map)
{
  FILE *fp;
  char buf[1024];	/* XXX */
  int lineno = 0;

  if (!dev_map || dev_map[0] == '\0')
    {
      holy_util_info ("no device.map");
      return;
    }

  fp = holy_util_fopen (dev_map, "r");
  if (! fp)
    {
      holy_util_info (_("cannot open `%s': %s"), dev_map, strerror (errno));
      return;
    }

  while (fgets (buf, sizeof (buf), fp))
    {
      char *p = buf;
      char *e;
      char *drive_e, *drive_p;
      int drive;

      lineno++;

      /* Skip leading spaces.  */
      while (*p && holy_isspace (*p))
	p++;

      /* If the first character is `#' or NUL, skip this line.  */
      if (*p == '\0' || *p == '#')
	continue;

      if (*p != '(')
	{
	  char *tmp;
	  tmp = xasprintf (_("missing `%c' symbol"), '(');
	  holy_util_error ("%s:%d: %s", dev_map, lineno, tmp);
	}

      p++;
      /* Find a free slot.  */
      drive = find_free_slot ();
      if (drive < 0)
	holy_util_error ("%s:%d: %s", dev_map, lineno, _("device count exceeds limit"));

      e = p;
      p = strchr (p, ')');
      if (! p)
	{
	  char *tmp;
	  tmp = xasprintf (_("missing `%c' symbol"), ')');
	  holy_util_error ("%s:%d: %s", dev_map, lineno, tmp);
	}

      map[drive].drive = 0;
      if ((e[0] == 'f' || e[0] == 'h' || e[0] == 'c') && e[1] == 'd')
	{
	  char *ptr;
	  for (ptr = e + 2; ptr < p; ptr++)
	    if (!holy_isdigit (*ptr))
	      break;
	  if (ptr == p)
	    {
	      map[drive].drive = xmalloc (p - e + sizeof ('\0'));
	      strncpy (map[drive].drive, e, p - e + sizeof ('\0'));
	      map[drive].drive[p - e] = '\0';
	    }
	  if (*ptr == ',')
	    {
	      *p = 0;

	      /* TRANSLATORS: Only one entry is ignored. However the suggestion
		 is to correct/delete the whole file.
		 device.map is a file indicating which
		 devices are available at boot time. Fedora populated it with
		 entries like (hd0,1) /dev/sda1 which would mean that every
		 partition is a separate disk for BIOS. Such entries were
		 inactive in holy due to its bug which is now gone. Without
		 this additional check these entries would be harmful now.
	      */
	      holy_util_warn (_("the device.map entry `%s' is invalid. "
				"Ignoring it. Please correct or "
				"delete your device.map"), e);
	      continue;
	    }
	}
      drive_e = e;
      drive_p = p;
      map[drive].device_map = 1;

      p++;
      /* Skip leading spaces.  */
      while (*p && holy_isspace (*p))
	p++;

      if (*p == '\0')
	holy_util_error ("%s:%d: %s", dev_map, lineno, _("filename expected"));

      /* NUL-terminate the filename.  */
      e = p;
      while (*e && ! holy_isspace (*e))
	e++;
      *e = '\0';

      if (!holy_util_check_file_presence (p))
	{
	  free (map[drive].drive);
	  map[drive].drive = NULL;
	  holy_util_info ("Cannot stat `%s', skipping", p);
	  continue;
	}

      /* On Linux, the devfs uses symbolic links horribly, and that
	 confuses the interface very much, so use realpath to expand
	 symbolic links.  */
      map[drive].device = holy_canonicalize_file_name (p);
      if (! map[drive].device)
	map[drive].device = xstrdup (p);
      
      if (!map[drive].drive)
	{
	  char c;
	  map[drive].drive = xmalloc (sizeof ("hostdisk/") + strlen (p));
	  memcpy (map[drive].drive, "hostdisk/", sizeof ("hostdisk/") - 1);
	  strcpy (map[drive].drive + sizeof ("hostdisk/") - 1, p);
	  c = *drive_p;
	  *drive_p = 0;
	  /* TRANSLATORS: device.map is a filename. Not to be translated.
	     device.map specifies disk correspondance overrides. Previously
	     one could create any kind of device name with this. Due to
	     some problems we decided to limit it to just a handful
	     possibilities.  */
	  holy_util_warn (_("the drive name `%s' in device.map is incorrect. "
			    "Using %s instead. "
			    "Please use the form [hfc]d[0-9]* "
			    "(E.g. `hd0' or `cd')"),
			  drive_e, map[drive].drive);
	  *drive_p = c;
	}

      holy_util_info ("adding `%s' -> `%s' from device.map", map[drive].drive,
		      map[drive].device);

      holy_hostdisk_flush_initial_buffer (map[drive].device);
    }

  fclose (fp);
}

void
holy_util_biosdisk_init (const char *dev_map)
{
  read_device_map (dev_map);
  holy_disk_dev_register (&holy_util_biosdisk_dev);
}

void
holy_util_biosdisk_fini (void)
{
  unsigned i;

  for (i = 0; i < ARRAY_SIZE(map); i++)
    {
      if (map[i].drive)
	free (map[i].drive);
      if (map[i].device)
	free (map[i].device);
      map[i].drive = map[i].device = NULL;
    }

  holy_disk_dev_unregister (&holy_util_biosdisk_dev);
}

const char *
holy_util_biosdisk_get_compatibility_hint (holy_disk_t disk)
{
  if (disk->dev != &holy_util_biosdisk_dev || map[disk->id].device_map)
    return disk->name;
  return 0;
}

const char *
holy_util_biosdisk_get_osdev (holy_disk_t disk)
{
  if (disk->dev != &holy_util_biosdisk_dev)
    return 0;

  return map[disk->id].device;
}


static char *
holy_util_path_concat_real (size_t n, int ext, va_list ap)
{
  size_t totlen = 0;
  char **l = xmalloc ((n + ext) * sizeof (l[0]));
  char *r, *p, *pi;
  size_t i;
  int first = 1;

  for (i = 0; i < n + ext; i++)
    {
      l[i] = va_arg (ap, char *);
      if (l[i])
	totlen += strlen (l[i]) + 1;
    }

  r = xmalloc (totlen + 10);

  p = r;
  for (i = 0; i < n; i++)
    {
      pi = l[i];
      if (!pi)
	continue;
      while (*pi == '/')
	pi++;
      if ((p != r || (pi != l[i] && first)) && (p == r || *(p - 1) != '/'))
	*p++ = '/';
      first = 0;
      p = holy_stpcpy (p, pi);
      while (p != r && p != r + 1 && *(p - 1) == '/')
	p--;
    }

  if (ext && l[i])
    p = holy_stpcpy (p, l[i]);

  *p = '\0';

  free (l);

  return r;
}

char *
holy_util_path_concat (size_t n, ...)
{
  va_list ap;
  char *r;

  va_start (ap, n);

  r = holy_util_path_concat_real (n, 0, ap);

  va_end (ap);

  return r;
}

char *
holy_util_path_concat_ext (size_t n, ...)
{
  va_list ap;
  char *r;

  va_start (ap, n);

  r = holy_util_path_concat_real (n, 1, ap);

  va_end (ap);

  return r;
}
