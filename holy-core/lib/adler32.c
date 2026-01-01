/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/crypto.h>

holy_MOD_LICENSE ("GPLv2+");

struct adler32_context
{
  holy_uint16_t a, b;
  holy_uint32_t c;
};

static void
adler32_init (void *context)
{
  struct adler32_context *ctx = context;

  ctx->a = 1;
  ctx->b = 0;
}

#define MOD 65521

static holy_uint16_t
mod_add (holy_uint16_t a, holy_uint16_t b)
{
  if ((holy_uint32_t) a + (holy_uint32_t) b >= MOD)
    return a + b - MOD;
  return a + b;
}

static void
adler32_write (void *context, const void *inbuf, holy_size_t inlen)
{
  struct adler32_context *ctx = context;
  const holy_uint8_t *ptr = inbuf;

  while (inlen)
    {
      ctx->a = mod_add (ctx->a, *ptr);
      ctx->b = mod_add (ctx->a, ctx->b);
      inlen--;
      ptr++;
    }
}

static void
adler32_final (void *context __attribute__ ((unused)))
{
}

static holy_uint8_t *
adler32_read (void *context)
{
  struct adler32_context *ctx = context;
  if (ctx->a > MOD)
    ctx->a -= MOD;
  if (ctx->b > MOD)
    ctx->b -= MOD;
  ctx->c = holy_cpu_to_be32 (ctx->a | (ctx->b << 16));
  return (holy_uint8_t *) &ctx->c;
}

static gcry_md_spec_t spec_adler32 =
  {
    "ADLER32", 0, 0, 0, 4,
    adler32_init, adler32_write, adler32_final, adler32_read,
    sizeof (struct adler32_context),
#ifdef holy_UTIL
    .modname = "adler32",
#endif
    .blocksize = 64
  };


holy_MOD_INIT(adler32)
{
  holy_md_register (&spec_adler32);
}

holy_MOD_FINI(adler32)
{
  holy_md_unregister (&spec_adler32);
}
