/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_TYPES_HEADER
#define holy_TYPES_HEADER	1

#include <holy/emu/config.h>
#ifndef holy_UTIL
#include <holy/cpu/types.h>
#endif

#ifdef __MINGW32__
#define holy_PACKED __attribute__ ((packed,gcc_struct))
#else
#define holy_PACKED __attribute__ ((packed))
#endif

#ifdef holy_BUILD
# define holy_CPU_SIZEOF_VOID_P	BUILD_SIZEOF_VOID_P
# define holy_CPU_SIZEOF_LONG	BUILD_SIZEOF_LONG
# if BUILD_WORDS_BIGENDIAN
#  define holy_CPU_WORDS_BIGENDIAN	1
# else
#  undef holy_CPU_WORDS_BIGENDIAN
# endif
#elif defined (holy_UTIL)
# define holy_CPU_SIZEOF_VOID_P	SIZEOF_VOID_P
# define holy_CPU_SIZEOF_LONG	SIZEOF_LONG
# ifdef WORDS_BIGENDIAN
#  define holy_CPU_WORDS_BIGENDIAN	1
# else
#  undef holy_CPU_WORDS_BIGENDIAN
# endif
#else /* ! holy_UTIL */
# define holy_CPU_SIZEOF_VOID_P	holy_TARGET_SIZEOF_VOID_P
# define holy_CPU_SIZEOF_LONG	holy_TARGET_SIZEOF_LONG
# ifdef holy_TARGET_WORDS_BIGENDIAN
#  define holy_CPU_WORDS_BIGENDIAN	1
# else
#  undef holy_CPU_WORDS_BIGENDIAN
# endif
#endif /* ! holy_UTIL */

#if holy_CPU_SIZEOF_VOID_P != 4 && holy_CPU_SIZEOF_VOID_P != 8
# error "This architecture is not supported because sizeof(void *) != 4 and sizeof(void *) != 8"
#endif

#if holy_CPU_SIZEOF_LONG != 4 && holy_CPU_SIZEOF_LONG != 8
# error "This architecture is not supported because sizeof(long) != 4 and sizeof(long) != 8"
#endif

#if !defined (holy_UTIL) && !defined (holy_TARGET_WORDSIZE)
# if holy_TARGET_SIZEOF_VOID_P == 4
#  define holy_TARGET_WORDSIZE 32
# elif holy_TARGET_SIZEOF_VOID_P == 8
#  define holy_TARGET_WORDSIZE 64
# endif
#endif

/* Define various wide integers.  */
typedef signed char		holy_int8_t;
typedef short			holy_int16_t;
typedef int			holy_int32_t;
#if holy_CPU_SIZEOF_LONG == 8
typedef long			holy_int64_t;
#else
typedef long long		holy_int64_t;
#endif

typedef unsigned char		holy_uint8_t;
typedef unsigned short		holy_uint16_t;
typedef unsigned		holy_uint32_t;
# define PRIxholy_UINT32_T	"x"
# define PRIuholy_UINT32_T	"u"
#if holy_CPU_SIZEOF_LONG == 8
typedef unsigned long		holy_uint64_t;
# define PRIxholy_UINT64_T	"lx"
# define PRIuholy_UINT64_T	"lu"
#else
typedef unsigned long long	holy_uint64_t;
# define PRIxholy_UINT64_T	"llx"
# define PRIuholy_UINT64_T	"llu"
#endif

/* Misc types.  */

#if holy_CPU_SIZEOF_VOID_P == 8
typedef holy_uint64_t	holy_addr_t;
typedef holy_uint64_t	holy_size_t;
typedef holy_int64_t	holy_ssize_t;

# define holy_SIZE_MAX 18446744073709551615UL

# if holy_CPU_SIZEOF_LONG == 8
#  define PRIxholy_SIZE	 "lx"
#  define PRIxholy_ADDR	 "lx"
#  define PRIuholy_SIZE	 "lu"
#  define PRIdholy_SSIZE "ld"
# else
#  define PRIxholy_SIZE	 "llx"
#  define PRIxholy_ADDR	 "llx"
#  define PRIuholy_SIZE  "llu"
#  define PRIdholy_SSIZE "lld"
# endif
#else
typedef holy_uint32_t	holy_addr_t;
typedef holy_uint32_t	holy_size_t;
typedef holy_int32_t	holy_ssize_t;

# define holy_SIZE_MAX 4294967295UL

# define PRIxholy_SIZE	"x"
# define PRIxholy_ADDR	"x"
# define PRIuholy_SIZE	"u"
# define PRIdholy_SSIZE	"d"
#endif

#define holy_UCHAR_MAX 0xFF
#define holy_USHRT_MAX 65535
#define holy_SHRT_MAX 0x7fff
#define holy_UINT_MAX 4294967295U
#define holy_INT_MAX 0x7fffffff
#define holy_INT32_MIN (-2147483647 - 1)
#define holy_INT32_MAX 2147483647

#if holy_CPU_SIZEOF_LONG == 8
# define holy_ULONG_MAX 18446744073709551615UL
# define holy_LONG_MAX 9223372036854775807L
# define holy_LONG_MIN (-9223372036854775807L - 1)
#else
# define holy_ULONG_MAX 4294967295UL
# define holy_LONG_MAX 2147483647L
# define holy_LONG_MIN (-2147483647L - 1)
#endif

typedef holy_uint64_t holy_properly_aligned_t;

#define holy_PROPERLY_ALIGNED_ARRAY(name, size) holy_properly_aligned_t name[((size) + sizeof (holy_properly_aligned_t) - 1) / sizeof (holy_properly_aligned_t)]

/* The type for representing a file offset.  */
typedef holy_uint64_t	holy_off_t;

/* The type for representing a disk block address.  */
typedef holy_uint64_t	holy_disk_addr_t;

/* Byte-orders.  */
static inline holy_uint16_t holy_swap_bytes16(holy_uint16_t _x)
{
   return (holy_uint16_t) ((_x << 8) | (_x >> 8));
}

#define holy_swap_bytes16_compile_time(x) ((((x) & 0xff) << 8) | (((x) & 0xff00) >> 8))
#define holy_swap_bytes32_compile_time(x) ((((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) & 0xff000000UL) >> 24))
#define holy_swap_bytes64_compile_time(x)	\
({ \
   holy_uint64_t _x = (x); \
   (holy_uint64_t) ((_x << 56) \
                    | ((_x & (holy_uint64_t) 0xFF00ULL) << 40) \
                    | ((_x & (holy_uint64_t) 0xFF0000ULL) << 24) \
                    | ((_x & (holy_uint64_t) 0xFF000000ULL) << 8) \
                    | ((_x & (holy_uint64_t) 0xFF00000000ULL) >> 8) \
                    | ((_x & (holy_uint64_t) 0xFF0000000000ULL) >> 24) \
                    | ((_x & (holy_uint64_t) 0xFF000000000000ULL) >> 40) \
                    | (_x >> 56)); \
})

#if (defined(__GNUC__) && (__GNUC__ > 3) && (__GNUC__ > 4 || __GNUC_MINOR__ >= 3)) || defined(__clang__)
static inline holy_uint32_t holy_swap_bytes32(holy_uint32_t x)
{
	return __builtin_bswap32(x);
}

static inline holy_uint64_t holy_swap_bytes64(holy_uint64_t x)
{
	return __builtin_bswap64(x);
}
#else					/* not gcc 4.3 or newer */
static inline holy_uint32_t holy_swap_bytes32(holy_uint32_t _x)
{
   return ((_x << 24)
	   | ((_x & (holy_uint32_t) 0xFF00UL) << 8)
	   | ((_x & (holy_uint32_t) 0xFF0000UL) >> 8)
	   | (_x >> 24));
}

static inline holy_uint64_t holy_swap_bytes64(holy_uint64_t _x)
{
   return ((_x << 56)
	   | ((_x & (holy_uint64_t) 0xFF00ULL) << 40)
	   | ((_x & (holy_uint64_t) 0xFF0000ULL) << 24)
	   | ((_x & (holy_uint64_t) 0xFF000000ULL) << 8)
	   | ((_x & (holy_uint64_t) 0xFF00000000ULL) >> 8)
	   | ((_x & (holy_uint64_t) 0xFF0000000000ULL) >> 24)
	   | ((_x & (holy_uint64_t) 0xFF000000000000ULL) >> 40)
	   | (_x >> 56));
}
#endif					/* not gcc 4.3 or newer */

#ifdef holy_CPU_WORDS_BIGENDIAN
# define holy_cpu_to_le16(x)	holy_swap_bytes16(x)
# define holy_cpu_to_le32(x)	holy_swap_bytes32(x)
# define holy_cpu_to_le64(x)	holy_swap_bytes64(x)
# define holy_le_to_cpu16(x)	holy_swap_bytes16(x)
# define holy_le_to_cpu32(x)	holy_swap_bytes32(x)
# define holy_le_to_cpu64(x)	holy_swap_bytes64(x)
# define holy_cpu_to_be16(x)	((holy_uint16_t) (x))
# define holy_cpu_to_be32(x)	((holy_uint32_t) (x))
# define holy_cpu_to_be64(x)	((holy_uint64_t) (x))
# define holy_be_to_cpu16(x)	((holy_uint16_t) (x))
# define holy_be_to_cpu32(x)	((holy_uint32_t) (x))
# define holy_be_to_cpu64(x)	((holy_uint64_t) (x))
# define holy_cpu_to_be16_compile_time(x)	((holy_uint16_t) (x))
# define holy_cpu_to_be32_compile_time(x)	((holy_uint32_t) (x))
# define holy_cpu_to_be64_compile_time(x)	((holy_uint64_t) (x))
# define holy_be_to_cpu64_compile_time(x)	((holy_uint64_t) (x))
# define holy_cpu_to_le32_compile_time(x)	holy_swap_bytes32_compile_time(x)
# define holy_cpu_to_le64_compile_time(x)	holy_swap_bytes64_compile_time(x)
# define holy_cpu_to_le16_compile_time(x)	holy_swap_bytes16_compile_time(x)
#else /* ! WORDS_BIGENDIAN */
# define holy_cpu_to_le16(x)	((holy_uint16_t) (x))
# define holy_cpu_to_le32(x)	((holy_uint32_t) (x))
# define holy_cpu_to_le64(x)	((holy_uint64_t) (x))
# define holy_le_to_cpu16(x)	((holy_uint16_t) (x))
# define holy_le_to_cpu32(x)	((holy_uint32_t) (x))
# define holy_le_to_cpu64(x)	((holy_uint64_t) (x))
# define holy_cpu_to_be16(x)	holy_swap_bytes16(x)
# define holy_cpu_to_be32(x)	holy_swap_bytes32(x)
# define holy_cpu_to_be64(x)	holy_swap_bytes64(x)
# define holy_be_to_cpu16(x)	holy_swap_bytes16(x)
# define holy_be_to_cpu32(x)	holy_swap_bytes32(x)
# define holy_be_to_cpu64(x)	holy_swap_bytes64(x)
# define holy_cpu_to_be16_compile_time(x)	holy_swap_bytes16_compile_time(x)
# define holy_cpu_to_be32_compile_time(x)	holy_swap_bytes32_compile_time(x)
# define holy_cpu_to_be64_compile_time(x)	holy_swap_bytes64_compile_time(x)
# define holy_be_to_cpu64_compile_time(x)	holy_swap_bytes64_compile_time(x)
# define holy_cpu_to_le16_compile_time(x)	((holy_uint16_t) (x))
# define holy_cpu_to_le32_compile_time(x)	((holy_uint32_t) (x))
# define holy_cpu_to_le64_compile_time(x)	((holy_uint64_t) (x))

#endif /* ! WORDS_BIGENDIAN */

struct holy_unaligned_uint16
{
  holy_uint16_t val;
} holy_PACKED;
struct holy_unaligned_uint32
{
  holy_uint32_t val;
} holy_PACKED;
struct holy_unaligned_uint64
{
  holy_uint64_t val;
} holy_PACKED;

typedef struct holy_unaligned_uint16 holy_unaligned_uint16_t;
typedef struct holy_unaligned_uint32 holy_unaligned_uint32_t;
typedef struct holy_unaligned_uint64 holy_unaligned_uint64_t;

static inline holy_uint16_t holy_get_unaligned16 (const void *ptr)
{
  const struct holy_unaligned_uint16 *dd
    = (const struct holy_unaligned_uint16 *) ptr;
  return dd->val;
}

static inline void holy_set_unaligned16 (void *ptr, holy_uint16_t val)
{
  struct holy_unaligned_uint16 *dd = (struct holy_unaligned_uint16 *) ptr;
  dd->val = val;
}

static inline holy_uint32_t holy_get_unaligned32 (const void *ptr)
{
  const struct holy_unaligned_uint32 *dd
    = (const struct holy_unaligned_uint32 *) ptr;
  return dd->val;
}

static inline void holy_set_unaligned32 (void *ptr, holy_uint32_t val)
{
  struct holy_unaligned_uint32 *dd = (struct holy_unaligned_uint32 *) ptr;
  dd->val = val;
}

static inline holy_uint64_t holy_get_unaligned64 (const void *ptr)
{
  const struct holy_unaligned_uint64 *dd
    = (const struct holy_unaligned_uint64 *) ptr;
  return dd->val;
}

static inline void holy_set_unaligned64 (void *ptr, holy_uint64_t val)
{
  struct holy_unaligned_uint64_t
  {
    holy_uint64_t d;
  } holy_PACKED;
  struct holy_unaligned_uint64_t *dd = (struct holy_unaligned_uint64_t *) ptr;
  dd->d = val;
}

#define holy_CHAR_BIT 8

#endif /* ! holy_TYPES_HEADER */
