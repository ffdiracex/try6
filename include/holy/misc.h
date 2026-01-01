/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MISC_HEADER
#define holy_MISC_HEADER	1

#include <stdarg.h>
#include <holy/types.h>
#include <holy/symbol.h>
#include <holy/err.h>
#include <holy/i18n.h>
#include <holy/compiler.h>


#define holy_LONG_MIN (-9223372036854775807L - 1)
#define holy_LONG_MAX (9223372036854775807L)



#define ALIGN_UP(addr, align) \
	((addr + (typeof (addr)) align - 1) & ~((typeof (addr)) align - 1))
#define ALIGN_UP_OVERHEAD(addr, align) ((-(addr)) & ((typeof (addr)) (align) - 1))
#define ALIGN_DOWN(addr, align) \
	((addr) & ~((typeof (addr)) align - 1))
#define ARRAY_SIZE(array) (sizeof (array) / sizeof (array[0]))
#define COMPILE_TIME_ASSERT(cond) switch (0) { case 1: case !(cond): ; }

#define holy_dprintf(condition, ...) holy_real_dprintf(holy_FILE, __LINE__, condition, __VA_ARGS__)

typedef unsigned long long int holy_size_t;
typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long int holy_uint64_t;

typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long int holy_int64_t;
typedef holy_int64_t holy_ssize_t;


void *EXPORT_FUNC(holy_memmove) (void *dest, const void *src, holy_size_t n);
char *EXPORT_FUNC(holy_strcpy) (char *dest, const char *src);

static inline char *
holy_strncpy (char *dest, const char *src, int c)
{
  char *p = dest;

  while ((*p++ = *src++) != '\0' && --c)
    ;

  return dest;
}

static inline char *
holy_stpcpy (char *dest, const char *src)
{
  char *d = dest;
  const char *s = src;

  do
    *d++ = *s;
  while (*s++ != '\0');

  return d - 1;
}

/* XXX: If holy_memmove is too slow, we must implement holy_memcpy.  */
static inline void *
holy_memcpy (void *dest, const void *src, holy_size_t n)
{
  return holy_memmove (dest, src, n);
}

#if defined(__x86_64__) && !defined (holy_UTIL)
#if defined (__MINGW32__) || defined (__CYGWIN__) || defined (__MINGW64__)
#define holy_ASM_ATTR __attribute__ ((sysv_abi))
#else
#define holy_ASM_ATTR
#endif
#endif

int EXPORT_FUNC(holy_memcmp) (const void *s1, const void *s2, holy_size_t n);
int EXPORT_FUNC(holy_strcmp) (const char *s1, const char *s2);
int EXPORT_FUNC(holy_strncmp) (const char *s1, const char *s2, holy_size_t n);

char *EXPORT_FUNC(holy_strchr) (const char *s, int c);
char *EXPORT_FUNC(holy_strrchr) (const char *s, int c);
int EXPORT_FUNC(holy_strword) (const char *s, const char *w);

/* Copied from gnulib.
   Written by Bruno Haible <bruno@clisp.org>, 2005. */
static inline char *
holy_strstr (const char *haystack, const char *needle)
{
  /* Be careful not to look at the entire extent of haystack or needle
     until needed.  This is useful because of these two cases:
       - haystack may be very long, and a match of needle found early,
       - needle may be very long, and not even a short initial segment of
       needle may be found in haystack.  */
  if (*needle != '\0')
    {
      /* Speed up the following searches of needle by caching its first
	 character.  */
      char b = *needle++;

      for (;; haystack++)
	{
	  if (*haystack == '\0')
	    /* No match.  */
	    return 0;
	  if (*haystack == b)
	    /* The first character matches.  */
	    {
	      const char *rhaystack = haystack + 1;
	      const char *rneedle = needle;

	      for (;; rhaystack++, rneedle++)
		{
		  if (*rneedle == '\0')
		    /* Found a match.  */
		    return (char *) haystack;
		  if (*rhaystack == '\0')
		    /* No match.  */
		    return 0;
		  if (*rhaystack != *rneedle)
		    /* Nothing in this round.  */
		    break;
		}
	    }
	}
    }
  else
    return (char *) haystack;
}

int EXPORT_FUNC(holy_isspace) (int c);

static inline int
holy_isprint (int c)
{
  return (c >= ' ' && c <= '~');
}

static inline int
holy_iscntrl (int c)
{
  return (c >= 0x00 && c <= 0x1F) || c == 0x7F;
}

static inline int
holy_isalpha (int c)
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline int
holy_islower (int c)
{
  return (c >= 'a' && c <= 'z');
}

static inline int
holy_isupper (int c)
{
  return (c >= 'A' && c <= 'Z');
}

static inline int
holy_isgraph (int c)
{
  return (c >= '!' && c <= '~');
}

static inline int
holy_isdigit (int c)
{
  return (c >= '0' && c <= '9');
}

static inline int
holy_isxdigit (int c)
{
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static inline int
holy_isalnum (int c)
{
  return holy_isalpha (c) || holy_isdigit (c);
}

static inline int
holy_tolower (int c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 'a';

  return c;
}

static inline int
holy_toupper (int c)
{
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 'A';

  return c;
}

static inline int
holy_strcasecmp (const char *s1, const char *s2)
{
  while (*s1 && *s2)
    {
      if (holy_tolower ((holy_uint8_t) *s1)
	  != holy_tolower ((holy_uint8_t) *s2))
	break;

      s1++;
      s2++;
    }

  return (int) holy_tolower ((holy_uint8_t) *s1)
    - (int) holy_tolower ((holy_uint8_t) *s2);
}

static inline int
holy_strncasecmp (const char *s1, const char *s2, holy_size_t n)
{
  if (n == 0)
    return 0;

  while (*s1 && *s2 && --n)
    {
      if (holy_tolower (*s1) != holy_tolower (*s2))
	break;

      s1++;
      s2++;
    }

  return (int) holy_tolower ((holy_uint8_t) *s1)
    - (int) holy_tolower ((holy_uint8_t) *s2);
}

unsigned long EXPORT_FUNC(holy_strtoul) (const char *str, char **end, int base);
unsigned long long EXPORT_FUNC(holy_strtoull) (const char *str, char **end, int base);

static inline long
holy_strtol (const char *str, char **end, int base)
{
  int negative = 0;
  unsigned long long magnitude;

  while (*str && holy_isspace (*str))
    str++;

  if (*str == '-')
    {
      negative = 1;
      str++;
    }

  magnitude = holy_strtoull (str, end, base);
  if (negative)
    {
      if (magnitude > (unsigned long) holy_LONG_MAX + 1)
        {
          holy_error (holy_ERR_OUT_OF_RANGE, N_("overflow is detected"));
          return holy_LONG_MIN;
        }
      return -((long) magnitude);
    }
  else
    {
      if (magnitude > holy_LONG_MAX)
        {
          holy_error (holy_ERR_OUT_OF_RANGE, N_("overflow is detected"));
          return holy_LONG_MAX;
        }
      return (long) magnitude;
    }
}

char *EXPORT_FUNC(holy_strdup) (const char *s) WARN_UNUSED_RESULT;
char *EXPORT_FUNC(holy_strndup) (const char *s, holy_size_t n) WARN_UNUSED_RESULT;
void *EXPORT_FUNC(holy_memset) (void *s, int c, holy_size_t n);
holy_size_t EXPORT_FUNC(holy_strlen) (const char *s) WARN_UNUSED_RESULT;
int EXPORT_FUNC(holy_printf) (const char *fmt, ...) __attribute__ ((format (GNU_PRINTF, 1, 2)));
int EXPORT_FUNC(holy_printf_) (const char *fmt, ...) __attribute__ ((format (GNU_PRINTF, 1, 2)));

/* Replace all `ch' characters of `input' with `with' and copy the
   result into `output'; return EOS address of `output'. */
static inline char *
holy_strchrsub (char *output, const char *input, char ch, const char *with)
{
  while (*input)
    {
      if (*input == ch)
	{
	  holy_strcpy (output, with);
	  output += holy_strlen (with);
	  input++;
	  continue;
	}
      *output++ = *input++;
    }
  *output = '\0';
  return output;
}

extern void (*EXPORT_VAR (holy_xputs)) (const char *str);

static inline int
holy_puts (const char *s)
{
  const char nl[2] = "\n";
  holy_xputs (s);
  holy_xputs (nl);

  return 1;	/* Cannot fail.  */
}

int EXPORT_FUNC(holy_puts_) (const char *s);
void EXPORT_FUNC(holy_real_dprintf) (const char *file,
                                     const int line,
                                     const char *condition,
                                     const char *fmt, ...) __attribute__ ((format (GNU_PRINTF, 4, 5)));
int EXPORT_FUNC(holy_vprintf) (const char *fmt, va_list args);
int EXPORT_FUNC(holy_snprintf) (char *str, holy_size_t n, const char *fmt, ...)
     __attribute__ ((format (GNU_PRINTF, 3, 4)));
int EXPORT_FUNC(holy_vsnprintf) (char *str, holy_size_t n, const char *fmt,
				 va_list args);
char *EXPORT_FUNC(holy_xasprintf) (const char *fmt, ...)
     __attribute__ ((format (GNU_PRINTF, 1, 2))) WARN_UNUSED_RESULT;
char *EXPORT_FUNC(holy_xvasprintf) (const char *fmt, va_list args) WARN_UNUSED_RESULT;
void EXPORT_FUNC(holy_exit) (void) __attribute__ ((noreturn));
holy_uint64_t EXPORT_FUNC(holy_divmod64) (holy_uint64_t n,
					  holy_uint64_t d,
					  holy_uint64_t *r);

/* Must match softdiv group in gentpl.py.  */
#if !defined(holy_MACHINE_EMU) && (defined(__arm__) || defined(__ia64__))
#define holy_DIVISION_IN_SOFTWARE 1
#else
#define holy_DIVISION_IN_SOFTWARE 0
#endif

/* Some division functions need to be in kernel if compiler generates calls
   to them. Otherwise we still need them for consistent tests but they go
   into a separate module.  */
#if holy_DIVISION_IN_SOFTWARE
#define EXPORT_FUNC_IF_SOFTDIV EXPORT_FUNC
#else
#define EXPORT_FUNC_IF_SOFTDIV(x) x
#endif

holy_int64_t
EXPORT_FUNC_IF_SOFTDIV(holy_divmod64s) (holy_int64_t n,
					holy_int64_t d,
					holy_int64_t *r);

holy_uint32_t
EXPORT_FUNC_IF_SOFTDIV (holy_divmod32) (holy_uint32_t n,
					holy_uint32_t d,
					holy_uint32_t *r);

holy_int32_t
EXPORT_FUNC_IF_SOFTDIV (holy_divmod32s) (holy_int32_t n,
					 holy_int32_t d,
					 holy_int32_t *r);

/* Inline functions.  */

static inline char *
holy_memchr (const void *p, int c, holy_size_t len)
{
  const char *s = (const char *) p;
  const char *e = s + len;

  for (; s < e; s++)
    if (*s == c)
      return (char *) s;

  return 0;
}


static inline unsigned int
holy_abs (int x)
{
  if (x < 0)
    return (unsigned int) (-x);
  else
    return (unsigned int) x;
}

/* Reboot the machine.  */
#if defined (holy_MACHINE_EMU) || defined (holy_MACHINE_QEMU_MIPS)
void EXPORT_FUNC(holy_reboot) (void) __attribute__ ((noreturn));
#else
void holy_reboot (void) __attribute__ ((noreturn));
#endif

#if defined (__clang__) && !defined (holy_UTIL)
void __attribute__ ((noreturn)) EXPORT_FUNC (abort) (void);
#endif

#ifdef holy_MACHINE_PCBIOS
/* Halt the system, using APM if possible. If NO_APM is true, don't
 * use APM even if it is available.  */
void holy_halt (int no_apm) __attribute__ ((noreturn));
#elif defined (__mips__) && !defined (holy_MACHINE_EMU)
void EXPORT_FUNC (holy_halt) (void) __attribute__ ((noreturn));
#else
void holy_halt (void) __attribute__ ((noreturn));
#endif

#ifdef holy_MACHINE_EMU
/* Flag to check if module loading is available.  */
extern const int EXPORT_VAR(holy_no_modules);
#else
#define holy_no_modules 0
#endif

static inline void
holy_error_save (struct holy_error_saved *save)
{
  holy_memcpy (save->errmsg, holy_errmsg, sizeof (save->errmsg));
  save->holy_errno = holy_errno;
  holy_errno = holy_ERR_NONE;
}

static inline void
holy_error_load (const struct holy_error_saved *save)
{
  holy_memcpy (holy_errmsg, save->errmsg, sizeof (holy_errmsg));
  holy_errno = save->holy_errno;
}

#if BOOT_TIME_STATS
struct holy_boot_time
{
  struct holy_boot_time *next;
  holy_uint64_t tp;
  const char *file;
  int line;
  char *msg;
};

extern struct holy_boot_time *EXPORT_VAR(holy_boot_time_head);

void EXPORT_FUNC(holy_real_boot_time) (const char *file,
				       const int line,
				       const char *fmt, ...) __attribute__ ((format (GNU_PRINTF, 3, 4)));
#define holy_boot_time(...) holy_real_boot_time(holy_FILE, __LINE__, __VA_ARGS__)
#else
#define holy_boot_time(...)
#endif

#define holy_max(a, b) (((a) > (b)) ? (a) : (b))
#define holy_min(a, b) (((a) < (b)) ? (a) : (b))

#endif /* ! holy_MISC_HEADER */
