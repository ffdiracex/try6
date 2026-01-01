/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/test.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/crypto.h>

holy_MOD_LICENSE ("GPLv2+");

static struct
{
  const char *P;
  holy_size_t Plen;
  const char *S;
  holy_size_t Slen;
  unsigned int c;
  holy_size_t dkLen;
  const char *DK;
} vectors[] = {
  /* RFC6070. */
  {
    "password", 8,
    "salt", 4,
    1, 20,
    "\x0c\x60\xc8\x0f\x96\x1f\x0e\x71\xf3\xa9\xb5\x24\xaf\x60\x12"
    "\x06\x2f\xe0\x37\xa6"
  },
  {
    "password", 8,
    "salt", 4,
    2, 20,
    "\xea\x6c\x01\x4d\xc7\x2d\x6f\x8c"
    "\xcd\x1e\xd9\x2a\xce\x1d\x41\xf0"
    "\xd8\xde\x89\x57"
  },
  {
    "password", 8,
    "salt", 4,
    4096, 20,
    "\x4b\x00\x79\x01\xb7\x65\x48\x9a\xbe\xad\x49\xd9\x26\xf7"
    "\x21\xd0\x65\xa4\x29\xc1"
  },
  {
    "passwordPASSWORDpassword", 24,
    "saltSALTsaltSALTsaltSALTsaltSALTsalt", 36,
    4096, 25,
    "\x3d\x2e\xec\x4f\xe4\x1c\x84\x9b\x80\xc8\xd8\x36\x62\xc0"
    "\xe4\x4a\x8b\x29\x1a\x96\x4c\xf2\xf0\x70\x38"
  },
  {
    "pass\0word", 9,
    "sa\0lt", 5,
    4096, 16,
    "\x56\xfa\x6a\xa7\x55\x48\x09\x9d\xcc\x37\xd7\xf0\x34\x25\xe0\xc3"
  }
};

static void
pbkdf2_test (void)
{
  holy_size_t i;

  for (i = 0; i < ARRAY_SIZE (vectors); i++)
    {
      gcry_err_code_t err;
      holy_uint8_t DK[32];
      err = holy_crypto_pbkdf2 (holy_MD_SHA1,
				(const holy_uint8_t *) vectors[i].P,
				vectors[i].Plen,
				(const holy_uint8_t *) vectors[i].S,
				vectors[i].Slen,
				vectors[i].c,
				DK, vectors[i].dkLen);
      holy_test_assert (err == 0, "gcry error %d", err);
      holy_test_assert (holy_memcmp (DK, vectors[i].DK, vectors[i].dkLen) == 0,
			"PBKDF2 mismatch");
    }
}

/* Register example_test method as a functional test.  */
holy_FUNCTIONAL_TEST (pbkdf2_test, pbkdf2_test);
