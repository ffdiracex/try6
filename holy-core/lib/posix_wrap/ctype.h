/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_POSIX_CTYPE_H
#define holy_POSIX_CTYPE_H	1

#include <holy/misc.h>

static inline int
toupper (int c)
{
  return holy_toupper (c);
}

static inline int 
isspace (int c)
{
  return holy_isspace (c);
}

static inline int 
isdigit (int c)
{
  return holy_isdigit (c);
}

static inline int
islower (int c)
{
  return holy_islower (c);
}

static inline int
isascii (int c)
{
  return !(c & ~0x7f);
}

static inline int
isupper (int c)
{
  return holy_isupper (c);
}

static inline int
isxdigit (int c)
{
  return holy_isxdigit (c);
}

static inline int 
isprint (int c)
{
  return holy_isprint (c);
}

static inline int 
iscntrl (int c)
{
  return !holy_isprint (c);
}

static inline int 
isgraph (int c)
{
  return holy_isprint (c) && !holy_isspace (c);
}

static inline int
isalnum (int c)
{
  return holy_isalpha (c) || holy_isdigit (c);
}

static inline int 
ispunct (int c)
{
  return holy_isprint (c) && !holy_isspace (c) && !isalnum (c);
}

static inline int 
isalpha (int c)
{
  return holy_isalpha (c);
}

static inline int
tolower (int c)
{
  return holy_tolower (c);
}

#endif
