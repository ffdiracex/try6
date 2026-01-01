/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EMU_HOSTFILE_H
#define holy_EMU_HOSTFILE_H 1

#include <config.h>
#include <stdarg.h>

#include <windows.h>
typedef HANDLE holy_util_fd_t;
#define holy_UTIL_FD_INVALID INVALID_HANDLE_VALUE
#define holy_UTIL_FD_IS_VALID(x) ((x) != holy_UTIL_FD_INVALID)
#define holy_UTIL_FD_STAT_IS_FUNCTIONAL 0

#define DEFAULT_DIRECTORY	"C:\\"holy_BOOT_DIR_NAME"\\"holy_DIR_NAME
#define DEFAULT_DEVICE_MAP	DEFAULT_DIRECTORY "/device.map"

struct holy_util_fd_dirent
{
  char d_name[0];
};
struct holy_util_fd_dir;
typedef struct holy_util_fd_dirent *holy_util_fd_dirent_t;
typedef struct holy_util_fd_dir *holy_util_fd_dir_t;

int
holy_util_rename (const char *from, const char *to);
int
holy_util_unlink (const char *name);
void
holy_util_mkdir (const char *dir);

holy_util_fd_dir_t
holy_util_fd_opendir (const char *name);

void
holy_util_fd_closedir (holy_util_fd_dir_t dirp);

holy_util_fd_dirent_t
holy_util_fd_readdir (holy_util_fd_dir_t dirp);

int
holy_util_rmdir (const char *pathname);

enum holy_util_fd_open_flags_t
  {
    holy_UTIL_FD_O_RDONLY = 1,
    holy_UTIL_FD_O_WRONLY = 2,
    holy_UTIL_FD_O_RDWR = 3,
    holy_UTIL_FD_O_CREATTRUNC = 4,
    holy_UTIL_FD_O_SYNC = 0,
  };

#if defined (__MINGW32__) && !defined (__MINGW64__)

/* 32 bit on Mingw-w64 already redefines them if _FILE_OFFSET_BITS=64 */
#ifndef _W64
#define fseeko fseeko64
#define ftello ftello64
#endif

#endif

LPTSTR
holy_util_utf8_to_tchar (const char *in);
char *
holy_util_tchar_to_utf8 (LPCTSTR in);

#endif
