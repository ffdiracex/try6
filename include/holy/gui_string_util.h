/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_GUI_STRING_UTIL_HEADER
#define holy_GUI_STRING_UTIL_HEADER 1

#include <holy/types.h>
#include <holy/gui.h>

char *holy_new_substring (const char *buf,
                          holy_size_t start, holy_size_t end);

char *holy_resolve_relative_path (const char *base, const char *path);

char *holy_get_dirname (const char *file_path);

#endif /* holy_GUI_STRING_UTIL_HEADER */
