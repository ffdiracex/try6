/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/crypto.h>
#include <holy/mm.h>
#include <holy/misc.h>

gcry_err_code_t AF_merge (const gcry_md_spec_t * hash, holy_uint8_t * src,
			  holy_uint8_t * dst, holy_size_t blocksize,
			  holy_size_t blocknumbers);

static void
diffuse (const gcry_md_spec_t * hash, holy_uint8_t * src,
	 holy_uint8_t * dst, holy_size_t size)
{
  holy_size_t i;
  holy_uint32_t IV;		/* host byte order independend hash IV */

  holy_size_t fullblocks = size / hash->mdlen;
  int padding = size % hash->mdlen;
  holy_uint8_t final[holy_CRYPTO_MAX_MDLEN];
  holy_uint8_t temp[sizeof (IV) + holy_CRYPTO_MAX_MDLEN];

  /* hash block the whole data set with different IVs to produce
   * more than just a single data block
   */
  for (i = 0; i < fullblocks; i++)
    {
      IV = holy_cpu_to_be32 (i);
      holy_memcpy (temp, &IV, sizeof (IV));
      holy_memcpy (temp + sizeof (IV), src + hash->mdlen * i, hash->mdlen);
      holy_crypto_hash (hash, dst + hash->mdlen * i, temp,
			sizeof (IV) + hash->mdlen);
    }

  if (padding)
    {
      IV = holy_cpu_to_be32 (i);
      holy_memcpy (temp, &IV, sizeof (IV));
      holy_memcpy (temp + sizeof (IV), src + hash->mdlen * i, padding);
      holy_crypto_hash (hash, final, temp, sizeof (IV) + padding);
      holy_memcpy (dst + hash->mdlen * i, final, padding);
    }
}

/**
 * Merges the splitted master key stored on disk into the original key
 */
gcry_err_code_t
AF_merge (const gcry_md_spec_t * hash, holy_uint8_t * src, holy_uint8_t * dst,
	  holy_size_t blocksize, holy_size_t blocknumbers)
{
  holy_size_t i;
  holy_uint8_t *bufblock;

  if (hash->mdlen > holy_CRYPTO_MAX_MDLEN || hash->mdlen == 0)
    return GPG_ERR_INV_ARG;

  bufblock = holy_zalloc (blocksize);
  if (bufblock == NULL)
    return GPG_ERR_OUT_OF_MEMORY;

  holy_memset (bufblock, 0, blocksize);
  for (i = 0; i < blocknumbers - 1; i++)
    {
      holy_crypto_xor (bufblock, src + (blocksize * i), bufblock, blocksize);
      diffuse (hash, bufblock, bufblock, blocksize);
    }
  holy_crypto_xor (dst, src + (i * blocksize), bufblock, blocksize);

  holy_free (bufblock);
  return GPG_ERR_NO_ERROR;
}
