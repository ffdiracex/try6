/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_POSIX_STDIO_H
#define holy_POSIX_STDIO_H	1

#include <holy/misc.h>
#include <holy/file.h>
#include <sys/types.h>

typedef struct holy_file FILE;

#define EOF    -1

static inline int
snprintf (char *str, holy_size_t n, const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start (ap, fmt);
  ret = holy_vsnprintf (str, n, fmt, ap);
  va_end (ap);

  return ret;
}

#endif
