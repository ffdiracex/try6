/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_READER_HEADER
#define holy_READER_HEADER	1

#include <holy/err.h>

typedef holy_err_t (*holy_reader_getline_t) (char **, int, void *);

void holy_rescue_run (void) __attribute__ ((noreturn));

#endif /* ! holy_READER_HEADER */
