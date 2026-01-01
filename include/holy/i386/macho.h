/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_CPU_MACHO_H
#define holy_CPU_MACHO_H 1

#include <holy/macho.h>

#define holy_MACHO_CPUTYPE_IS_HOST32(x) ((x) == holy_MACHO_CPUTYPE_IA32)
#define holy_MACHO_CPUTYPE_IS_HOST64(x) ((x) == holy_MACHO_CPUTYPE_AMD64)
#ifdef __x86_64__
#define holy_MACHO_CPUTYPE_IS_HOST_CURRENT(x) ((x) == holy_MACHO_CPUTYPE_AMD64)
#else
#define holy_MACHO_CPUTYPE_IS_HOST_CURRENT(x) ((x) == holy_MACHO_CPUTYPE_IA32)
#endif

#endif
