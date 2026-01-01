/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_HOSTFILE_EMU_HEADER
#define holy_HOSTFILE_EMU_HEADER	1

#include <holy/disk.h>
#include <holy/partition.h>
#include <sys/types.h>
#include <holy/osdep/hostfile.h>
#include <holy/symbol.h>
int
holy_util_is_directory (const char *path);
int
holy_util_is_special_file (const char *path);
int
holy_util_is_regular (const char *path);

char *
holy_util_path_concat (size_t n, ...);
char *
holy_util_path_concat_ext (size_t n, ...);

int
holy_util_fd_seek (holy_util_fd_t fd, holy_uint64_t off);
ssize_t
EXPORT_FUNC(holy_util_fd_read) (holy_util_fd_t fd, char *buf, size_t len);
ssize_t
EXPORT_FUNC(holy_util_fd_write) (holy_util_fd_t fd, const char *buf, size_t len);

holy_util_fd_t
EXPORT_FUNC(holy_util_fd_open) (const char *os_dev, int flags);
const char *
EXPORT_FUNC(holy_util_fd_strerror) (void);
void
holy_util_fd_sync (holy_util_fd_t fd);
void
holy_util_disable_fd_syncs (void);
void
EXPORT_FUNC(holy_util_fd_close) (holy_util_fd_t fd);

holy_uint64_t
holy_util_get_fd_size (holy_util_fd_t fd, const char *name, unsigned *log_secsize);
char *
holy_util_make_temporary_file (void);
char *
holy_util_make_temporary_dir (void);
void
holy_util_unlink_recursive (const char *name);
holy_uint32_t
holy_util_get_mtime (const char *name);

#endif /* ! holy_BIOSDISK_MACHINE_UTIL_HEADER */
