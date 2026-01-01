/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_POSIX_STDLIB_H
#define holy_POSIX_STDLIB_H	1

#include <holy/mm.h>
#include <holy/misc.h>

static inline void 
free (void *ptr)
{
  holy_free (ptr);
}

static inline void *
malloc (holy_size_t size)
{
  return holy_malloc (size);
}

static inline void *
calloc (holy_size_t size, holy_size_t nelem)
{
  return holy_zalloc (size * nelem);
}

static inline void *
realloc (void *ptr, holy_size_t size)
{
  return holy_realloc (ptr, size);
}

static inline int
abs (int c)
{
  return (c >= 0) ? c : -c;
}

#endif
