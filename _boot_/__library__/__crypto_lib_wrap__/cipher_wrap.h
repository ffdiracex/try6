/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_GCRY_WRAP_HEADER
#define holy_GCRY_WRAP_HEADER 1

#include <holy/types.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/dl.h>
#include <holy/crypto.h>

#include <sys/types.h>

#define _gcry_mpi_invm gcry_mpi_invm
#define _gcry_mpi_set gcry_mpi_set
#define _gcry_mpi_set_ui gcry_mpi_set_ui
#define size_t holy_size_t

#undef __GNU_LIBRARY__
#define __GNU_LIBRARY__ 1

#define U64_C(c) (c ## ULL)

#define PUBKEY_FLAG_NO_BLINDING    (1 << 0)

#define CIPHER_INFO_NO_WEAK_KEY    1

#define HAVE_U64_TYPEDEF 1

/* Selftests are in separate modules.  */
static inline char *
selftest (void)
{
  return NULL;
}

static inline int
_gcry_fips_mode (void)
{
  return 0;
}

#define assert gcry_assert

#ifdef holy_UTIL

#define memset holy_memset

#endif


#define DBG_CIPHER 0

#include <string.h>
#pragma GCC diagnostic ignored "-Wredundant-decls"
#include <holy/gcrypt/g10lib.h>
#include <holy/gcrypt/gcrypt.h>

#define gcry_mpi_mod _gcry_mpi_mod

#endif
