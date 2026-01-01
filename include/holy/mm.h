/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MM_H
#define holy_MM_H	1

#include <holy/types.h>
#include <holy/symbol.h>
#include <config.h>

#ifndef NULL
# define NULL	((void *) 0)
#endif

typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long long int holy_int64_t;
typedef holy_int64_t holy_ssize;

typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long long int holy_uint64_t;
typedef holy_uint64_t holy_size_t;


void holy_mm_init_region (void *addr, holy_size_t size);
void *EXPORT_FUNC(holy_malloc) (holy_size_t size);
void *EXPORT_FUNC(holy_zalloc) (holy_size_t size);
void EXPORT_FUNC(holy_free) (void *ptr);
void *EXPORT_FUNC(holy_realloc) (void *ptr, holy_size_t size);
#ifndef holy_MACHINE_EMU
void *EXPORT_FUNC(holy_memalign) (holy_size_t align, holy_size_t size);
#endif

void holy_mm_check_real (const char *file, int line);
#define holy_mm_check() holy_mm_check_real (holy_FILE, __LINE__);

/* For debugging.  */
#if defined(MM_DEBUG) && !defined(holy_UTIL) && !defined (holy_MACHINE_EMU)
/* Set this variable to 1 when you want to trace all memory function calls.  */
extern int EXPORT_VAR(holy_mm_debug);

void holy_mm_dump_free (void);
void holy_mm_dump (unsigned lineno);

#define holy_malloc(size)	\
  holy_debug_malloc (holy_FILE, __LINE__, size)

#define holy_zalloc(size)	\
  holy_debug_zalloc (holy_FILE, __LINE__, size)

#define holy_realloc(ptr,size)	\
  holy_debug_realloc (holy_FILE, __LINE__, ptr, size)

#define holy_memalign(align,size)	\
  holy_debug_memalign (holy_FILE, __LINE__, align, size)

#define holy_free(ptr)	\
  holy_debug_free (holy_FILE, __LINE__, ptr)

void *EXPORT_FUNC(holy_debug_malloc) (const char *file, int line,
				      holy_size_t size);
void *EXPORT_FUNC(holy_debug_zalloc) (const char *file, int line,
				       holy_size_t size);
void EXPORT_FUNC(holy_debug_free) (const char *file, int line, void *ptr);
void *EXPORT_FUNC(holy_debug_realloc) (const char *file, int line, void *ptr,
				       holy_size_t size);
void *EXPORT_FUNC(holy_debug_memalign) (const char *file, int line,
					holy_size_t align, holy_size_t size);
#endif /* MM_DEBUG && ! holy_UTIL */

#endif /* ! holy_MM_H */
