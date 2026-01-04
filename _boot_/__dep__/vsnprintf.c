/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Specification.  */
#include <stdio.h>

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "vasnprintf.h"

/* Print formatted output to string STR.  Similar to vsprintf, but
   additional length SIZE limit how much is written into STR.  Returns
   string length of formatted string (which may be larger than SIZE).
   STR may be NULL, in which case nothing will be written.  On error,
   return a negative value.  */
int
vsnprintf (char *str, size_t size, const char *format, va_list args)
{
  char *output;
  size_t len;
  size_t lenbuf = size;

  output = vasnprintf (str, &lenbuf, format, args);
  len = lenbuf;

  if (!output)
    return -1;

  if (output != str)
    {
      if (size)
        {
          size_t pruned_len = (len < size ? len : size - 1);
          memcpy (str, output, pruned_len);
          str[pruned_len] = '\0';
        }

      free (output);
    }

  if (len > INT_MAX)
    {
      errno = EOVERFLOW;
      return -1;
    }

  return len;
}
