/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EMU_MISC_H
#define holy_EMU_MISC_H 1

/*#include <config.h> */
#include <config.h>
#include <stdarg.h>
#include <holy/compiler.h>
#include <stdio.h>
#include <holy/gcrypt/gcrypt.h>
#include <holy/symbol.h>
#include <holy/types.h>
#include <holy/misc.h>
#include <holy/util/misc.h>

extern int verbosity;
extern const char *program_name;


extern const unsigned long long int holy_size_t;
void holy_init_all (void);
void holy_fini_all (void);

void holy_find_zpool_from_dir (const char *dir,
			       char **poolname, char **poolfs);

char *holy_make_system_path_relative_to_its_root (const char *path)
 WARN_UNUSED_RESULT;
int
holy_util_device_is_mapped (const char *dev);

#ifdef __MINGW32__
#define holy_HOST_PRIuLONG_LONG "I64u"
#define holy_HOST_PRIxLONG_LONG "I64x"
#else
#define holy_HOST_PRIuLONG_LONG "llu"
#define holy_HOST_PRIxLONG_LONG "llx"
#endif

void * EXPORT_FUNC(xmalloc) (holy_size_t size) WARN_UNUSED_RESULT;
void * EXPORT_FUNC(xrealloc) (void *ptr, holy_size_t size) WARN_UNUSED_RESULT;
char * EXPORT_FUNC(xstrdup) (const char *str) WARN_UNUSED_RESULT;
char * EXPORT_FUNC(xasprintf) (const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2))) WARN_UNUSED_RESULT;

void EXPORT_FUNC(holy_util_warn) (const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2)));
void EXPORT_FUNC(holy_util_info) (const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2)));
void EXPORT_FUNC(holy_util_error) (const char *fmt, ...) __attribute__ ((format (__printf__, 1, 2), noreturn));

holy_uint64_t EXPORT_FUNC (holy_util_get_cpu_time_ms) (void);

#ifdef HAVE_DEVICE_MAPPER
int holy_device_mapper_supported (void);
#endif

#ifdef holy_BUILD
#define holy_util_fopen fopen
#else
FILE *
holy_util_fopen (const char *path, const char *mode);
#endif

void holy_util_file_sync (FILE *f);

#endif /* holy_EMU_MISC_H */
