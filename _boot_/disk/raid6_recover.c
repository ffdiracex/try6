/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/disk.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/misc.h>
#include <holy/diskfilter.h>
#include <holy/crypto.h>

holy_MOD_LICENSE ("GPLv2+");

/* x**y.  */
static holy_uint8_t powx[255 * 2];
/* Such an s that x**s = y */
static unsigned powx_inv[256];
static const holy_uint8_t poly = 0x1d;

static void
holy_raid_block_mulx (unsigned mul, char *buf, holy_size_t size)
{
  holy_size_t i;
  holy_uint8_t *p;

  p = (holy_uint8_t *) buf;
  for (i = 0; i < size; i++, p++)
    if (*p)
      *p = powx[mul + powx_inv[*p]];
}

static void
holy_raid6_init_table (void)
{
  unsigned i;

  holy_uint8_t cur = 1;
  for (i = 0; i < 255; i++)
    {
      powx[i] = cur;
      powx[i + 255] = cur;
      powx_inv[cur] = i;
      if (cur & 0x80)
	cur = (cur << 1) ^ poly;
      else
	cur <<= 1;
    }
}

static unsigned
mod_255 (unsigned x)
{
  while (x > 0xff)
    x = (x >> 8) + (x & 0xff);
  if (x == 0xff)
    return 0;
  return x;
}

static holy_err_t
holy_raid6_recover (struct holy_diskfilter_segment *array, int disknr, int p,
                    char *buf, holy_disk_addr_t sector, holy_size_t size)
{
  int i, q, pos;
  int bad1 = -1, bad2 = -1;
  char *pbuf = 0, *qbuf = 0;

  size <<= holy_DISK_SECTOR_BITS;
  pbuf = holy_zalloc (size);
  if (!pbuf)
    goto quit;

  qbuf = holy_zalloc (size);
  if (!qbuf)
    goto quit;

  q = p + 1;
  if (q == (int) array->node_count)
    q = 0;

  pos = q + 1;
  if (pos == (int) array->node_count)
    pos = 0;

  for (i = 0; i < (int) array->node_count - 2; i++)
    {
      int c;
      if (array->layout & holy_RAID_LAYOUT_MUL_FROM_POS)
	c = pos;
      else
	c = i;
      if (pos == disknr)
        bad1 = c;
      else
        {
          if (! holy_diskfilter_read_node (&array->nodes[pos], sector,
					   size >> holy_DISK_SECTOR_BITS, buf))
            {
              holy_crypto_xor (pbuf, pbuf, buf, size);
              holy_raid_block_mulx (c, buf, size);
              holy_crypto_xor (qbuf, qbuf, buf, size);
            }
          else
            {
              /* Too many bad devices */
              if (bad2 >= 0)
                goto quit;

              bad2 = c;
              holy_errno = holy_ERR_NONE;
            }
        }

      pos++;
      if (pos == (int) array->node_count)
        pos = 0;
    }

  /* Invalid disknr or p */
  if (bad1 < 0)
    goto quit;

  if (bad2 < 0)
    {
      /* One bad device */
      if ((! holy_diskfilter_read_node (&array->nodes[p], sector,
					size >> holy_DISK_SECTOR_BITS, buf)))
        {
          holy_crypto_xor (buf, buf, pbuf, size);
          goto quit;
        }

      holy_errno = holy_ERR_NONE;
      if (holy_diskfilter_read_node (&array->nodes[q], sector,
				     size >> holy_DISK_SECTOR_BITS, buf))
        goto quit;

      holy_crypto_xor (buf, buf, qbuf, size);
      holy_raid_block_mulx (255 - bad1, buf,
                           size);
    }
  else
    {
      /* Two bad devices */
      unsigned c;

      if (holy_diskfilter_read_node (&array->nodes[p], sector,
				     size >> holy_DISK_SECTOR_BITS, buf))
        goto quit;

      holy_crypto_xor (pbuf, pbuf, buf, size);

      if (holy_diskfilter_read_node (&array->nodes[q], sector,
				     size >> holy_DISK_SECTOR_BITS, buf))
        goto quit;

      holy_crypto_xor (qbuf, qbuf, buf, size);

      c = mod_255((255 ^ bad1)
		  + (255 ^ powx_inv[(powx[bad2 + (bad1 ^ 255)] ^ 1)]));
      holy_raid_block_mulx (c, qbuf, size);

      c = mod_255((unsigned) bad2 + c);
      holy_raid_block_mulx (c, pbuf, size);

      holy_crypto_xor (pbuf, pbuf, qbuf, size);
      holy_memcpy (buf, pbuf, size);
    }

quit:
  holy_free (pbuf);
  holy_free (qbuf);

  return holy_errno;
}

holy_MOD_INIT(raid6rec)
{
  holy_raid6_init_table ();
  holy_raid6_recover_func = holy_raid6_recover;
}

holy_MOD_FINI(raid6rec)
{
  holy_raid6_recover_func = 0;
}
