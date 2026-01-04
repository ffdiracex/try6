/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define holy_TYPES_CPU_HEADER	1

#ifdef __i386__
# define SIZEOF_VOID_P	4
# define SIZEOF_LONG	4
# define holy_TARGET_SIZEOF_VOID_P	4
# define holy_TARGET_SIZEOF_LONG	4
# define EFI_FUNC(x) x
#elif defined (__x86_64__)
# define SIZEOF_VOID_P	8
# define SIZEOF_LONG	8
# define holy_TARGET_SIZEOF_VOID_P	8
# define holy_TARGET_SIZEOF_LONG	8
# define EFI_FUNC(x) x ## _real
#else
#error "Unknown architecture"
#endif
