/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/err.h>
#include <holy/mm.h>
#include <stdarg.h>
#include <holy/term.h>
#include <holy/env.h>
#include <holy/i18n.h>

union printf_arg
{
  /* Yes, type is also part of union as the moment we fill the value
     we don't need to store its type anymore (when we'll need it, we'll
     have format spec again. So save some space.  */
  enum
    {
      INT, LONG, LONGLONG,
      UNSIGNED_INT = 3, UNSIGNED_LONG, UNSIGNED_LONGLONG
    } type;
  long long ll;
};

struct printf_args
{
  union printf_arg prealloc[32];
  union printf_arg *ptr;
  holy_size_t count;
};

static void
parse_printf_args (const char *fmt0, struct printf_args *args,
		   va_list args_in);
static int
holy_vsnprintf_real (char *str, holy_size_t max_len, const char *fmt0,
		     struct printf_args *args);

static void
free_printf_args (struct printf_args *args)
{
  if (args->ptr != args->prealloc)
    holy_free (args->ptr);
}

static int
holy_iswordseparator (int c)
{
  return (holy_isspace (c) || c == ',' || c == ';' || c == '|' || c == '&');
}

/* holy_gettext_dummy is not translating anything.  */
static const char *
holy_gettext_dummy (const char *s)
{
  return s;
}

const char* (*holy_gettext) (const char *s) = holy_gettext_dummy;

void *
holy_memmove (void *dest, const void *src, holy_size_t n)
{
  char *d = (char *) dest;
  const char *s = (const char *) src;

  if (d < s)
    while (n--)
      *d++ = *s++;
  else
    {
      d += n;
      s += n;

      while (n--)
	*--d = *--s;
    }

  return dest;
}

char *
holy_strcpy (char *dest, const char *src)
{
  char *p = dest;

  while ((*p++ = *src++) != '\0')
    ;

  return dest;
}

int
holy_printf (const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start (ap, fmt);
  ret = holy_vprintf (fmt, ap);
  va_end (ap);

  return ret;
}

int
holy_printf_ (const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start (ap, fmt);
  ret = holy_vprintf (_(fmt), ap);
  va_end (ap);

  return ret;
}

int
holy_puts_ (const char *s)
{
  return holy_puts (_(s));
}

#if defined (__APPLE__) && ! defined (holy_UTIL)
int
holy_err_printf (const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start (ap, fmt);
	ret = holy_vprintf (fmt, ap);
	va_end (ap);

	return ret;
}
#endif

#if ! defined (__APPLE__) && ! defined (holy_UTIL)
int holy_err_printf (const char *fmt, ...)
__attribute__ ((alias("holy_printf")));
#endif

void
holy_real_dprintf (const char *file, const int line, const char *condition,
		   const char *fmt, ...)
{
  va_list args;
  const char *debug = holy_env_get ("debug");

  if (! debug)
    return;

  if (holy_strword (debug, "all") || holy_strword (debug, condition))
    {
      holy_printf ("%s:%d: ", file, line);
      va_start (args, fmt);
      holy_vprintf (fmt, args);
      va_end (args);
      holy_refresh ();
    }
}

#define PREALLOC_SIZE 255

int
holy_vprintf (const char *fmt, va_list ap)
{
  holy_size_t s;
  static char buf[PREALLOC_SIZE + 1];
  char *curbuf = buf;
  struct printf_args args;

  parse_printf_args (fmt, &args, ap);

  s = holy_vsnprintf_real (buf, PREALLOC_SIZE, fmt, &args);
  if (s > PREALLOC_SIZE)
    {
      curbuf = holy_malloc (s + 1);
      if (!curbuf)
	{
	  holy_errno = holy_ERR_NONE;
	  buf[PREALLOC_SIZE - 3] = '.';
	  buf[PREALLOC_SIZE - 2] = '.';
	  buf[PREALLOC_SIZE - 1] = '.';
	  buf[PREALLOC_SIZE] = 0;
	  curbuf = buf;
	}
      else
	s = holy_vsnprintf_real (curbuf, s, fmt, &args);
    }

  free_printf_args (&args);

  holy_xputs (curbuf);

  if (curbuf != buf)
    holy_free (curbuf);

  return s;
}

int
holy_memcmp (const void *s1, const void *s2, holy_size_t n)
{
  const holy_uint8_t *t1 = s1;
  const holy_uint8_t *t2 = s2;

  while (n--)
    {
      if (*t1 != *t2)
	return (int) *t1 - (int) *t2;

      t1++;
      t2++;
    }

  return 0;
}

int
holy_strcmp (const char *s1, const char *s2)
{
  while (*s1 && *s2)
    {
      if (*s1 != *s2)
	break;

      s1++;
      s2++;
    }

  return (int) (holy_uint8_t) *s1 - (int) (holy_uint8_t) *s2;
}

int
holy_strncmp (const char *s1, const char *s2, holy_size_t n)
{
  if (n == 0)
    return 0;

  while (*s1 && *s2 && --n)
    {
      if (*s1 != *s2)
	break;

      s1++;
      s2++;
    }

  return (int) (holy_uint8_t) *s1 - (int) (holy_uint8_t)  *s2;
}

char *
holy_strchr (const char *s, int c)
{
  do
    {
      if (*s == c)
	return (char *) s;
    }
  while (*s++);

  return 0;
}

char *
holy_strrchr (const char *s, int c)
{
  char *p = NULL;

  do
    {
      if (*s == c)
	p = (char *) s;
    }
  while (*s++);

  return p;
}

int
holy_strword (const char *haystack, const char *needle)
{
  const char *n_pos = needle;

  while (holy_iswordseparator (*haystack))
    haystack++;

  while (*haystack)
    {
      /* Crawl both the needle and the haystack word we're on.  */
      while(*haystack && !holy_iswordseparator (*haystack)
            && *haystack == *n_pos)
        {
          haystack++;
          n_pos++;
        }

      /* If we reached the end of both words at the same time, the word
      is found. If not, eat everything in the haystack that isn't the
      next word (or the end of string) and "reset" the needle.  */
      if ( (!*haystack || holy_iswordseparator (*haystack))
         && (!*n_pos || holy_iswordseparator (*n_pos)))
        return 1;
      else
        {
          n_pos = needle;
          while (*haystack && !holy_iswordseparator (*haystack))
            haystack++;
          while (holy_iswordseparator (*haystack))
            haystack++;
        }
    }

  return 0;
}

int
holy_isspace (int c)
{
  return (c == '\n' || c == '\r' || c == ' ' || c == '\t');
}

unsigned long
holy_strtoul (const char *str, char **end, int base)
{
  unsigned long long num;

  num = holy_strtoull (str, end, base);
#if holy_CPU_SIZEOF_LONG != 8
  if (num > ~0UL)
    {
      holy_error (holy_ERR_OUT_OF_RANGE, N_("overflow is detected"));
      return ~0UL;
    }
#endif

  return (unsigned long) num;
}

unsigned long long
holy_strtoull (const char *str, char **end, int base)
{
  unsigned long long num = 0;
  int found = 0;

  /* Skip white spaces.  */
  /* holy_isspace checks that *str != '\0'.  */
  while (holy_isspace (*str))
    str++;

  /* Guess the base, if not specified. The prefix `0x' means 16, and
     the prefix `0' means 8.  */
  if (str[0] == '0')
    {
      if (str[1] == 'x')
	{
	  if (base == 0 || base == 16)
	    {
	      base = 16;
	      str += 2;
	    }
	}
      else if (base == 0 && str[1] >= '0' && str[1] <= '7')
	base = 8;
    }

  if (base == 0)
    base = 10;

  while (*str)
    {
      unsigned long digit;

      digit = holy_tolower (*str) - '0';
      if (digit > 9)
	{
	  digit += '0' - 'a' + 10;
	  if (digit >= (unsigned long) base)
	    break;
	}

      found = 1;

      /* NUM * BASE + DIGIT > ~0ULL */
      if (num > holy_divmod64 (~0ULL - digit, base, 0))
	{
	  holy_error (holy_ERR_OUT_OF_RANGE,
		      N_("overflow is detected"));
	  return ~0ULL;
	}

      num = num * base + digit;
      str++;
    }

  if (! found)
    {
      holy_error (holy_ERR_BAD_NUMBER,
		  N_("unrecognized number"));
      return 0;
    }

  if (end)
    *end = (char *) str;

  return num;
}

char *
holy_strdup (const char *s)
{
  holy_size_t len;
  char *p;

  len = holy_strlen (s) + 1;
  p = (char *) holy_malloc (len);
  if (! p)
    return 0;

  return holy_memcpy (p, s, len);
}

char *
holy_strndup (const char *s, holy_size_t n)
{
  holy_size_t len;
  char *p;

  len = holy_strlen (s);
  if (len > n)
    len = n;
  p = (char *) holy_malloc (len + 1);
  if (! p)
    return 0;

  holy_memcpy (p, s, len);
  p[len] = '\0';
  return p;
}

/* clang detects that we're implementing here a memset so it decides to
   optimise and calls memset resulting in infinite recursion. With volatile
   we make it not optimise in this way.  */
#ifdef __clang__
#define VOLATILE_CLANG volatile
#else
#define VOLATILE_CLANG
#endif

void *
holy_memset (void *s, int c, holy_size_t len)
{
  void *p = s;
  holy_uint8_t pattern8 = c;

  if (len >= 3 * sizeof (unsigned long))
    {
      unsigned long patternl = 0;
      holy_size_t i;

      for (i = 0; i < sizeof (unsigned long); i++)
	patternl |= ((unsigned long) pattern8) << (8 * i);

      while (len > 0 && (((holy_addr_t) p) & (sizeof (unsigned long) - 1)))
	{
	  *(VOLATILE_CLANG holy_uint8_t *) p = pattern8;
	  p = (holy_uint8_t *) p + 1;
	  len--;
	}
      while (len >= sizeof (unsigned long))
	{
	  *(VOLATILE_CLANG unsigned long *) p = patternl;
	  p = (unsigned long *) p + 1;
	  len -= sizeof (unsigned long);
	}
    }

  while (len > 0)
    {
      *(VOLATILE_CLANG holy_uint8_t *) p = pattern8;
      p = (holy_uint8_t *) p + 1;
      len--;
    }

  return s;
}

holy_size_t
holy_strlen (const char *s)
{
  const char *p = s;

  while (*p)
    p++;

  return p - s;
}

static inline void
holy_reverse (char *str)
{
  char *p = str + holy_strlen (str) - 1;

  while (str < p)
    {
      char tmp;

      tmp = *str;
      *str = *p;
      *p = tmp;
      str++;
      p--;
    }
}

/* Divide N by D, return the quotient, and store the remainder in *R.  */
holy_uint64_t
holy_divmod64 (holy_uint64_t n, holy_uint64_t d, holy_uint64_t *r)
{
  /* This algorithm is typically implemented by hardware. The idea
     is to get the highest bit in N, 64 times, by keeping
     upper(N * 2^i) = (Q * D + M), where upper
     represents the high 64 bits in 128-bits space.  */
  unsigned bits = 64;
  holy_uint64_t q = 0;
  holy_uint64_t m = 0;

  /* ARM and IA64 don't have a fast 32-bit division.
     Using that code would just make us use software division routines, calling
     ourselves indirectly and hence getting infinite recursion.
  */
#if !holy_DIVISION_IN_SOFTWARE
  /* Skip the slow computation if 32-bit arithmetic is possible.  */
  if (n < 0xffffffff && d < 0xffffffff)
    {
      if (r)
	*r = ((holy_uint32_t) n) % (holy_uint32_t) d;

      return ((holy_uint32_t) n) / (holy_uint32_t) d;
    }
#endif

  while (bits--)
    {
      m <<= 1;

      if (n & (1ULL << 63))
	m |= 1;

      q <<= 1;
      n <<= 1;

      if (m >= d)
	{
	  q |= 1;
	  m -= d;
	}
    }

  if (r)
    *r = m;

  return q;
}

/* Convert a long long value to a string. This function avoids 64-bit
   modular arithmetic or divisions.  */
static inline char *
holy_lltoa (char *str, int c, unsigned long long n)
{
  unsigned base = (c == 'x') ? 16 : 10;
  char *p;

  if ((long long) n < 0 && c == 'd')
    {
      n = (unsigned long long) (-((long long) n));
      *str++ = '-';
    }

  p = str;

  if (base == 16)
    do
      {
	unsigned d = (unsigned) (n & 0xf);
	*p++ = (d > 9) ? d + 'a' - 10 : d + '0';
      }
    while (n >>= 4);
  else
    /* BASE == 10 */
    do
      {
	holy_uint64_t m;

	n = holy_divmod64 (n, 10, &m);
	*p++ = m + '0';
      }
    while (n);

  *p = 0;

  holy_reverse (str);
  return p;
}

static void
parse_printf_args (const char *fmt0, struct printf_args *args,
		   va_list args_in)
{
  const char *fmt;
  char c;
  holy_size_t n = 0;

  args->count = 0;

  COMPILE_TIME_ASSERT (sizeof (int) == sizeof (holy_uint32_t));
  COMPILE_TIME_ASSERT (sizeof (int) <= sizeof (long long));
  COMPILE_TIME_ASSERT (sizeof (long) <= sizeof (long long));
  COMPILE_TIME_ASSERT (sizeof (long long) == sizeof (void *)
		       || sizeof (int) == sizeof (void *));

  fmt = fmt0;
  while ((c = *fmt++) != 0)
    {
      if (c != '%')
	continue;

      if (*fmt =='-')
	fmt++;

      while (holy_isdigit (*fmt))
	fmt++;

      if (*fmt == '$')
	fmt++;

      if (*fmt =='-')
	fmt++;

      while (holy_isdigit (*fmt))
	fmt++;

      if (*fmt =='.')
	fmt++;

      while (holy_isdigit (*fmt))
	fmt++;

      c = *fmt++;
      if (c == 'l')
	c = *fmt++;
      if (c == 'l')
	c = *fmt++;

      switch (c)
	{
	case 'p':
	case 'x':
	case 'u':
	case 'd':
	case 'c':
	case 'C':
	case 's':
	  args->count++;
	  break;
	}
    }

  if (args->count <= ARRAY_SIZE (args->prealloc))
    args->ptr = args->prealloc;
  else
    {
      args->ptr = holy_malloc (args->count * sizeof (args->ptr[0]));
      if (!args->ptr)
	{
	  holy_errno = holy_ERR_NONE;
	  args->ptr = args->prealloc;
	  args->count = ARRAY_SIZE (args->prealloc);
	}
    }

  holy_memset (args->ptr, 0, args->count * sizeof (args->ptr[0]));

  fmt = fmt0;
  n = 0;
  while ((c = *fmt++) != 0)
    {
      int longfmt = 0;
      holy_size_t curn;
      const char *p;

      if (c != '%')
	continue;

      curn = n++;

      if (*fmt =='-')
	fmt++;

      p = fmt;

      while (holy_isdigit (*fmt))
	fmt++;

      if (*fmt == '$')
	{
	  curn = holy_strtoull (p, 0, 10) - 1;
	  fmt++;
	}

      if (*fmt =='-')
	fmt++;

      while (holy_isdigit (*fmt))
	fmt++;

      if (*fmt =='.')
	fmt++;

      while (holy_isdigit (*fmt))
	fmt++;

      c = *fmt++;
      if (c == '%')
	{
	  n--;
	  continue;
	}

      if (c == 'l')
	{
	  c = *fmt++;
	  longfmt = 1;
	}
      if (c == 'l')
	{
	  c = *fmt++;
	  longfmt = 2;
	}
      if (curn >= args->count)
	continue;
      switch (c)
	{
	case 'x':
	case 'u':
	  args->ptr[curn].type = UNSIGNED_INT + longfmt;
	  break;
	case 'd':
	  args->ptr[curn].type = INT + longfmt;
	  break;
	case 'p':
	case 's':
	  if (sizeof (void *) == sizeof (long long))
	    args->ptr[curn].type = UNSIGNED_LONGLONG;
	  else
	    args->ptr[curn].type = UNSIGNED_INT;
	  break;
	case 'C':
	case 'c':
	  args->ptr[curn].type = INT;
	  break;
	}
    }

  for (n = 0; n < args->count; n++)
    switch (args->ptr[n].type)
      {
      case INT:
	args->ptr[n].ll = va_arg (args_in, int);
	break;
      case LONG:
	args->ptr[n].ll = va_arg (args_in, long);
	break;
      case UNSIGNED_INT:
	args->ptr[n].ll = va_arg (args_in, unsigned int);
	break;
      case UNSIGNED_LONG:
	args->ptr[n].ll = va_arg (args_in, unsigned long);
	break;
      case LONGLONG:
      case UNSIGNED_LONGLONG:
	args->ptr[n].ll = va_arg (args_in, long long);
	break;
      }
}

static inline void __attribute__ ((always_inline))
write_char (char *str, holy_size_t *count, holy_size_t max_len, unsigned char ch)
{
  if (*count < max_len)
    str[*count] = ch;

  (*count)++;
}

static int
holy_vsnprintf_real (char *str, holy_size_t max_len, const char *fmt0,
		     struct printf_args *args)
{
  char c;
  holy_size_t n = 0;
  holy_size_t count = 0;
  const char *fmt;

  fmt = fmt0;

  while ((c = *fmt++) != 0)
    {
      unsigned int format1 = 0;
      unsigned int format2 = ~ 0U;
      char zerofill = ' ';
      char rightfill = 0;
      holy_size_t curn;

      if (c != '%')
	{
	  write_char (str, &count, max_len,c);
	  continue;
	}

      curn = n++;

    rescan:;

      if (*fmt =='-')
	{
	  rightfill = 1;
	  fmt++;
	}

      /* Read formatting parameters.  */
      if (holy_isdigit (*fmt))
	{
	  if (fmt[0] == '0')
	    zerofill = '0';
	  format1 = holy_strtoul (fmt, (char **) &fmt, 10);
	}

      if (*fmt == '.')
	fmt++;

      if (holy_isdigit (*fmt))
	format2 = holy_strtoul (fmt, (char **) &fmt, 10);

      if (*fmt == '$')
	{
	  curn = format1 - 1;
	  fmt++;
	  format1 = 0;
	  format2 = ~ 0U;
	  zerofill = ' ';
	  rightfill = 0;

	  goto rescan;
	}

      c = *fmt++;
      if (c == 'l')
	c = *fmt++;
      if (c == 'l')
	c = *fmt++;

      if (c == '%')
	{
	  write_char (str, &count, max_len,c);
	  n--;
	  continue;
	}

      if (curn >= args->count)
	continue;

      long long curarg = args->ptr[curn].ll;

      switch (c)
	{
	case 'p':
	  write_char (str, &count, max_len, '0');
	  write_char (str, &count, max_len, 'x');
	  c = 'x';
	  /* Fall through. */
	case 'x':
	case 'u':
	case 'd':
	  {
	    char tmp[32];
	    const char *p = tmp;
	    holy_size_t len;
	    holy_size_t fill;

	    len = holy_lltoa (tmp, c, curarg) - tmp;
	    fill = len < format1 ? format1 - len : 0;
	    if (! rightfill)
	      while (fill--)
		write_char (str, &count, max_len, zerofill);
	    while (*p)
	      write_char (str, &count, max_len, *p++);
	    if (rightfill)
	      while (fill--)
		write_char (str, &count, max_len, zerofill);
	  }
	  break;

	case 'c':
	  write_char (str, &count, max_len,curarg & 0xff);
	  break;

	case 'C':
	  {
	    holy_uint32_t code = curarg;
	    int shift;
	    unsigned mask;

	    if (code <= 0x7f)
	      {
		shift = 0;
		mask = 0;
	      }
	    else if (code <= 0x7ff)
	      {
		shift = 6;
		mask = 0xc0;
	      }
	    else if (code <= 0xffff)
	      {
		shift = 12;
		mask = 0xe0;
	      }
	    else if (code <= 0x10ffff)
	      {
		shift = 18;
		mask = 0xf0;
	      }
	    else
	      {
		code = '?';
		shift = 0;
		mask = 0;
	      }

	    write_char (str, &count, max_len,mask | (code >> shift));

	    for (shift -= 6; shift >= 0; shift -= 6)
	      write_char (str, &count, max_len,0x80 | (0x3f & (code >> shift)));
	  }
	  break;

	case 's':
	  {
	    holy_size_t len = 0;
	    holy_size_t fill;
	    const char *p = ((char *) (holy_addr_t) curarg) ? : "(null)";
	    holy_size_t i;

	    while (len < format2 && p[len])
	      len++;

	    fill = len < format1 ? format1 - len : 0;

	    if (!rightfill)
	      while (fill--)
		write_char (str, &count, max_len, zerofill);

	    for (i = 0; i < len; i++)
	      write_char (str, &count, max_len,*p++);

	    if (rightfill)
	      while (fill--)
		write_char (str, &count, max_len, zerofill);
	  }

	  break;

	default:
	  write_char (str, &count, max_len,c);
	  break;
	}
    }

  if (count < max_len)
    str[count] = '\0';
  else
    str[max_len] = '\0';
  return count;
}

int
holy_vsnprintf (char *str, holy_size_t n, const char *fmt, va_list ap)
{
  holy_size_t ret;
  struct printf_args args;

  if (!n)
    return 0;

  n--;

  parse_printf_args (fmt, &args, ap);

  ret = holy_vsnprintf_real (str, n, fmt, &args);

  free_printf_args (&args);

  return ret < n ? ret : n;
}

int
holy_snprintf (char *str, holy_size_t n, const char *fmt, ...)
{
  va_list ap;
  int ret;

  va_start (ap, fmt);
  ret = holy_vsnprintf (str, n, fmt, ap);
  va_end (ap);

  return ret;
}

char *
holy_xvasprintf (const char *fmt, va_list ap)
{
  holy_size_t s, as = PREALLOC_SIZE;
  char *ret;
  struct printf_args args;

  parse_printf_args (fmt, &args, ap);

  while (1)
    {
      ret = holy_malloc (as + 1);
      if (!ret)
	{
	  free_printf_args (&args);
	  return NULL;
	}

      s = holy_vsnprintf_real (ret, as, fmt, &args);

      if (s <= as)
	{
	  free_printf_args (&args);
	  return ret;
	}

      holy_free (ret);
      as = s;
    }
}

char *
holy_xasprintf (const char *fmt, ...)
{
  va_list ap;
  char *ret;

  va_start (ap, fmt);
  ret = holy_xvasprintf (fmt, ap);
  va_end (ap);

  return ret;
}

/* Abort holy. This function does not return.  */
static void __attribute__ ((noreturn))
holy_abort (void)
{
  holy_printf ("\nAborted.");
  
#ifndef holy_UTIL
  if (holy_term_inputs)
#endif
    {
      holy_printf (" Press any key to exit.");
      holy_getkey ();
    }

  holy_exit ();
}

void
holy_fatal (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  holy_vprintf (_(fmt), ap);
  va_end (ap);

  holy_refresh ();

  holy_abort ();
}

#if BOOT_TIME_STATS

#include <holy/time.h>

struct holy_boot_time *holy_boot_time_head;
static struct holy_boot_time **boot_time_last = &holy_boot_time_head;

void
holy_real_boot_time (const char *file,
		     const int line,
		     const char *fmt, ...)
{
  struct holy_boot_time *n;
  va_list args;

  holy_error_push ();
  n = holy_malloc (sizeof (*n));
  if (!n)
    {
      holy_errno = 0;
      holy_error_pop ();
      return;
    }
  n->file = file;
  n->line = line;
  n->tp = holy_get_time_ms ();
  n->next = 0;

  va_start (args, fmt);
  n->msg = holy_xvasprintf (fmt, args);
  va_end (args);

  *boot_time_last = n;
  boot_time_last = &n->next;

  holy_errno = 0;
  holy_error_pop ();
}
#endif
