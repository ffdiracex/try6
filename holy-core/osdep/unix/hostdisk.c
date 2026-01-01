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

#if !defined (__CYGWIN__) && !defined (__MINGW32__) && !defined (__AROS__)

#ifdef __linux__
# include <sys/ioctl.h>         /* ioctl */
# include <sys/mount.h>
#endif /* __linux__ */

holy_uint64_t
holy_util_get_fd_size (holy_util_fd_t fd, const char *name, unsigned *log_secsize)
{
  struct stat st;
  holy_int64_t ret = -1;

  if (fstat (fd, &st) < 0)
    /* TRANSLATORS: "stat" comes from the name of POSIX function.  */
    holy_util_error (_("cannot stat `%s': %s"), name, strerror (errno));
#if holy_DISK_DEVS_ARE_CHAR
  if (S_ISCHR (st.st_mode))
#else
  if (S_ISBLK (st.st_mode))
#endif
    ret = holy_util_get_fd_size_os (fd, name, log_secsize);
  if (ret != -1LL)
    return ret;

  if (log_secsize)
    *log_secsize = 9;

  return st.st_size;
}

int
holy_util_fd_seek (holy_util_fd_t fd, holy_uint64_t off)
{
  off_t offset = (off_t) off;

  if (lseek (fd, offset, SEEK_SET) != offset)
    return -1;

  return 0;
}


/* Read LEN bytes from FD in BUF. Return less than or equal to zero if an
   error occurs, otherwise return LEN.  */
ssize_t
holy_util_fd_read (holy_util_fd_t fd, char *buf, size_t len)
{
  ssize_t size = 0;

  while (len)
    {
      ssize_t ret = read (fd, buf, len);

      if (ret == 0)
	break;

      if (ret < 0)
        {
          if (errno == EINTR)
            continue;
          else
            return ret;
        }

      len -= ret;
      buf += ret;
      size += ret;
    }

  return size;
}

/* Write LEN bytes from BUF to FD. Return less than or equal to zero if an
   error occurs, otherwise return LEN.  */
ssize_t
holy_util_fd_write (holy_util_fd_t fd, const char *buf, size_t len)
{
  ssize_t size = 0;

  while (len)
    {
      ssize_t ret = write (fd, buf, len);

      if (ret == 0)
	break;

      if (ret < 0)
        {
          if (errno == EINTR)
            continue;
          else
            return ret;
        }

      len -= ret;
      buf += ret;
      size += ret;
    }

  return size;
}

#if !defined (__NetBSD__) && !defined (__APPLE__) && !defined (__FreeBSD__) && !defined(__FreeBSD_kernel__)
holy_util_fd_t
holy_util_fd_open (const char *os_dev, int flags)
{
#ifdef O_LARGEFILE
  flags |= O_LARGEFILE;
#endif
#ifdef O_BINARY
  flags |= O_BINARY;
#endif

  return open (os_dev, flags, S_IROTH | S_IRGRP | S_IRUSR | S_IWUSR);
}
#endif

const char *
holy_util_fd_strerror (void)
{
  return strerror (errno);
}

static int allow_fd_syncs = 1;

void
holy_util_fd_sync (holy_util_fd_t fd)
{
  if (allow_fd_syncs)
    fsync (fd);
}

void
holy_util_file_sync (FILE *f)
{
  fflush (f);
  if (!allow_fd_syncs)
    return;
  fsync (fileno (f));
}

void
holy_util_disable_fd_syncs (void)
{
  allow_fd_syncs = 0;
}

void
holy_util_fd_close (holy_util_fd_t fd)
{
  close (fd);
}

char *
holy_canonicalize_file_name (const char *path)
{
#if defined (PATH_MAX)
  char *ret;

  ret = xmalloc (PATH_MAX);
  if (!realpath (path, ret))
    return NULL;
  return ret;
#else
  return realpath (path, NULL);
#endif
}

FILE *
holy_util_fopen (const char *path, const char *mode)
{
  return fopen (path, mode);
}

int
holy_util_is_directory (const char *path)
{
  struct stat st;

  if (stat (path, &st) == -1)
    return 0;

  return S_ISDIR (st.st_mode);
}

int
holy_util_is_regular (const char *path)
{
  struct stat st;

  if (stat (path, &st) == -1)
    return 0;

  return S_ISREG (st.st_mode);
}

holy_uint32_t
holy_util_get_mtime (const char *path)
{
  struct stat st;

  if (stat (path, &st) == -1)
    return 0;

  return st.st_mtime;
}

#endif

#if defined (__CYGWIN__) || (!defined (__MINGW32__) && !defined (__AROS__))

int
holy_util_is_special_file (const char *path)
{
  struct stat st;

  if (lstat (path, &st) == -1)
    return 1;
  return (!S_ISREG (st.st_mode) && !S_ISDIR (st.st_mode));
}


char *
holy_util_make_temporary_file (void)
{
  const char *t = getenv ("TMPDIR");
  size_t tl;
  char *tmp;
  if (!t)
    t = "/tmp";
  tl = strlen (t);
  tmp = xmalloc (tl + sizeof ("/holy.XXXXXX"));
  memcpy (tmp, t, tl);
  memcpy (tmp + tl, "/holy.XXXXXX",
	  sizeof ("/holy.XXXXXX"));
  if (mkstemp (tmp) == -1)
    holy_util_error (_("cannot make temporary file: %s"), strerror (errno));
  return tmp;
}

char *
holy_util_make_temporary_dir (void)
{
  const char *t = getenv ("TMPDIR");
  size_t tl;
  char *tmp;
  if (!t)
    t = "/tmp";
  tl = strlen (t);
  tmp = xmalloc (tl + sizeof ("/holy.XXXXXX"));
  memcpy (tmp, t, tl);
  memcpy (tmp + tl, "/holy.XXXXXX",
	  sizeof ("/holy.XXXXXX"));
  if (!mkdtemp (tmp))
    holy_util_error (_("cannot make temporary directory: %s"),
		     strerror (errno));
  return tmp;
}

#endif
