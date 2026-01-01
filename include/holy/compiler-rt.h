/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_COMPILER_RT_HEADER
#define holy_COMPILER_RT_HEADER	1

#include <stdarg.h>
#include <holy/types.h>
#include <holy/symbol.h>
#include <holy/misc.h>

#if defined(holy_DIVISION_IN_SOFTWARE) && holy_DIVISION_IN_SOFTWARE

holy_uint32_t
EXPORT_FUNC (__udivsi3) (holy_uint32_t a, holy_uint32_t b);

holy_uint32_t
EXPORT_FUNC (__umodsi3) (holy_uint32_t a, holy_uint32_t b);

holy_int32_t
EXPORT_FUNC (__divsi3) (holy_int32_t a, holy_int32_t b);

holy_int32_t
EXPORT_FUNC (__modsi3) (holy_int32_t a, holy_int32_t b);

holy_int64_t
EXPORT_FUNC (__divdi3) (holy_int64_t a, holy_int64_t b);

holy_int64_t
EXPORT_FUNC (__moddi3) (holy_int64_t a, holy_int64_t b);

holy_uint64_t
EXPORT_FUNC (__udivdi3) (holy_uint64_t a, holy_uint64_t b);

holy_uint64_t
EXPORT_FUNC (__umoddi3) (holy_uint64_t a, holy_uint64_t b);

#endif

#if defined (__sparc__) || defined (__powerpc__) || defined (__mips__) || defined (__arm__)
unsigned
EXPORT_FUNC (__ctzdi2) (holy_uint64_t x);
#define NEED_CTZDI2 1
#endif

#if defined (__mips__) || defined (__arm__)
unsigned
EXPORT_FUNC (__ctzsi2) (holy_uint32_t x);
#define NEED_CTZSI2 1
#endif

#ifdef __arm__
holy_uint32_t
EXPORT_FUNC (__aeabi_uidiv) (holy_uint32_t a, holy_uint32_t b);
holy_uint32_t
EXPORT_FUNC (__aeabi_uidivmod) (holy_uint32_t a, holy_uint32_t b);

holy_int32_t
EXPORT_FUNC (__aeabi_idiv) (holy_int32_t a, holy_int32_t b);
holy_int32_t
EXPORT_FUNC (__aeabi_idivmod) (holy_int32_t a, holy_int32_t b);

int
EXPORT_FUNC (__aeabi_ulcmp) (holy_uint64_t a, holy_uint64_t b);

/* Needed for allowing modules to be compiled as thumb.  */
holy_uint64_t
EXPORT_FUNC (__muldi3) (holy_uint64_t a, holy_uint64_t b);
holy_uint64_t
EXPORT_FUNC (__aeabi_lmul) (holy_uint64_t a, holy_uint64_t b);

void *
EXPORT_FUNC (__aeabi_memcpy) (void *dest, const void *src, holy_size_t n);
void *
EXPORT_FUNC (__aeabi_memcpy4) (void *dest, const void *src, holy_size_t n);
void *
EXPORT_FUNC (__aeabi_memcpy8) (void *dest, const void *src, holy_size_t n);
void *
EXPORT_FUNC(__aeabi_memset) (void *s, int c, holy_size_t n);
void EXPORT_FUNC(__aeabi_memclr) (void *s, holy_size_t n);
void EXPORT_FUNC(__aeabi_memclr4) (void *s, holy_size_t n);
void EXPORT_FUNC(__aeabi_memclr8) (void *s, holy_size_t n);

holy_uint64_t
EXPORT_FUNC (__aeabi_lasr) (holy_uint64_t u, int b);

holy_uint64_t
EXPORT_FUNC (__aeabi_llsl) (holy_uint64_t u, int b);

holy_uint64_t
EXPORT_FUNC (__aeabi_llsr) (holy_uint64_t u, int b);

#endif


#if defined (__powerpc__)

void EXPORT_FUNC (_restgpr_14_x) (void);
void EXPORT_FUNC (_restgpr_15_x) (void);
void EXPORT_FUNC (_restgpr_16_x) (void);
void EXPORT_FUNC (_restgpr_17_x) (void);
void EXPORT_FUNC (_restgpr_18_x) (void);
void EXPORT_FUNC (_restgpr_19_x) (void);
void EXPORT_FUNC (_restgpr_20_x) (void);
void EXPORT_FUNC (_restgpr_21_x) (void);
void EXPORT_FUNC (_restgpr_22_x) (void);
void EXPORT_FUNC (_restgpr_23_x) (void);
void EXPORT_FUNC (_restgpr_24_x) (void);
void EXPORT_FUNC (_restgpr_25_x) (void);
void EXPORT_FUNC (_restgpr_26_x) (void);
void EXPORT_FUNC (_restgpr_27_x) (void);
void EXPORT_FUNC (_restgpr_28_x) (void);
void EXPORT_FUNC (_restgpr_29_x) (void);
void EXPORT_FUNC (_restgpr_30_x) (void);
void EXPORT_FUNC (_restgpr_31_x) (void);
void EXPORT_FUNC (_savegpr_14) (void);
void EXPORT_FUNC (_savegpr_15) (void);
void EXPORT_FUNC (_savegpr_16) (void);
void EXPORT_FUNC (_savegpr_17) (void);
void EXPORT_FUNC (_savegpr_18) (void);
void EXPORT_FUNC (_savegpr_19) (void);
void EXPORT_FUNC (_savegpr_20) (void);
void EXPORT_FUNC (_savegpr_21) (void);
void EXPORT_FUNC (_savegpr_22) (void);
void EXPORT_FUNC (_savegpr_23) (void);
void EXPORT_FUNC (_savegpr_24) (void);
void EXPORT_FUNC (_savegpr_25) (void);
void EXPORT_FUNC (_savegpr_26) (void);
void EXPORT_FUNC (_savegpr_27) (void);
void EXPORT_FUNC (_savegpr_28) (void);
void EXPORT_FUNC (_savegpr_29) (void);
void EXPORT_FUNC (_savegpr_30) (void);
void EXPORT_FUNC (_savegpr_31) (void);

#endif

#if defined (__powerpc__) || defined(__mips__) || defined (__arm__)

int
EXPORT_FUNC(__ucmpdi2) (holy_uint64_t a, holy_uint64_t b);

holy_uint64_t
EXPORT_FUNC(__ashldi3) (holy_uint64_t u, int b);

holy_uint64_t
EXPORT_FUNC(__ashrdi3) (holy_uint64_t u, int b);

holy_uint64_t
EXPORT_FUNC (__lshrdi3) (holy_uint64_t u, int b);
#endif

#if defined (__powerpc__) || defined(__mips__) || defined(__sparc__) || defined (__arm__)
holy_uint32_t
EXPORT_FUNC(__bswapsi2) (holy_uint32_t u);

holy_uint64_t
EXPORT_FUNC(__bswapdi2) (holy_uint64_t u);
#endif

#if defined (__APPLE__) && defined(__i386__)
#define holy_BUILTIN_ATTR  __attribute__ ((regparm(0)))
#else
#define holy_BUILTIN_ATTR
#endif

/* Prototypes for aliases.  */
int holy_BUILTIN_ATTR EXPORT_FUNC(memcmp) (const void *s1, const void *s2, holy_size_t n);
void *holy_BUILTIN_ATTR EXPORT_FUNC(memmove) (void *dest, const void *src, holy_size_t n);
void *holy_BUILTIN_ATTR EXPORT_FUNC(memcpy) (void *dest, const void *src, holy_size_t n);
void *holy_BUILTIN_ATTR EXPORT_FUNC(memset) (void *s, int c, holy_size_t n);

#ifdef __APPLE__
void holy_BUILTIN_ATTR EXPORT_FUNC (__bzero) (void *s, holy_size_t n);
#endif

#if defined (__MINGW32__) || defined (__CYGWIN__)
void EXPORT_FUNC (__register_frame_info) (void);
void EXPORT_FUNC (__deregister_frame_info) (void);
void EXPORT_FUNC (___chkstk_ms) (void);
void EXPORT_FUNC (__chkstk_ms) (void);
#endif

#endif

