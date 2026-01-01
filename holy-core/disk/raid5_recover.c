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

static holy_err_t
holy_raid5_recover (struct holy_diskfilter_segment *array, int disknr,
                    char *buf, holy_disk_addr_t sector, holy_size_t size)
{
  char *buf2;
  int i;

  size <<= holy_DISK_SECTOR_BITS;
  buf2 = holy_malloc (size);
  if (!buf2)
    return holy_errno;

  holy_memset (buf, 0, size);

  for (i = 0; i < (int) array->node_count; i++)
    {
      holy_err_t err;

      if (i == disknr)
        continue;

      err = holy_diskfilter_read_node (&array->nodes[i], sector,
				       size >> holy_DISK_SECTOR_BITS, buf2);

      if (err)
        {
          holy_free (buf2);
          return err;
        }

      holy_crypto_xor (buf, buf, buf2, size);
    }

  holy_free (buf2);

  return holy_ERR_NONE;
}

holy_MOD_INIT(raid5rec)
{
  holy_raid5_recover_func = holy_raid5_recover;
}

holy_MOD_FINI(raid5rec)
{
  holy_raid5_recover_func = 0;
}
