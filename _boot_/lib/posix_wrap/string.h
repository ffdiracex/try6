/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_POSIX_STRING_H
#define holy_POSIX_STRING_H	1

#include <holy/misc.h>
#include <sys/types.h>

#define HAVE_STRCASECMP 1

static inline holy_size_t
strlen (const char *s)
{
  return holy_strlen (s);
}

static inline int 
strcmp (const char *s1, const char *s2)
{
  return holy_strcmp (s1, s2);
}

static inline int 
strcasecmp (const char *s1, const char *s2)
{
  return holy_strcasecmp (s1, s2);
}

static inline void
bcopy (const void *src, void *dest, holy_size_t n)
{
  holy_memcpy (dest, src, n);
}

static inline char *
strcpy (char *dest, const char *src)
{
  return holy_strcpy (dest, src);
}

static inline char *
strstr (const char *haystack, const char *needle)
{
  return holy_strstr (haystack, needle);
}

static inline char *
strchr (const char *s, int c)
{
  return holy_strchr (s, c);
}

static inline char *
strncpy (char *dest, const char *src, holy_size_t n)
{
  return holy_strncpy (dest, src, n);
}

static inline int
strcoll (const char *s1, const char *s2)
{
  return holy_strcmp (s1, s2);
}

static inline void *
memchr (const void *s, int c, holy_size_t n)
{
  return holy_memchr (s, c, n);
}

#define memcmp holy_memcmp
#define memcpy holy_memcpy
#define memmove holy_memmove
#define memset holy_memset

#endif
