/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/crypto.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

/* Implement PKCS#5 PBKDF2 as per RFC 2898.  The PRF to use is HMAC variant
   of digest supplied by MD.  Inputs are the password P of length PLEN,
   the salt S of length SLEN, the iteration counter C (> 0), and the
   desired derived output length DKLEN.  Output buffer is DK which
   must have room for at least DKLEN octets.  The output buffer will
   be filled with the derived data.  */

gcry_err_code_t
holy_crypto_pbkdf2 (const struct gcry_md_spec *md,
		    const holy_uint8_t *P, holy_size_t Plen,
		    const holy_uint8_t *S, holy_size_t Slen,
		    unsigned int c,
		    holy_uint8_t *DK, holy_size_t dkLen)
{
  unsigned int hLen = md->mdlen;
  holy_uint8_t U[holy_CRYPTO_MAX_MDLEN];
  holy_uint8_t T[holy_CRYPTO_MAX_MDLEN];
  unsigned int u;
  unsigned int l;
  unsigned int r;
  unsigned int i;
  unsigned int k;
  gcry_err_code_t rc;
  holy_uint8_t *tmp;
  holy_size_t tmplen = Slen + 4;

  if (md->mdlen > holy_CRYPTO_MAX_MDLEN || md->mdlen == 0)
    return GPG_ERR_INV_ARG;

  if (c == 0)
    return GPG_ERR_INV_ARG;

  if (dkLen == 0)
    return GPG_ERR_INV_ARG;

  if (dkLen > 4294967295U)
    return GPG_ERR_INV_ARG;

  l = ((dkLen - 1) / hLen) + 1;
  r = dkLen - (l - 1) * hLen;

  tmp = holy_malloc (tmplen);
  if (tmp == NULL)
    return GPG_ERR_OUT_OF_MEMORY;

  holy_memcpy (tmp, S, Slen);

  for (i = 1; i - 1 < l; i++)
    {
      holy_memset (T, 0, hLen);

      for (u = 0; u < c; u++)
	{
	  if (u == 0)
	    {
	      tmp[Slen + 0] = (i & 0xff000000) >> 24;
	      tmp[Slen + 1] = (i & 0x00ff0000) >> 16;
	      tmp[Slen + 2] = (i & 0x0000ff00) >> 8;
	      tmp[Slen + 3] = (i & 0x000000ff) >> 0;

	      rc = holy_crypto_hmac_buffer (md, P, Plen, tmp, tmplen, U);
	    }
	  else
	    rc = holy_crypto_hmac_buffer (md, P, Plen, U, hLen, U);

	  if (rc != GPG_ERR_NO_ERROR)
	    {
	      holy_free (tmp);
	      return rc;
	    }

	  for (k = 0; k < hLen; k++)
	    T[k] ^= U[k];
	}

      holy_memcpy (DK + (i - 1) * hLen, T, i == l ? r : hLen);
    }

  holy_free (tmp);

  return GPG_ERR_NO_ERROR;
}
