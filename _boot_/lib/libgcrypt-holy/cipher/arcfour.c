/* This file was automatically imported with 
   import_gcry.py. Please don't modify it */
#include <holy/dl.h>
holy_MOD_LICENSE ("GPLv2+");
/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include "types.h"
#include "g10lib.h"
#include "cipher.h"


typedef struct {
    int idx_i, idx_j;
    byte sbox[256];
} ARCFOUR_context;

static void
do_encrypt_stream( ARCFOUR_context *ctx,
		   byte *outbuf, const byte *inbuf, unsigned int length )
{
  register int i = ctx->idx_i;
  register int j = ctx->idx_j;
  register byte *sbox = ctx->sbox;
  register int t;

  while ( length-- )
    {
      i++;
      i = i & 255; /* The and-op seems to be faster than the mod-op. */
      j += sbox[i];
      j &= 255;
      t = sbox[i]; sbox[i] = sbox[j]; sbox[j] = t;
      *outbuf++ = *inbuf++ ^ sbox[(sbox[i] + sbox[j]) & 255];
    }

  ctx->idx_i = i;
  ctx->idx_j = j;
}

static void
encrypt_stream (void *context,
                byte *outbuf, const byte *inbuf, unsigned int length)
{
  ARCFOUR_context *ctx = (ARCFOUR_context *) context;
  do_encrypt_stream (ctx, outbuf, inbuf, length );
  _gcry_burn_stack (64);
}


static gcry_err_code_t
do_arcfour_setkey (void *context, const byte *key, unsigned int keylen)
{
  static int initialized;
  static const char* selftest_failed;
  int i, j;
  byte karr[256];
  ARCFOUR_context *ctx = (ARCFOUR_context *) context;

  if (!initialized )
    {
      initialized = 1;
      selftest_failed = selftest();
      if( selftest_failed )
        log_error ("ARCFOUR selftest failed (%s)\n", selftest_failed );
    }
  if( selftest_failed )
    return GPG_ERR_SELFTEST_FAILED;

  if( keylen < 40/8 ) /* we want at least 40 bits */
    return GPG_ERR_INV_KEYLEN;

  ctx->idx_i = ctx->idx_j = 0;
  for (i=0; i < 256; i++ )
    ctx->sbox[i] = i;
  for (i=0; i < 256; i++ )
    karr[i] = key[i%keylen];
  for (i=j=0; i < 256; i++ )
    {
      int t;
      j = (j + ctx->sbox[i] + karr[i]) % 256;
      t = ctx->sbox[i];
      ctx->sbox[i] = ctx->sbox[j];
      ctx->sbox[j] = t;
    }
  memset( karr, 0, 256 );

  return GPG_ERR_NO_ERROR;
}

static gcry_err_code_t
arcfour_setkey ( void *context, const byte *key, unsigned int keylen )
{
  ARCFOUR_context *ctx = (ARCFOUR_context *) context;
  gcry_err_code_t rc = do_arcfour_setkey (ctx, key, keylen );
  _gcry_burn_stack (300);
  return rc;
}




gcry_cipher_spec_t _gcry_cipher_spec_arcfour =
  {
    "ARCFOUR", NULL, NULL, 1, 128, sizeof (ARCFOUR_context),
    arcfour_setkey, NULL, NULL, encrypt_stream, encrypt_stream,
#ifdef holy_UTIL
    .modname = "gcry_arcfour",
#endif
  };


holy_MOD_INIT(gcry_arcfour)
{
  holy_cipher_register (&_gcry_cipher_spec_arcfour);
}

holy_MOD_FINI(gcry_arcfour)
{
  holy_cipher_unregister (&_gcry_cipher_spec_arcfour);
}
