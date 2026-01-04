/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef _VASNPRINTF_H
#define _VASNPRINTF_H

/* Get va_list.  */
#include <stdarg.h>

/* Get size_t.  */
#include <stddef.h>

/* The __attribute__ feature is available in gcc versions 2.5 and later.
   The __-protected variants of the attributes 'format' and 'printf' are
   accepted by gcc versions 2.6.4 (effectively 2.7) and later.
   We enable _GL_ATTRIBUTE_FORMAT only if these are supported too, because
   gnulib and libintl do '#define printf __printf__' when they override
   the 'printf' function.  */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
# define _GL_ATTRIBUTE_FORMAT(spec) __attribute__ ((__format__ spec))
#else
# define _GL_ATTRIBUTE_FORMAT(spec) /* empty */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Write formatted output to a string dynamically allocated with malloc().
   You can pass a preallocated buffer for the result in RESULTBUF and its
   size in *LENGTHP; otherwise you pass RESULTBUF = NULL.
   If successful, return the address of the string (this may be = RESULTBUF
   if no dynamic memory allocation was necessary) and set *LENGTHP to the
   number of resulting bytes, excluding the trailing NUL.  Upon error, set
   errno and return NULL.

   When dynamic memory allocation occurs, the preallocated buffer is left
   alone (with possibly modified contents).  This makes it possible to use
   a statically allocated or stack-allocated buffer, like this:

          char buf[100];
          size_t len = sizeof (buf);
          char *output = vasnprintf (buf, &len, format, args);
          if (output == NULL)
            ... error handling ...;
          else
            {
              ... use the output string ...;
              if (output != buf)
                free (output);
            }
  */
#if REPLACE_VASNPRINTF
# define asnprintf rpl_asnprintf
# define vasnprintf rpl_vasnprintf
#endif
extern char * asnprintf (char *resultbuf, size_t *lengthp, const char *format, ...)
       _GL_ATTRIBUTE_FORMAT ((__printf__, 3, 4));
extern char * vasnprintf (char *resultbuf, size_t *lengthp, const char *format, va_list args)
       _GL_ATTRIBUTE_FORMAT ((__printf__, 3, 0));

#ifdef __cplusplus
}
#endif

#endif /* _VASNPRINTF_H */
