/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/lib/crc.h>

static holy_uint32_t crc32c_table [256];

/* Helper for init_crc32c_table.  */
static holy_uint32_t
reflect (holy_uint32_t ref, int len)
{
  holy_uint32_t result = 0;
  int i;

  for (i = 1; i <= len; i++)
    {
      if (ref & 1)
	result |= 1 << (len - i);
      ref >>= 1;
    }

  return result;
}

static void
init_crc32c_table (void)
{
  holy_uint32_t polynomial = 0x1edc6f41;
  int i, j;

  for(i = 0; i < 256; i++)
    {
      crc32c_table[i] = reflect(i, 8) << 24;
      for (j = 0; j < 8; j++)
        crc32c_table[i] = (crc32c_table[i] << 1) ^
            (crc32c_table[i] & (1 << 31) ? polynomial : 0);
      crc32c_table[i] = reflect(crc32c_table[i], 32);
    }
}

holy_uint32_t
holy_getcrc32c (holy_uint32_t crc, const void *buf, int size)
{
  int i;
  const holy_uint8_t *data = buf;

  if (! crc32c_table[1])
    init_crc32c_table ();

  crc^= 0xffffffff;

  for (i = 0; i < size; i++)
    {
      crc = (crc >> 8) ^ crc32c_table[(crc & 0xFF) ^ *data];
      data++;
    }

  return crc ^ 0xffffffff;
}
