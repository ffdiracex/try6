/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_UTIL_MISC_HEADER
#define holy_UTIL_MISC_HEADER	1

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>
#include <unistd.h>

#include <config.h>
#include <holy/types.h>
#include <holy/symbol.h>
#include <holy/emu/misc.h>

char *holy_util_get_path (const char *dir, const char *file);
size_t holy_util_get_image_size (const char *path);
char *holy_util_read_image (const char *path);
void holy_util_load_image (const char *path, char *buf);
void holy_util_write_image (const char *img, size_t size, FILE *out,
			    const char *name);
void holy_util_write_image_at (const void *img, size_t size, off_t offset,
			       FILE *out, const char *name);

char *make_system_path_relative_to_its_root (const char *path);

char *holy_canonicalize_file_name (const char *path);

void holy_util_init_nls (void);

void holy_util_host_init (int *argc, char ***argv);

int holy_qsort_strcmp (const void *, const void *);

#endif /* ! holy_UTIL_MISC_HEADER */
