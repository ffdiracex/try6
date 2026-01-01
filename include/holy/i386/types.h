/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_TYPES_CPU_HEADER
#define holy_TYPES_CPU_HEADER	1

/* The size of void *.  */
#define holy_TARGET_SIZEOF_VOID_P	4

/* The size of long.  */
#define holy_TARGET_SIZEOF_LONG		4

/* i386 is little-endian.  */
#undef holy_TARGET_WORDS_BIGENDIAN

#define holy_HAVE_UNALIGNED_ACCESS 1

#endif /* ! holy_TYPES_CPU_HEADER */
