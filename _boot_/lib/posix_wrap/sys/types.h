/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_POSIX_SYS_TYPES_H
#define holy_POSIX_SYS_TYPES_H	1

#include <holy/misc.h>

#include <stddef.h>

typedef holy_ssize_t ssize_t;
#ifndef holy_POSIX_BOOL_DEFINED
typedef enum { false = 0, true = 1 } bool;
#define holy_POSIX_BOOL_DEFINED 1
#endif

typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long long int holy_uint64_t;

typedef unsigned long long int holy_addr_t;

typedef holy_uint8_t uint8_t;
typedef holy_uint16_t uint16_t;
typedef holy_uint32_t uint32_t;
typedef holy_uint64_t uint64_t;

typedef holy_int8_t int8_t;
typedef holy_int16_t int16_t;
typedef holy_int32_t int32_t;
typedef holy_int64_t int64_t;

#define HAVE_U64_TYPEDEF 1
typedef holy_uint64_t u64;
#define HAVE_U32_TYPEDEF 1
typedef holy_uint32_t u32;
#define HAVE_U16_TYPEDEF 1
typedef holy_uint16_t u16;
#define HAVE_BYTE_TYPEDEF 1
typedef holy_uint8_t byte;

typedef holy_addr_t uintptr_t;

#define SIZEOF_UNSIGNED_LONG holy_CPU_SIZEOF_LONG
#define SIZEOF_UNSIGNED_INT 4
#define SIZEOF_UNSIGNED_LONG_LONG 8
#define SIZEOF_UNSIGNED_SHORT 2
#define SIZEOF_UINT64_T 8

#ifdef holy_CPU_WORDS_BIGENDIAN
#define WORDS_BIGENDIAN 1
#else
#undef WORDS_BIGENDIAN
#endif

#endif
