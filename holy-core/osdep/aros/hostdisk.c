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
#include <time.h>

#include <string.h>
#include <dos/dos.h>
#include <dos/filesystem.h>
#include <dos/exall.h>
#include <proto/dos.h>
#include <devices/hardblocks.h>
#include <devices/newstyle.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <proto/partition.h>
#include <devices/trackdisk.h>
#include <exec/errors.h>

#define BOUNCE_SIZE 1048576

static ULONG *bounce;

char *
holy_canonicalize_file_name (const char *path)
{
  char *ret;
  BPTR lck;

  if (path[0] == '/' && path[1] == '/' && path[2] == ':')
    return xstrdup (path);

  ret = xmalloc (2048);
  lck = Lock ((const unsigned char *) path, SHARED_LOCK);

  if (!lck || !NameFromLock (lck, (unsigned char *) ret, 2040))
    {
      free (ret);
      ret = xstrdup (path);
    }
  if (lck)
    UnLock (lck);

  return ret;
}

static holy_uint64_t
holy_util_get_fd_size_volume (holy_util_fd_t fd __attribute__ ((unused)),
			      const char *dev,
			      unsigned *log_secsize)
{
  struct DriveGeometry *geo;
  LONG err;
  unsigned sector_size, log_sector_size;

  if (!bounce)
    bounce = AllocVec (BOUNCE_SIZE, MEMF_PUBLIC | MEMF_CLEAR);
  if (!bounce)
    holy_util_error ("out of memory");

  fd->ioreq->iotd_Req.io_Command = TD_GETGEOMETRY;
  fd->ioreq->iotd_Req.io_Length = sizeof (*geo);
  fd->ioreq->iotd_Req.io_Data = bounce;
  fd->ioreq->iotd_Req.io_Offset = 0;
  fd->ioreq->iotd_Req.io_Actual = 0;
  err = DoIO ((struct IORequest *) fd->ioreq);
  if (err)
    {
      holy_util_info ("I/O failed with error %d, IoErr=%d", (int)err, (int) IoErr ());
      return -1;
    }

  geo = (struct DriveGeometry *) bounce;

  sector_size = geo->dg_SectorSize;

  if (sector_size & (sector_size - 1) || !sector_size)
    return -1;

  for (log_sector_size = 0;
       (1 << log_sector_size) < sector_size;
       log_sector_size++);

  if (log_secsize)
    *log_secsize = log_sector_size;

  return (holy_uint64_t) geo->dg_TotalSectors * (holy_uint64_t) geo->dg_SectorSize;
}

static holy_uint64_t
holy_util_get_fd_size_file (holy_util_fd_t fd,
			    const char *dev __attribute__ ((unused)),
			    unsigned *log_secsize)
{
  off_t oo, ro;
  *log_secsize = 9;
  /* FIXME: support 64-bit offsets.  */
  oo = lseek (fd->fd, 0, SEEK_CUR);
  ro = lseek (fd->fd, 0, SEEK_END);
  lseek (fd->fd, oo, SEEK_SET);
  return ro;
}

int
holy_util_fd_seek (holy_util_fd_t fd, holy_uint64_t off)
{
  switch (fd->type)
    {
    case holy_UTIL_FD_FILE:
      if (lseek (fd->fd, 0, SEEK_SET) == (off_t) -1)
	return -1;
      fd->off = off;
      return 0;
    case holy_UTIL_FD_DISK:
      fd->off = off;
      return 0;
    }

  return -1;
}

holy_util_fd_t
holy_util_fd_open (const char *dev, int flg)
{
  holy_util_fd_t ret = xmalloc (sizeof (*ret));
  const char *p1, *p2;
  const char *dname;
  char *tmp;
  IPTR unit = 0;
  ULONG flags = 0;

#ifdef O_LARGEFILE
  flg |= O_LARGEFILE;
#endif
#ifdef O_BINARY
  flg |= O_BINARY;
#endif

  ret->off = 0;

  if (dev[0] != '/' || dev[1] != '/' || dev[2] != ':')
    {
      ret->type = holy_UTIL_FD_FILE;
      ret->fd = open (dev, flg, S_IROTH | S_IRGRP | S_IRUSR | S_IWUSR);
      if (ret->fd < 0)
	{
	  free (ret);
	  return NULL;
	}
      return ret;
    }

  p1 = strchr (dev + 3, '/');
  if (!p1)
    p1 = dev + strlen (dev);
  else
    {
      unit = holy_strtoul (p1 + 1, (char **) &p2, 16);
      if (p2 && *p2 == '/')
	flags = holy_strtoul (p2 + 1, 0, 16);
    }

  ret->mp = CreateMsgPort();
  if (!ret->mp)
    {
      free (ret);
      return NULL;
    }
  ret->ioreq = (struct IOExtTD *) CreateIORequest(ret->mp,
						 sizeof(struct IOExtTD));
  if (!ret->ioreq)
    {
      free (ret);
      DeleteMsgPort (ret->mp);
      return NULL;
    }

  dname = dev + 3;
  ret->type = holy_UTIL_FD_DISK;

  tmp = xmalloc (p1 - dname + 1);
  memcpy (tmp, dname, p1 - dname);
  tmp[p1 - dname] = '\0';

  ret->is_floppy = (strcmp (tmp, TD_NAME) == 0);
  ret->is_64 = 1;

  if (OpenDevice ((unsigned char *) tmp, unit,
		  (struct IORequest *) ret->ioreq, flags))
    {
      free (tmp);
      free (ret);
      DeleteMsgPort (ret->mp);
      return NULL;
    }
  free (tmp);
  return ret;
}

static ssize_t
holy_util_fd_read_file (holy_util_fd_t fd, char *buf, size_t len)
{
  ssize_t size = len;

  while (len)
    {
      ssize_t ret = read (fd->fd, buf, len);

      if (ret <= 0)
        {
          if (errno == EINTR)
            continue;
          else
            return ret;
        }

      fd->off += ret;
      len -= ret;
      buf += ret;
    }

  return size;
}

/* Write LEN bytes from BUF to FD. Return less than or equal to zero if an
   error occurs, otherwise return LEN.  */
static ssize_t
holy_util_fd_write_file (holy_util_fd_t fd, const char *buf, size_t len)
{
  ssize_t size = len;

  while (len)
    {
      ssize_t ret = write (fd->fd, buf, len);

      if (ret <= 0)
        {
          if (errno == EINTR)
            continue;
          else
            return ret;
        }

      fd->off += ret;
      len -= ret;
      buf += ret;
    }

  return size;
}

static void
stop_motor (holy_util_fd_t fd)
{
  if (!fd->is_floppy)
    return;
  fd->ioreq->iotd_Req.io_Command = TD_MOTOR;
  fd->ioreq->iotd_Req.io_Length = 0;
  fd->ioreq->iotd_Req.io_Data = 0;
  fd->ioreq->iotd_Req.io_Offset = 0;
  fd->ioreq->iotd_Req.io_Actual = 0;
  DoIO ((struct IORequest *) fd->ioreq);
}

static ssize_t
holy_util_fd_read_volume (holy_util_fd_t fd, char *buf, size_t len)
{
  holy_uint64_t adj = 0;

  if (!bounce)
    bounce = AllocVec (BOUNCE_SIZE, MEMF_PUBLIC | MEMF_CLEAR);
  if (!bounce)
    holy_util_error ("out of memory");

  while (len)
    {
      size_t cr = len;
      LONG err;
      if (cr > BOUNCE_SIZE)
	cr = BOUNCE_SIZE;
    retry:
      if (fd->is_64)
	fd->ioreq->iotd_Req.io_Command = NSCMD_TD_READ64;
      else
	fd->ioreq->iotd_Req.io_Command = CMD_READ;
      fd->ioreq->iotd_Req.io_Length = cr;
      fd->ioreq->iotd_Req.io_Data = bounce;
      fd->ioreq->iotd_Req.io_Offset = fd->off & 0xFFFFFFFF;
      fd->ioreq->iotd_Req.io_Actual = fd->off >> 32;
      err = DoIO ((struct IORequest *) fd->ioreq);
      if (err == IOERR_NOCMD && fd->is_64)
	{
	  fd->is_64 = 0;
	  goto retry;
	}
      if (err)
	{
	  holy_util_info ("I/O failed with error %d, IoErr=%d", (int)err, (int) IoErr ());
	  stop_motor (fd);
	  return -1;
	}
      memcpy (buf, bounce, cr);
      adj += cr;
      len -= cr;
      buf += cr;
    }

  fd->off += adj;
  stop_motor (fd);
  return adj;
}

static ssize_t
holy_util_fd_write_volume (holy_util_fd_t fd, const char *buf, size_t len)
{
  holy_uint64_t adj = 0;
  if (!bounce)
    bounce = AllocVec (BOUNCE_SIZE, MEMF_PUBLIC | MEMF_CLEAR);
  if (!bounce)
    holy_util_error ("out of memory");

  while (len)
    {
      size_t cr = len;
      LONG err;
      if (cr > BOUNCE_SIZE)
	cr = BOUNCE_SIZE;
    retry:
      if (fd->is_64)
	fd->ioreq->iotd_Req.io_Command = NSCMD_TD_WRITE64;
      else
	fd->ioreq->iotd_Req.io_Command = CMD_WRITE;
      fd->ioreq->iotd_Req.io_Length = cr;
      fd->ioreq->iotd_Req.io_Data = bounce;
      fd->ioreq->iotd_Req.io_Offset = fd->off & 0xFFFFFFFF;
      fd->ioreq->iotd_Req.io_Actual = fd->off >> 32;
      memcpy (bounce, buf, cr);
      err = DoIO ((struct IORequest *) fd->ioreq);
      if (err == IOERR_NOCMD && fd->is_64)
	{
	  fd->is_64 = 0;
	  goto retry;
	}
      if (err)
	{
	  holy_util_info ("I/O failed with error %d", err);
	  stop_motor (fd);
	  return -1;
	}

      adj += cr;
      len -= cr;
      buf += cr;
    }

  fd->off += adj;
  stop_motor (fd);
  return adj;
}

ssize_t
holy_util_fd_read (holy_util_fd_t fd, char *buf, size_t len)
{
  switch (fd->type)
    {
    case holy_UTIL_FD_FILE:
      return holy_util_fd_read_file (fd, buf, len);
    case holy_UTIL_FD_DISK:
      return holy_util_fd_read_volume (fd, buf, len);
    }
  return -1;
}

ssize_t
holy_util_fd_write (holy_util_fd_t fd, const char *buf, size_t len)
{
  switch (fd->type)
    {
    case holy_UTIL_FD_FILE:
      return holy_util_fd_write_file (fd, buf, len);
    case holy_UTIL_FD_DISK:
      return holy_util_fd_write_volume (fd, buf, len);
    }
  return -1;
}

holy_uint64_t
holy_util_get_fd_size (holy_util_fd_t fd,
		       const char *dev,
		       unsigned *log_secsize)
{
  switch (fd->type)
    {
    case holy_UTIL_FD_FILE:
      return holy_util_get_fd_size_file (fd, dev, log_secsize);

    case holy_UTIL_FD_DISK:
      return holy_util_get_fd_size_volume (fd, dev, log_secsize);
    }
  return -1;
}

void
holy_util_fd_close (holy_util_fd_t fd)
{
  switch (fd->type)
    {
    case holy_UTIL_FD_FILE:
      close (fd->fd);
      return;
    case holy_UTIL_FD_DISK:
      CloseDevice ((struct IORequest *) fd->ioreq);
      DeleteIORequest((struct IORequest *) fd->ioreq);
      DeleteMsgPort (fd->mp);
      return;
    }
}

static int allow_fd_syncs = 1;

static void
holy_util_fd_sync_volume (holy_util_fd_t fd)
{
  fd->ioreq->iotd_Req.io_Command = CMD_UPDATE;
  fd->ioreq->iotd_Req.io_Length = 0;
  fd->ioreq->iotd_Req.io_Data = 0;
  fd->ioreq->iotd_Req.io_Offset = 0;
  fd->ioreq->iotd_Req.io_Actual = 0;
  DoIO ((struct IORequest *) fd->ioreq);
}

void
holy_util_fd_sync (holy_util_fd_t fd)
{
  if (allow_fd_syncs)
    {
      switch (fd->type)
	{
	case holy_UTIL_FD_FILE:
	  fsync (fd->fd);
	  return;
	case holy_UTIL_FD_DISK:
	  holy_util_fd_sync_volume (fd);
	  return;
	}
    }
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
holy_hostdisk_flush_initial_buffer (const char *os_dev __attribute__ ((unused)))
{
}


const char *
holy_util_fd_strerror (void)
{
  static char buf[201];
  LONG err = IoErr ();
  if (!err)
    return _("Success");
  memset (buf, '\0', sizeof (buf));
  Fault (err, (const unsigned char *) "", (STRPTR) buf, sizeof (buf));
  if (buf[0] == ':')
    return buf + 1;
  return buf;
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

int
holy_util_is_special_file (const char *path)
{
  struct stat st;

  if (lstat (path, &st) == -1)
    return 1;
  return (!S_ISREG (st.st_mode) && !S_ISDIR (st.st_mode));
}

static char *
get_temp_name (void)
{
  static int ctr = 0;
  char *t;
  struct stat st;
  
  while (1)
    {
      t = xasprintf ("T:holy.%d.%d.%d.%d", (int) getpid (), (int) getppid (),
		     ctr++, time (0));
      if (stat (t, &st) == -1)
	return t;
      free (t);
    }
}

char *
holy_util_make_temporary_file (void)
{
  char *ret = get_temp_name ();
  FILE *f;

  f = holy_util_fopen (ret, "wb");
  if (f)
    fclose (f);
  return ret;
}

char *
holy_util_make_temporary_dir (void)
{
  char *ret = get_temp_name ();

  holy_util_mkdir (ret);

  return ret;
}

holy_uint32_t
holy_util_get_mtime (const char *path)
{
  struct stat st;

  if (stat (path, &st) == -1)
    return 0;

  return st.st_mtime;
}
