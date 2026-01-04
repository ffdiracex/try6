/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_POSIX_ASSERT_H
#define holy_POSIX_ASSERT_H	1

#include <holy/misc.h>

#define assert(x) assert_real(__FILE__, __LINE__, x)

static inline void
assert_real (const char *file, int line, int cond)
{
  if (!cond)
    holy_printf ("Assertion failed at %s:%d\n", file, line);
}

#endif
