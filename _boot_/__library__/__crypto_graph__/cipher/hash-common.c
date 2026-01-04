/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif

#include "g10lib.h"
#include "hash-common.h"


/* Run a selftest for hash algorithm ALGO.  If the resulting digest
   matches EXPECT/EXPECTLEN and everything else is fine as well,
   return NULL.  If an error occurs, return a static text string
   describing the error.

   DATAMODE controls what will be hashed according to this table:

     0 - Hash the supplied DATA of DATALEN.
     1 - Hash one million times a 'a'.  DATA and DATALEN are ignored.

*/
const char *
_gcry_hash_selftest_check_one (int algo,
                               int datamode, const void *data, size_t datalen,
                               const void *expect, size_t expectlen)
{
  const char *result = NULL;
  gcry_error_t err = 0;
  gcry_md_hd_t hd;
  unsigned char *digest;

  if (_gcry_md_get_algo_dlen (algo) != expectlen)
    return "digest size does not match expected size";

  err = _gcry_md_open (&hd, algo, 0);
  if (err)
    return "gcry_md_open failed";

  switch (datamode)
    {
    case 0:
      _gcry_md_write (hd, data, datalen);
      break;

    case 1: /* Hash one million times an "a". */
      {
        char aaa[1000];
        int i;

        /* Write in odd size chunks so that we test the buffering.  */
        memset (aaa, 'a', 1000);
        for (i = 0; i < 1000; i++)
          _gcry_md_write (hd, aaa, 1000);
      }
      break;

    default:
      result = "invalid DATAMODE";
    }

  if (!result)
    {
      digest = _gcry_md_read (hd, algo);

      if ( memcmp (digest, expect, expectlen) )
        result = "digest mismatch";
    }

  _gcry_md_close (hd);

  return result;
}
