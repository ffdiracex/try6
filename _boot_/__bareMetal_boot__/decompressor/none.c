/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/decompressor.h>

void
holy_decompress_core (void *src, void *dest, unsigned long n,
		      unsigned long dstsize __attribute__ ((unused)))
{
  char *d = (char *) dest;
  const char *s = (const char *) src;

  if (d == s)
    return;

  if (d < s)
    while (n--)
      *d++ = *s++;
  else
    {
      d += n;
      s += n;

      while (n--)
	*--d = *--s;
    }
}
