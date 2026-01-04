/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/types.h>
#include <holy/crypto.h>
#include <holy/auth.h>
#include <holy/emu/misc.h>
#include <holy/util/misc.h>
#include <holy/i18n.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <wincrypt.h>

int
holy_get_random (void *out, holy_size_t len)
{
  HCRYPTPROV   hCryptProv;
  if (!CryptAcquireContext (&hCryptProv,
			    NULL,
			    MS_DEF_PROV,
			    PROV_RSA_FULL,
			    CRYPT_VERIFYCONTEXT))
    return 1;
  if (!CryptGenRandom (hCryptProv, len, out))
    {
      CryptReleaseContext (hCryptProv, 0);
      return 1;
    }

  CryptReleaseContext (hCryptProv, 0);

  return 0;
}
