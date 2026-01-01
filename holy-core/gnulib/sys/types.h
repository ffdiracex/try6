/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#if __GNUC__ >= 3
#pragma GCC system_header
#endif


#ifndef _GL_SYS_TYPES_H

/* The include_next requires a split double-inclusion guard.  */
#include_next <sys/types.h>

#ifndef _GL_SYS_TYPES_H
#define _GL_SYS_TYPES_H

/* Override off_t if Large File Support is requested on native Windows.  */
#if 0
/* Same as int64_t in <stdint.h>.  */
# if defined _MSC_VER
#  define off_t __int64
# else
#  define off_t long long int
# endif
/* Indicator, for gnulib internal purposes.  */
# define _GL_WINDOWS_64_BIT_OFF_T 1
#endif

/* MSVC 9 defines size_t in <stddef.h>, not in <sys/types.h>.  */
/* But avoid namespace pollution on glibc systems.  */
#if ((defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__) \
    && ! defined __GLIBC__
# include <stddef.h>
#endif

#endif /* _GL_SYS_TYPES_H */
#endif /* _GL_SYS_TYPES_H */
