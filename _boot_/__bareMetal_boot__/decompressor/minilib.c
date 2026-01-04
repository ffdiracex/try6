/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/decompressor.h>

void *
holy_memset (void *s, int c, holy_size_t len)
{
  holy_uint8_t *ptr;
  for (ptr = s; len; ptr++, len--)
    *ptr = c;
  return s;
}

void *
holy_memmove (void *dest, const void *src, holy_size_t n)
{
  char *d = (char *) dest;
  const char *s = (const char *) src;

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

  return dest;
}

int
holy_memcmp (const void *s1, const void *s2, holy_size_t n)
{
  const unsigned char *t1 = s1;
  const unsigned char *t2 = s2;

  while (n--)
    {
      if (*t1 != *t2)
	return (int) *t1 - (int) *t2;

      t1++;
      t2++;
    }

  return 0;
}

void *holy_decompressor_scratch;

void
find_scratch (void *src, void *dst, unsigned long srcsize,
	      unsigned long dstsize)
{
#ifdef _mips
  /* Decoding from ROM.  */
  if (((holy_addr_t) src & 0x10000000))
    {
      holy_decompressor_scratch = (void *) ALIGN_UP((holy_addr_t) dst + dstsize,
						    256);
      return;
    }
#endif
  if ((char *) src + srcsize > (char *) dst + dstsize)
    holy_decompressor_scratch = (void *) ALIGN_UP ((holy_addr_t) src + srcsize,
						   256);
  else
    holy_decompressor_scratch = (void *) ALIGN_UP ((holy_addr_t) dst + dstsize,
						   256);
  return;
}
