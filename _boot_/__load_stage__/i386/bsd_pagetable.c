/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

static void
fill_bsd64_pagetable (holy_uint8_t *src, holy_addr_t target)
{
  holy_uint64_t *pt2, *pt3, *pt4;
  holy_addr_t pt2t, pt3t;
  int i;

#define PG_V		0x001
#define PG_RW		0x002
#define PG_U		0x004
#define PG_PS		0x080

  pt4 = (holy_uint64_t *) src;
  pt3 = (holy_uint64_t *) (src + 4096);
  pt2 = (holy_uint64_t *) (src + 8192);

  pt3t = target + 4096;
  pt2t = target + 8192;

  holy_memset (src, 0, 4096 * 3);

  /*
   * This is kinda brutal, but every single 1GB VM memory segment points to
   * the same first 1GB of physical memory.  But it is how BSD expects
   * it to be.
   */
  for (i = 0; i < 512; i++)
    {
      /* Each slot of the level 4 pages points to the same level 3 page */
      pt4[i] = (holy_addr_t) pt3t;
      pt4[i] |= PG_V | PG_RW | PG_U;

      /* Each slot of the level 3 pages points to the same level 2 page */
      pt3[i] = (holy_addr_t) pt2t;
      pt3[i] |= PG_V | PG_RW | PG_U;

      /* The level 2 page slots are mapped with 2MB pages for 1GB. */
      pt2[i] = i * (2 * 1024 * 1024);
      pt2[i] |= PG_V | PG_RW | PG_PS | PG_U;
    }
}
