/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_RANDOM_HEADER
#define holy_RANDOM_HEADER 1

#include <holy/types.h>
#include <holy/err.h>

/* Not peer-reviewed. May not be any better than string of zeros.  */
holy_err_t
holy_crypto_get_random (void *buffer, holy_size_t sz);

/* Do not use directly. Use holy_crypto_get_random instead.  */
int
holy_crypto_arch_get_random (void *buffer, holy_size_t sz);

#endif
