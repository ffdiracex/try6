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

int
holy_get_random (void *out, holy_size_t len)
{
  FILE *f;
  size_t rd;

  f = holy_util_fopen ("/dev/urandom", "rb");
  if (!f)
    return 1;
  rd = fread (out, 1, len, f);
  fclose (f);

  if (rd != len)
    return 1;
  return 0;
}
