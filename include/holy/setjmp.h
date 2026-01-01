/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_SETJMP_HEADER
#define holy_SETJMP_HEADER	1

#if defined(holy_UTIL)
#include <setjmp.h>
typedef jmp_buf holy_jmp_buf;
#define holy_setjmp setjmp
#define holy_longjmp longjmp
#else

#include <holy/misc.h>

#if GNUC_PREREQ(4,0)
#define RETURNS_TWICE __attribute__ ((returns_twice))
#else
#define RETURNS_TWICE
#endif

/* This must define holy_jmp_buf, and declare holy_setjmp and
   holy_longjmp.  */
# include <holy/cpu/setjmp.h>
#endif

#endif /* ! holy_SETJMP_HEADER */
