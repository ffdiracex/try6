/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_POSIX_ERRNO_H
#define holy_POSIX_ERRNO_H	1

#include <holy/err.h>

#undef errno
#define errno  holy_errno
#define EINVAL holy_ERR_BAD_NUMBER
#define ENOMEM holy_ERR_OUT_OF_MEMORY

#endif
