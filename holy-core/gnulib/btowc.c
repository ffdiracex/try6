/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

/* Specification.  */
#include <wchar.h>

#include <stdio.h>
#include <stdlib.h>

wint_t
btowc (int c)
{
  if (c != EOF)
    {
      char buf[1];
      wchar_t wc;

      buf[0] = c;
      if (mbtowc (&wc, buf, 1) >= 0)
        return wc;
    }
  return WEOF;
}
