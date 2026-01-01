/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_POSIX_WCHAR_H
#define holy_POSIX_WCHAR_H	1

#include <holy/charset.h>

#define wint_t holy_posix_wint_t
#define wchar_t holy_posix_wchar_t
#define mbstate_t holy_posix_mbstate_t

/* UCS-4.  */
typedef holy_int32_t wint_t;
enum
  {
    WEOF = -1
  };

/* UCS-4.  */
typedef holy_int32_t wchar_t;

typedef struct mbstate {
  holy_uint32_t code;
  int count;
} mbstate_t;

/* UTF-8. */
#define MB_CUR_MAX 4
#define MB_LEN_MAX 4

static inline size_t
mbrtowc (wchar_t *pwc, const char *s, size_t n, mbstate_t *ps)
{
  const char *ptr;
  if (!s)
    {
      pwc = 0;
      s = "";
      n = 1;
    }

  if (pwc)
    *pwc = 0;

  for (ptr = s; ptr < s + n; ptr++)
    {
      if (!holy_utf8_process (*ptr, &ps->code, &ps->count))
	return -1;
      if (ps->count)
	continue;
      if (pwc)
	*pwc = ps->code;
      if (ps->code == 0)
	return 0;
      return ptr - s + 1;
    }
  return -2;
}

static inline int
mbsinit(const mbstate_t *ps)
{
  return ps->count == 0;
}

static inline size_t
wcrtomb (char *s, wchar_t wc, mbstate_t *ps __attribute__ ((unused)))
{
  if (s == 0)
    return 1;
  return holy_encode_utf8_character ((holy_uint8_t *) s,
				     (holy_uint8_t *) s + MB_LEN_MAX,
				     wc);
}

static inline wint_t btowc (int c)
{
  if (c & ~0x7f)
    return WEOF;
  return c;
}


static inline int
wcscoll (const wchar_t *s1, const wchar_t *s2)
{
  while (*s1 && *s2)
    {
      if (*s1 != *s2)
	break;

      s1++;
      s2++;
    }

  if (*s1 < *s2)
    return -1;
  if (*s1 > *s2)
    return +1;
  return 0;
}

#endif
