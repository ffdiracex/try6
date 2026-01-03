/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef _MSVC_NOTHROW_H
#define _MSVC_NOTHROW_H

/* With MSVC runtime libraries with the "invalid parameter handler" concept,
   functions like fprintf(), dup2(), or close() crash when the caller passes
   an invalid argument.  But POSIX wants error codes (such as EINVAL or EBADF)
   instead.
   This file defines wrappers that turn such an invalid parameter notification
   into an error code.  */

#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__

/* Get original declaration of _get_osfhandle.  */
# include <io.h>

# if HAVE_MSVC_INVALID_PARAMETER_HANDLER

/* Override _get_osfhandle.  */
extern intptr_t _gl_nothrow_get_osfhandle (int fd);
#  define _get_osfhandle _gl_nothrow_get_osfhandle

# endif

#endif

#endif /* _MSVC_NOTHROW_H */
