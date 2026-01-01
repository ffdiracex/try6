/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/crypto.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/term.h>
#include <holy/dl.h>
#include <holy/i18n.h>
#include <holy/env.h>

holy_MOD_LICENSE ("GPLv2+");

struct holy_crypto_hmac_handle
{
  const struct gcry_md_spec *md;
  void *ctx;
  void *opad;
};

static gcry_cipher_spec_t *holy_ciphers = NULL;
static gcry_md_spec_t *holy_digests = NULL;

void (*holy_crypto_autoload_hook) (const char *name) = NULL;

/* Based on libgcrypt-1.4.4/src/misc.c.  */
void
holy_burn_stack (holy_size_t size)
{
  char buf[64];

  holy_memset (buf, 0, sizeof (buf));
  if (size > sizeof (buf))
    holy_burn_stack (size - sizeof (buf));
}

void
_gcry_burn_stack (int size)
{
  holy_burn_stack (size);
}

void __attribute__ ((noreturn))
_gcry_assert_failed (const char *expr, const char *file, int line,
		     const char *func)
  
{
  holy_fatal ("assertion %s at %s:%d (%s) failed\n", expr, file, line, func);
}


void _gcry_log_error (const char *fmt, ...)
{
  va_list args;
  const char *debug = holy_env_get ("debug");

  if (! debug)
    return;

  if (holy_strword (debug, "all") || holy_strword (debug, "gcrypt"))
    {
      holy_printf ("gcrypt error: ");
      va_start (args, fmt);
      holy_vprintf (fmt, args);
      va_end (args);
      holy_refresh ();
    }
}

void 
holy_cipher_register (gcry_cipher_spec_t *cipher)
{
  cipher->next = holy_ciphers;
  holy_ciphers = cipher;
}

void
holy_cipher_unregister (gcry_cipher_spec_t *cipher)
{
  gcry_cipher_spec_t **ciph;
  for (ciph = &holy_ciphers; *ciph; ciph = &((*ciph)->next))
    if (*ciph == cipher)
      {
	*ciph = (*ciph)->next;
	break;
      }
}

void 
holy_md_register (gcry_md_spec_t *digest)
{
  digest->next = holy_digests;
  holy_digests = digest;
}

void 
holy_md_unregister (gcry_md_spec_t *cipher)
{
  gcry_md_spec_t **ciph;
  for (ciph = &holy_digests; *ciph; ciph = &((*ciph)->next))
    if (*ciph == cipher)
      {
	*ciph = (*ciph)->next;
	break;
      }
}

void
holy_crypto_hash (const gcry_md_spec_t *hash, void *out, const void *in,
		  holy_size_t inlen)
{
  holy_PROPERLY_ALIGNED_ARRAY (ctx, holy_CRYPTO_MAX_MD_CONTEXT_SIZE);

  if (hash->contextsize > sizeof (ctx))
    holy_fatal ("Too large md context");
  hash->init (&ctx);
  hash->write (&ctx, in, inlen);
  hash->final (&ctx);
  holy_memcpy (out, hash->read (&ctx), hash->mdlen);
}

const gcry_md_spec_t *
holy_crypto_lookup_md_by_name (const char *name)
{
  const gcry_md_spec_t *md;
  int first = 1;
  while (1)
    {
      for (md = holy_digests; md; md = md->next)
	if (holy_strcasecmp (name, md->name) == 0)
	  return md;
      if (holy_crypto_autoload_hook && first)
	holy_crypto_autoload_hook (name);
      else
	return NULL;
      first = 0;
    }
}

const gcry_cipher_spec_t *
holy_crypto_lookup_cipher_by_name (const char *name)
{
  const gcry_cipher_spec_t *ciph;
  int first = 1;
  while (1)
    {
      for (ciph = holy_ciphers; ciph; ciph = ciph->next)
	{
	  const char **alias;
	  if (holy_strcasecmp (name, ciph->name) == 0)
	    return ciph;
	  if (!ciph->aliases)
	    continue;
	  for (alias = ciph->aliases; *alias; alias++)
	    if (holy_strcasecmp (name, *alias) == 0)
	      return ciph;
	}
      if (holy_crypto_autoload_hook && first)
	holy_crypto_autoload_hook (name);
      else
	return NULL;
      first = 0;
    }
}


holy_crypto_cipher_handle_t
holy_crypto_cipher_open (const struct gcry_cipher_spec *cipher)
{
  holy_crypto_cipher_handle_t ret;
  ret = holy_malloc (sizeof (*ret) + cipher->contextsize);
  if (!ret)
    return NULL;
  ret->cipher = cipher;
  return ret;
}

gcry_err_code_t
holy_crypto_cipher_set_key (holy_crypto_cipher_handle_t cipher,
			    const unsigned char *key,
			    unsigned keylen)
{
  return cipher->cipher->setkey (cipher->ctx, key, keylen);
}

gcry_err_code_t
holy_crypto_ecb_decrypt (holy_crypto_cipher_handle_t cipher,
			 void *out, const void *in, holy_size_t size)
{
  const holy_uint8_t *inptr, *end;
  holy_uint8_t *outptr;
  holy_size_t blocksize;
  if (!cipher->cipher->decrypt)
    return GPG_ERR_NOT_SUPPORTED;
  blocksize = cipher->cipher->blocksize;
  if (blocksize == 0 || (((blocksize - 1) & blocksize) != 0)
      || ((size & (blocksize - 1)) != 0))
    return GPG_ERR_INV_ARG;
  end = (const holy_uint8_t *) in + size;
  for (inptr = in, outptr = out; inptr < end;
       inptr += blocksize, outptr += blocksize)
    cipher->cipher->decrypt (cipher->ctx, outptr, inptr);
  return GPG_ERR_NO_ERROR;
}

gcry_err_code_t
holy_crypto_ecb_encrypt (holy_crypto_cipher_handle_t cipher,
			 void *out, const void *in, holy_size_t size)
{
  const holy_uint8_t *inptr, *end;
  holy_uint8_t *outptr;
  holy_size_t blocksize;
  if (!cipher->cipher->encrypt)
    return GPG_ERR_NOT_SUPPORTED;
  blocksize = cipher->cipher->blocksize;
  if (blocksize == 0 || (((blocksize - 1) & blocksize) != 0)
      || ((size & (blocksize - 1)) != 0))
    return GPG_ERR_INV_ARG;
  end = (const holy_uint8_t *) in + size;
  for (inptr = in, outptr = out; inptr < end;
       inptr += blocksize, outptr += blocksize)
    cipher->cipher->encrypt (cipher->ctx, outptr, inptr);
  return GPG_ERR_NO_ERROR;
}

gcry_err_code_t
holy_crypto_cbc_encrypt (holy_crypto_cipher_handle_t cipher,
			 void *out, const void *in, holy_size_t size,
			 void *iv_in)
{
  holy_uint8_t *outptr;
  const holy_uint8_t *inptr, *end;
  void *iv;
  holy_size_t blocksize;
  if (!cipher->cipher->encrypt)
    return GPG_ERR_NOT_SUPPORTED;
  blocksize = cipher->cipher->blocksize;
  if (blocksize == 0 || (((blocksize - 1) & blocksize) != 0)
      || ((size & (blocksize - 1)) != 0))
    return GPG_ERR_INV_ARG;
  end = (const holy_uint8_t *) in + size;
  iv = iv_in;
  for (inptr = in, outptr = out; inptr < end;
       inptr += blocksize, outptr += blocksize)
    {
      holy_crypto_xor (outptr, inptr, iv, blocksize);
      cipher->cipher->encrypt (cipher->ctx, outptr, outptr);
      iv = outptr;
    }
  holy_memcpy (iv_in, iv, blocksize);
  return GPG_ERR_NO_ERROR;
}

gcry_err_code_t
holy_crypto_cbc_decrypt (holy_crypto_cipher_handle_t cipher,
			 void *out, const void *in, holy_size_t size,
			 void *iv)
{
  const holy_uint8_t *inptr, *end;
  holy_uint8_t *outptr;
  holy_uint8_t ivt[holy_CRYPTO_MAX_CIPHER_BLOCKSIZE];
  holy_size_t blocksize;
  if (!cipher->cipher->decrypt)
    return GPG_ERR_NOT_SUPPORTED;
  blocksize = cipher->cipher->blocksize;
  if (blocksize == 0 || (((blocksize - 1) & blocksize) != 0)
      || ((size & (blocksize - 1)) != 0))
    return GPG_ERR_INV_ARG;
  if (blocksize > holy_CRYPTO_MAX_CIPHER_BLOCKSIZE)
    return GPG_ERR_INV_ARG;
  end = (const holy_uint8_t *) in + size;
  for (inptr = in, outptr = out; inptr < end;
       inptr += blocksize, outptr += blocksize)
    {
      holy_memcpy (ivt, inptr, blocksize);
      cipher->cipher->decrypt (cipher->ctx, outptr, inptr);
      holy_crypto_xor (outptr, outptr, iv, blocksize);
      holy_memcpy (iv, ivt, blocksize);
    }
  return GPG_ERR_NO_ERROR;
}

/* Based on gcry/cipher/md.c.  */
struct holy_crypto_hmac_handle *
holy_crypto_hmac_init (const struct gcry_md_spec *md,
		       const void *key, holy_size_t keylen)
{
  holy_uint8_t *helpkey = NULL;
  holy_uint8_t *ipad = NULL, *opad = NULL;
  void *ctx = NULL;
  struct holy_crypto_hmac_handle *ret = NULL;
  unsigned i;

  if (md->mdlen > md->blocksize)
    return NULL;

  ctx = holy_malloc (md->contextsize);
  if (!ctx)
    goto err;

  if ( keylen > md->blocksize ) 
    {
      helpkey = holy_malloc (md->mdlen);
      if (!helpkey)
	goto err;
      holy_crypto_hash (md, helpkey, key, keylen);

      key = helpkey;
      keylen = md->mdlen;
    }

  ipad = holy_zalloc (md->blocksize);
  if (!ipad)
    goto err;

  opad = holy_zalloc (md->blocksize);
  if (!opad)
    goto err;

  holy_memcpy ( ipad, key, keylen );
  holy_memcpy ( opad, key, keylen );
  for (i=0; i < md->blocksize; i++ ) 
    {
      ipad[i] ^= 0x36;
      opad[i] ^= 0x5c;
    }
  holy_free (helpkey);
  helpkey = NULL;

  md->init (ctx);

  md->write (ctx, ipad, md->blocksize); /* inner pad */
  holy_memset (ipad, 0, md->blocksize);
  holy_free (ipad);
  ipad = NULL;

  ret = holy_malloc (sizeof (*ret));
  if (!ret)
    goto err;

  ret->md = md;
  ret->ctx = ctx;
  ret->opad = opad;

  return ret;

 err:
  holy_free (helpkey);
  holy_free (ctx);
  holy_free (ipad);
  holy_free (opad);
  return NULL;
}

void
holy_crypto_hmac_write (struct holy_crypto_hmac_handle *hnd,
			const void *data,
			holy_size_t datalen)
{
  hnd->md->write (hnd->ctx, data, datalen);
}

gcry_err_code_t
holy_crypto_hmac_fini (struct holy_crypto_hmac_handle *hnd, void *out)
{
  holy_uint8_t *p;
  holy_uint8_t *ctx2;

  ctx2 = holy_malloc (hnd->md->contextsize);
  if (!ctx2)
    return GPG_ERR_OUT_OF_MEMORY;

  hnd->md->final (hnd->ctx);
  hnd->md->read (hnd->ctx);
  p = hnd->md->read (hnd->ctx);

  hnd->md->init (ctx2);
  hnd->md->write (ctx2, hnd->opad, hnd->md->blocksize);
  hnd->md->write (ctx2, p, hnd->md->mdlen);
  hnd->md->final (ctx2);
  holy_memset (hnd->opad, 0, hnd->md->blocksize);
  holy_free (hnd->opad);
  holy_memset (hnd->ctx, 0, hnd->md->contextsize);
  holy_free (hnd->ctx);

  holy_memcpy (out, hnd->md->read (ctx2), hnd->md->mdlen);
  holy_memset (ctx2, 0, hnd->md->contextsize);
  holy_free (ctx2);

  holy_memset (hnd, 0, sizeof (*hnd));
  holy_free (hnd);

  return GPG_ERR_NO_ERROR;
}

gcry_err_code_t
holy_crypto_hmac_buffer (const struct gcry_md_spec *md,
			 const void *key, holy_size_t keylen,
			 const void *data, holy_size_t datalen, void *out)
{
  struct holy_crypto_hmac_handle *hnd;

  hnd = holy_crypto_hmac_init (md, key, keylen);
  if (!hnd)
    return GPG_ERR_OUT_OF_MEMORY;

  holy_crypto_hmac_write (hnd, data, datalen);
  return holy_crypto_hmac_fini (hnd, out);
}


holy_err_t
holy_crypto_gcry_error (gcry_err_code_t in)
{
  if (in == GPG_ERR_NO_ERROR)
    return holy_ERR_NONE;
  return holy_ACCESS_DENIED;
}

int
holy_crypto_memcmp (const void *a, const void *b, holy_size_t n)
{
  register holy_size_t counter = 0;
  const holy_uint8_t *pa, *pb;

  for (pa = a, pb = b; n; pa++, pb++, n--)
    {
      if (*pa != *pb)
	counter++;
    }

  return !!counter;
}

#ifndef holy_UTIL

int
holy_password_get (char buf[], unsigned buf_size)
{
  unsigned cur_len = 0;
  int key;

  while (1)
    {
      key = holy_getkey ();
      if (key == '\n' || key == '\r')
	break;

      if (key == '\e')
	{
	  cur_len = 0;
	  break;
	}

      if (key == '\b')
	{
	  if (cur_len)
	    cur_len--;
	  continue;
	}

      if (!holy_isprint (key))
	continue;

      if (cur_len + 2 < buf_size)
	buf[cur_len++] = key;
    }

  holy_memset (buf + cur_len, 0, buf_size - cur_len);

  holy_xputs ("\n");
  holy_refresh ();

  return (key != '\e');
}
#endif

