/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EMU_HOSTFILE_H
#define holy_EMU_HOSTFILE_H 1

#include <config.h>
#include <stdarg.h>

#include <holy/symbol.h>
#include <holy/types.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>

typedef struct dirent *holy_util_fd_dirent_t;
typedef DIR *holy_util_fd_dir_t;

static inline holy_util_fd_dir_t
holy_util_fd_opendir (const char *name)
{
  return opendir (name);
}

static inline void
holy_util_fd_closedir (holy_util_fd_dir_t dirp)
{
  closedir (dirp);
}

static inline holy_util_fd_dirent_t
holy_util_fd_readdir (holy_util_fd_dir_t dirp)
{
  return readdir (dirp);
}

static inline int
holy_util_unlink (const char *pathname)
{
  return unlink (pathname);
}

static inline int
holy_util_rmdir (const char *pathname)
{
  return rmdir (pathname);
}

static inline int
holy_util_rename (const char *from, const char *to)
{
  return rename (from, to);
}

#define holy_util_mkdir(a) mkdir ((a), 0755)

#if defined (__NetBSD__)
/* NetBSD uses /boot for its boot block.  */
# define DEFAULT_DIRECTORY	"/"holy_DIR_NAME
#else
# define DEFAULT_DIRECTORY	"/"holy_BOOT_DIR_NAME"/"holy_DIR_NAME
#endif

enum holy_util_fd_open_flags_t
  {
    holy_UTIL_FD_O_RDONLY = O_RDONLY,
    holy_UTIL_FD_O_WRONLY = O_WRONLY,
    holy_UTIL_FD_O_RDWR = O_RDWR,
    holy_UTIL_FD_O_CREATTRUNC = O_CREAT | O_TRUNC,
    holy_UTIL_FD_O_SYNC = (0
#ifdef O_SYNC
			   | O_SYNC
#endif
#ifdef O_FSYNC
			   | O_FSYNC
#endif
			   )
  };

#define DEFAULT_DEVICE_MAP	DEFAULT_DIRECTORY "/device.map"

typedef int holy_util_fd_t;
#define holy_UTIL_FD_INVALID -1
#define holy_UTIL_FD_IS_VALID(x) ((x) >= 0)
#define holy_UTIL_FD_STAT_IS_FUNCTIONAL 1

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__APPLE__) || defined(__NetBSD__) || defined (__sun__) || defined(__OpenBSD__) || defined(__HAIKU__)
#define holy_DISK_DEVS_ARE_CHAR 1
#else
#define holy_DISK_DEVS_ARE_CHAR 0
#endif

#endif
