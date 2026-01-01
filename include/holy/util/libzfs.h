/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_LIBZFS_UTIL_HEADER
#define holy_LIBZFS_UTIL_HEADER 1

#include <config.h>

#ifdef HAVE_LIBZFS_H
#include <libzfs.h>
#else /* ! HAVE_LIBZFS_H */

#include <holy/util/libnvpair.h>

typedef void libzfs_handle_t;
typedef void zpool_handle_t;

extern libzfs_handle_t *libzfs_init (void);
extern void libzfs_fini (libzfs_handle_t *);

extern zpool_handle_t *zpool_open (libzfs_handle_t *, const char *);
extern void zpool_close (zpool_handle_t *);

extern int zpool_get_physpath (zpool_handle_t *, const char *);

extern nvlist_t *zpool_get_config (zpool_handle_t *, nvlist_t **);

#endif /* ! HAVE_LIBZFS_H */

libzfs_handle_t *holy_get_libzfs_handle (void);

#endif
