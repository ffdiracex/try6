/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_POSIX_WCTYPE_H
#define holy_POSIX_WCTYPE_H	1

#include <holy/misc.h>
#include <wchar.h>

#define wctype_t holy_posix_wctype_t

typedef enum { holy_CTYPE_INVALID,
	       holy_CTYPE_ALNUM, holy_CTYPE_CNTRL, holy_CTYPE_LOWER,
	       holy_CTYPE_SPACE, holy_CTYPE_ALPHA, holy_CTYPE_DIGIT,
	       holy_CTYPE_PRINT, holy_CTYPE_UPPER, holy_CTYPE_BLANK,
	       holy_CTYPE_GRAPH, holy_CTYPE_PUNCT, holy_CTYPE_XDIGIT,
	       holy_CTYPE_MAX} wctype_t;

#define CHARCLASS_NAME_MAX (sizeof ("xdigit") - 1)

static inline wctype_t
wctype (const char *name)
{
  wctype_t i;
  static const char names[][10] = { "", 
				    "alnum", "cntrl", "lower",
				    "space", "alpha", "digit",
				    "print", "upper", "blank",
				    "graph", "punct", "xdigit" };
  for (i = holy_CTYPE_INVALID; i < holy_CTYPE_MAX; i++)
    if (holy_strcmp (names[i], name) == 0)
      return i;
  return holy_CTYPE_INVALID;
}

/* FIXME: take into account international lowercase characters.  */
static inline int
iswlower (wint_t wc)
{
  return holy_islower (wc);
}

static inline wint_t
towlower (wint_t c)
{
  return holy_tolower (c);
}

static inline wint_t
towupper (wint_t c)
{
  return holy_toupper (c);
}

static inline int
iswalnum (wint_t c)
{
  return holy_isalpha (c) || holy_isdigit (c);
}

static inline int
iswctype (wint_t wc, wctype_t desc)
{
  switch (desc)
    {
    case holy_CTYPE_ALNUM:
      return iswalnum (wc);
    case holy_CTYPE_CNTRL:
      return holy_iscntrl (wc);
    case holy_CTYPE_LOWER:
      return iswlower (wc);
    case holy_CTYPE_SPACE:
      return holy_isspace (wc);
    case holy_CTYPE_ALPHA:
      return holy_isalpha (wc);
    case holy_CTYPE_DIGIT:
      return holy_isdigit (wc);
    case holy_CTYPE_PRINT:
      return holy_isprint (wc);
    case holy_CTYPE_UPPER:
      return holy_isupper (wc);
    case holy_CTYPE_BLANK:
      return wc == ' ' || wc == '\t';
    case holy_CTYPE_GRAPH:
      return holy_isgraph (wc);
    case holy_CTYPE_PUNCT:
      return holy_isprint (wc) && !holy_isspace (wc) && !iswalnum (wc);
    case holy_CTYPE_XDIGIT:
      return holy_isxdigit (wc);
    default:
      return 0;
    }
}

#endif
