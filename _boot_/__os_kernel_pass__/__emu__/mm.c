/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config-util.h>

#include <holy/types.h>
#include <holy/err.h>
#include <holy/mm.h>
#include <stdlib.h>
#include <string.h>
#include <holy/i18n.h>

void *
holy_malloc (holy_size_t size)
{
  void *ret;
  ret = malloc (size);
  if (!ret)
    holy_error (holy_ERR_OUT_OF_MEMORY, N_("out of memory"));
  return ret;
}

void *
holy_zalloc (holy_size_t size)
{
  void *ret;

  ret = holy_malloc (size);
  if (!ret)
    return NULL;
  memset (ret, 0, size);
  return ret;
}

void
holy_free (void *ptr)
{
  free (ptr);
}

void *
holy_realloc (void *ptr, holy_size_t size)
{
  void *ret;
  ret = realloc (ptr, size);
  if (!ret)
    holy_error (holy_ERR_OUT_OF_MEMORY, N_("out of memory"));
  return ret;
}
