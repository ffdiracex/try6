/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	holy_I18N_H
#define	holy_I18N_H	1

#include <config.h>
#include <holy/symbol.h>

/* NLS can be disabled through the configure --disable-nls option.  */
#if (defined(ENABLE_NLS) && ENABLE_NLS) || !defined (holy_UTIL)

extern const char *(*EXPORT_VAR(holy_gettext)) (const char *s) __attribute__ ((format_arg (1)));

# ifdef holy_UTIL

#  include <locale.h>
#  include <libintl.h>

# endif /* holy_UTIL */

#else /* ! (defined(ENABLE_NLS) && ENABLE_NLS) */

/* Disabled NLS.
   The casts to 'const char *' serve the purpose of producing warnings
   for invalid uses of the value returned from these functions.
   On pre-ANSI systems without 'const', the config.h file is supposed to
   contain "#define const".  */
static inline const char * __attribute__ ((always_inline,format_arg (1)))
gettext (const char *str)
{
  return str;
}

#endif /* (defined(ENABLE_NLS) && ENABLE_NLS) */

#ifdef holy_UTIL
static inline const char * __attribute__ ((always_inline,format_arg (1)))
_ (const char *str)
{
  return gettext(str);
}
#else
static inline const char * __attribute__ ((always_inline,format_arg (1)))
_ (const char *str)
{
  return holy_gettext(str);
}
#endif /* holy_UTIL */

#define N_(str) str

#endif /* holy_I18N_H */
