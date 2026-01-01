/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef _GL_STRERROR_OVERRIDE_H
# define _GL_STRERROR_OVERRIDE_H

# include <errno.h>
# include <stddef.h>

/* Reasonable buffer size that should never trigger ERANGE; if this
   proves too small, we intentionally abort(), to remind us to fix
   this value.  */
# define STACKBUF_LEN 256

/* If ERRNUM maps to an errno value defined by gnulib, return a string
   describing the error.  Otherwise return NULL.  */
# if REPLACE_STRERROR_0 \
     || GNULIB_defined_ESOCK \
     || GNULIB_defined_ESTREAMS \
     || GNULIB_defined_EWINSOCK \
     || GNULIB_defined_ENOMSG \
     || GNULIB_defined_EIDRM \
     || GNULIB_defined_ENOLINK \
     || GNULIB_defined_EPROTO \
     || GNULIB_defined_EMULTIHOP \
     || GNULIB_defined_EBADMSG \
     || GNULIB_defined_EOVERFLOW \
     || GNULIB_defined_ENOTSUP \
     || GNULIB_defined_ENETRESET \
     || GNULIB_defined_ECONNABORTED \
     || GNULIB_defined_ESTALE \
     || GNULIB_defined_EDQUOT \
     || GNULIB_defined_ECANCELED \
     || GNULIB_defined_EOWNERDEAD \
     || GNULIB_defined_ENOTRECOVERABLE \
     || GNULIB_defined_EILSEQ
extern const char *strerror_override (int errnum);
# else
#  define strerror_override(ignored) NULL
# endif

#endif /* _GL_STRERROR_OVERRIDE_H */
