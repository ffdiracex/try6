/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/gcrypt/g10lib.h>
#include <holy/gcrypt/gpg-error.h>
#include <holy/term.h>
#include <holy/crypto.h>
#include <holy/dl.h>
#include <holy/env.h>

holy_MOD_LICENSE ("GPLv2+");

void *
gcry_malloc (size_t n)
{
  return holy_malloc (n);
}

void *
gcry_malloc_secure (size_t n)
{
  return holy_malloc (n);
}

void
gcry_free (void *a)
{
  holy_free (a);
}

int
gcry_is_secure (const void *a __attribute__ ((unused)))
{
  return 0;
}

/* FIXME: implement "exit".  */
void *
gcry_xcalloc (size_t n, size_t m)
{
  void *ret;
  ret = holy_zalloc (n * m);
  if (!ret)
    holy_fatal ("gcry_xcalloc failed");
  return ret;
}

void *
gcry_xmalloc_secure (size_t n)
{
  void *ret;
  ret = holy_malloc (n);
  if (!ret)
    holy_fatal ("gcry_xmalloc failed");
  return ret;
}

void *
gcry_xcalloc_secure (size_t n, size_t m)
{
  void *ret;
  ret = holy_zalloc (n * m);
  if (!ret)
    holy_fatal ("gcry_xcalloc failed");
  return ret;
}

void *
gcry_xmalloc (size_t n)
{
  void *ret;
  ret = holy_malloc (n);
  if (!ret)
    holy_fatal ("gcry_xmalloc failed");
  return ret;
}

void *
gcry_xrealloc (void *a, size_t n)
{
  void *ret;
  ret = holy_realloc (a, n);
  if (!ret)
    holy_fatal ("gcry_xrealloc failed");
  return ret;
}

void
_gcry_check_heap (const void *a __attribute__ ((unused)))
{

}

void _gcry_log_printf (const char *fmt, ...)
{
  va_list args;
  const char *debug = holy_env_get ("debug");

  if (! debug)
    return;

  if (holy_strword (debug, "all") || holy_strword (debug, "gcrypt"))
    {
      holy_printf ("gcrypt: ");
      va_start (args, fmt);
      holy_vprintf (fmt, args);
      va_end (args);
      holy_refresh ();
    }
}

void _gcry_log_bug (const char *fmt, ...)
{
  va_list args;

  holy_printf ("gcrypt bug: ");
  va_start (args, fmt);
  holy_vprintf (fmt, args);
  va_end (args);
  holy_refresh ();
  holy_fatal ("gcrypt bug");
}

gcry_err_code_t
gpg_error_from_syserror (void)
{
  switch (holy_errno)
    {
    case holy_ERR_NONE:
      return GPG_ERR_NO_ERROR;
    case holy_ERR_OUT_OF_MEMORY:
      return GPG_ERR_OUT_OF_MEMORY;
    default:
      return GPG_ERR_GENERAL;
    }
}
