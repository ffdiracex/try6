/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_GCRY_TYPES_HEADER
#define holy_GCRY_TYPES_HEADER 1

#include <holy/types.h>
#include <holy/misc.h>

#ifdef holy_CPU_WORDS_BIGENDIAN
#define WORDS_BIGENDIAN
#else
#undef WORDS_BIGENDIAN
#endif

typedef holy_uint64_t u64;
typedef holy_uint32_t u32;
typedef holy_uint16_t u16;
typedef holy_uint8_t byte;
typedef holy_size_t size_t;

#endif
