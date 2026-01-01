/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_GPG_ERROR_H
#define holy_GPG_ERROR_H 1

#include <holy/crypto.h>
typedef enum
  {
    GPG_ERR_SOURCE_USER_1
  }
  gpg_err_source_t;
#define GPG_ERR_INLINE inline
static inline int
gpg_err_make (gpg_err_source_t source __attribute__ ((unused)), gpg_err_code_t code)
{
  return code;
}

static inline gpg_err_code_t
gpg_err_code (gpg_error_t err)
{
  return err;
}

static inline gpg_err_source_t
gpg_err_source (gpg_error_t err __attribute__ ((unused)))
{
  return GPG_ERR_SOURCE_USER_1;
}

gcry_err_code_t
gpg_error_from_syserror (void);

#endif
