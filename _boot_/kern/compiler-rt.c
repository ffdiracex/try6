/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/compiler-rt.h>

#ifndef holy_EMBED_DECOMPRESSOR
void * holy_BUILTIN_ATTR
memcpy (void *dest, const void *src, holy_size_t n)
{
	return holy_memmove (dest, src, n);
}
void * holy_BUILTIN_ATTR
memmove (void *dest, const void *src, holy_size_t n)
{
	return holy_memmove (dest, src, n);
}
int holy_BUILTIN_ATTR
memcmp (const void *s1, const void *s2, holy_size_t n)
{
  return holy_memcmp (s1, s2, n);
}
void * holy_BUILTIN_ATTR
memset (void *s, int c, holy_size_t n)
{
  return holy_memset (s, c, n);
}

#ifdef __APPLE__

void holy_BUILTIN_ATTR
__bzero (void *s, holy_size_t n)
{
  holy_memset (s, 0, n);
}

#endif

#if holy_DIVISION_IN_SOFTWARE

holy_uint32_t
__udivsi3 (holy_uint32_t a, holy_uint32_t b)
{
  return holy_divmod64 (a, b, 0);
}

holy_int32_t
__divsi3 (holy_int32_t a, holy_int32_t b)
{
  return holy_divmod64s (a, b, 0);
}

holy_uint32_t
__umodsi3 (holy_uint32_t a, holy_uint32_t b)
{
  holy_uint64_t ret;
  holy_divmod64 (a, b, &ret);
  return ret;
}

holy_int32_t
__modsi3 (holy_int32_t a, holy_int32_t b)
{
  holy_int64_t ret;
  holy_divmod64s (a, b, &ret);
  return ret;
}

holy_uint64_t
__udivdi3 (holy_uint64_t a, holy_uint64_t b)
{
  return holy_divmod64 (a, b, 0);
}

holy_uint64_t
__umoddi3 (holy_uint64_t a, holy_uint64_t b)
{
  holy_uint64_t ret;
  holy_divmod64 (a, b, &ret);
  return ret;
}

holy_int64_t
__divdi3 (holy_int64_t a, holy_int64_t b)
{
  return holy_divmod64s (a, b, 0);
}

holy_int64_t
__moddi3 (holy_int64_t a, holy_int64_t b)
{
  holy_int64_t ret;
  holy_divmod64s (a, b, &ret);
  return ret;
}

#endif

#endif

#ifdef NEED_CTZDI2

unsigned
__ctzdi2 (holy_uint64_t x)
{
  unsigned ret = 0;
  if (!x)
    return 64;
  if (!(x & 0xffffffff))
    {
      x >>= 32;
      ret |= 32;
    }
  if (!(x & 0xffff))
    {
      x >>= 16;
      ret |= 16;
    }
  if (!(x & 0xff))
    {
      x >>= 8;
      ret |= 8;
    }
  if (!(x & 0xf))
    {
      x >>= 4;
      ret |= 4;
    }
  if (!(x & 0x3))
    {
      x >>= 2;
      ret |= 2;
    }
  if (!(x & 0x1))
    {
      x >>= 1;
      ret |= 1;
    }
  return ret;
}
#endif

#ifdef NEED_CTZSI2
unsigned
__ctzsi2 (holy_uint32_t x)
{
  unsigned ret = 0;
  if (!x)
    return 32;

  if (!(x & 0xffff))
    {
      x >>= 16;
      ret |= 16;
    }
  if (!(x & 0xff))
    {
      x >>= 8;
      ret |= 8;
    }
  if (!(x & 0xf))
    {
      x >>= 4;
      ret |= 4;
    }
  if (!(x & 0x3))
    {
      x >>= 2;
      ret |= 2;
    }
  if (!(x & 0x1))
    {
      x >>= 1;
      ret |= 1;
    }
  return ret;
}

#endif


#if defined (__clang__) && !defined(holy_EMBED_DECOMPRESSOR)
/* clang emits references to abort().  */
void __attribute__ ((noreturn))
abort (void)
{
  holy_fatal ("compiler abort");
}
#endif

#if (defined (__MINGW32__) || defined (__CYGWIN__))
void __register_frame_info (void)
{
}

void __deregister_frame_info (void)
{
}

void ___chkstk_ms (void)
{
}

void __chkstk_ms (void)
{
}
#endif

union component64
{
  holy_uint64_t full;
  struct
  {
#ifdef holy_CPU_WORDS_BIGENDIAN
    holy_uint32_t high;
    holy_uint32_t low;
#else
    holy_uint32_t low;
    holy_uint32_t high;
#endif
  };
};

#if defined (__powerpc__) || defined (__arm__) || defined(__mips__)

/* Based on libgcc2.c from gcc suite.  */
holy_uint64_t
__lshrdi3 (holy_uint64_t u, int b)
{
  if (b == 0)
    return u;

  const union component64 uu = {.full = u};
  const int bm = 32 - b;
  union component64 w;

  if (bm <= 0)
    {
      w.high = 0;
      w.low = (holy_uint32_t) uu.high >> -bm;
    }
  else
    {
      const holy_uint32_t carries = (holy_uint32_t) uu.high << bm;

      w.high = (holy_uint32_t) uu.high >> b;
      w.low = ((holy_uint32_t) uu.low >> b) | carries;
    }

  return w.full;
}

/* Based on libgcc2.c from gcc suite.  */
holy_uint64_t
__ashrdi3 (holy_uint64_t u, int b)
{
  if (b == 0)
    return u;

  const union component64 uu = {.full = u};
  const int bm = 32 - b;
  union component64 w;

  if (bm <= 0)
    {
      /* w.high = 1..1 or 0..0 */
      w.high = ((holy_int32_t) uu.high) >> (32 - 1);
      w.low = ((holy_int32_t) uu.high) >> -bm;
    }
  else
    {
      const holy_uint32_t carries = ((holy_uint32_t) uu.high) << bm;

      w.high = ((holy_int32_t) uu.high) >> b;
      w.low = ((holy_uint32_t) uu.low >> b) | carries;
    }

  return w.full;
}

/* Based on libgcc2.c from gcc suite.  */
holy_uint64_t
__ashldi3 (holy_uint64_t u, int b)
{
  if (b == 0)
    return u;

  const union component64 uu = {.full = u};
  const int bm = 32 - b;
  union component64 w;

  if (bm <= 0)
    {
      w.low = 0;
      w.high = (holy_uint32_t) uu.low << -bm;
    }
  else
    {
      const holy_uint32_t carries = (holy_uint32_t) uu.low >> bm;

      w.low = (holy_uint32_t) uu.low << b;
      w.high = ((holy_uint32_t) uu.high << b) | carries;
    }

  return w.full;
}

/* Based on libgcc2.c from gcc suite.  */
int
__ucmpdi2 (holy_uint64_t a, holy_uint64_t b)
{
  union component64 ac, bc;
  ac.full = a;
  bc.full = b;

  if (ac.high < bc.high)
    return 0;
  else if (ac.high > bc.high)
    return 2;

  if (ac.low < bc.low)
    return 0;
  else if (ac.low > bc.low)
    return 2;
  return 1;
}

#endif

#if defined (__powerpc__) || defined(__mips__) || defined(__sparc__) || defined(__arm__)

/* Based on libgcc2.c from gcc suite.  */
holy_uint32_t
__bswapsi2 (holy_uint32_t u)
{
  return ((((u) & 0xff000000) >> 24)
	  | (((u) & 0x00ff0000) >>  8)
	  | (((u) & 0x0000ff00) <<  8)
	  | (((u) & 0x000000ff) << 24));
}

/* Based on libgcc2.c from gcc suite.  */
holy_uint64_t
__bswapdi2 (holy_uint64_t u)
{
  return ((((u) & 0xff00000000000000ull) >> 56)
	  | (((u) & 0x00ff000000000000ull) >> 40)
	  | (((u) & 0x0000ff0000000000ull) >> 24)
	  | (((u) & 0x000000ff00000000ull) >>  8)
	  | (((u) & 0x00000000ff000000ull) <<  8)
	  | (((u) & 0x0000000000ff0000ull) << 24)
	  | (((u) & 0x000000000000ff00ull) << 40)
	  | (((u) & 0x00000000000000ffull) << 56));
}


#endif

#ifdef __arm__
holy_uint32_t
__aeabi_uidiv (holy_uint32_t a, holy_uint32_t b)
  __attribute__ ((alias ("__udivsi3")));
holy_int32_t
__aeabi_idiv (holy_int32_t a, holy_int32_t b)
  __attribute__ ((alias ("__divsi3")));
void *__aeabi_memcpy (void *dest, const void *src, holy_size_t n)
  __attribute__ ((alias ("holy_memcpy")));
void *__aeabi_memcpy4 (void *dest, const void *src, holy_size_t n)
  __attribute__ ((alias ("holy_memcpy")));
void *__aeabi_memcpy8 (void *dest, const void *src, holy_size_t n)
  __attribute__ ((alias ("holy_memcpy")));
void *__aeabi_memset (void *s, int c, holy_size_t n)
  __attribute__ ((alias ("memset")));

void
__aeabi_memclr (void *s, holy_size_t n)
{
  holy_memset (s, 0, n);
}

void __aeabi_memclr4 (void *s, holy_size_t n)
  __attribute__ ((alias ("__aeabi_memclr")));
void __aeabi_memclr8 (void *s, holy_size_t n)
  __attribute__ ((alias ("__aeabi_memclr")));

int
__aeabi_ulcmp (holy_uint64_t a, holy_uint64_t b)
{
  return __ucmpdi2 (a, b) - 1;
}

holy_uint64_t
__aeabi_lasr (holy_uint64_t u, int b)
  __attribute__ ((alias ("__ashrdi3")));
holy_uint64_t
__aeabi_llsr (holy_uint64_t u, int b)
  __attribute__ ((alias ("__lshrdi3")));

holy_uint64_t
__aeabi_llsl (holy_uint64_t u, int b)
  __attribute__ ((alias ("__ashldi3")));

#endif
