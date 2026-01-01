/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_POSIX_LANGINFO_H
#define holy_POSIX_LANGINFO_H	1

#include <localcharset.h>

typedef enum { CODESET } nl_item;

static inline const char *
nl_langinfo (nl_item item)
{
  switch (item)
    {
    case CODESET:
      return "UTF-8";
    default:
      return "";
    }
}

#endif
