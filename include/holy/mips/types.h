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

#ifdef holy_CPU_MIPSEL
/* mipsEL is little-endian.  */
#undef holy_TARGET_WORDS_BIGENDIAN
#elif defined (holy_CPU_MIPS)
/* mips is big-endian.  */
#define holy_TARGET_WORDS_BIGENDIAN
#elif !defined (holy_SYMBOL_GENERATOR)
#error Neither holy_CPU_MIPS nor holy_CPU_MIPSEL is defined
#endif

#endif /* ! holy_TYPES_CPU_HEADER */
