/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <stddef.h>

/* Avoid a clash of our mbswidth() with a function of the same name defined
   in UnixWare 7.1.1 <wchar.h>.  We need this #include before the #define
   below.
   However, we don't want to #include <wchar.h> on all platforms because
   - Tru64 with Desktop Toolkit C has a bug: <stdio.h> must be included before
     <wchar.h>.
   - BSD/OS 4.1 has a bug: <stdio.h> and <time.h> must be included before
     <wchar.h>.  */
#if HAVE_DECL_MBSWIDTH_IN_WCHAR_H
# include <wchar.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif


/* Optional flags to influence mbswidth/mbsnwidth behavior.  */

/* If this bit is set, return -1 upon finding an invalid or incomplete
   character.  Otherwise, assume invalid characters have width 1.  */
#define MBSW_REJECT_INVALID 1

/* If this bit is set, return -1 upon finding a non-printable character.
   Otherwise, assume unprintable characters have width 0 if they are
   control characters and 1 otherwise.  */
#define MBSW_REJECT_UNPRINTABLE 2

/* If this bit is set \0 is treated as the end of string.
   Otherwise it's treated as a normal one column width character.  */
#define MBSW_STOP_AT_NUL 4

/* Returns the number of screen columns needed for STRING.  */
#define mbswidth gnu_mbswidth  /* avoid clash with UnixWare 7.1.1 function */
extern int mbswidth (const char *string, int flags);

/* Returns the number of screen columns needed for the NBYTES bytes
   starting at BUF.  */
extern int mbsnwidth (const char *buf, size_t nbytes, int flags);


#ifdef __cplusplus
}
#endif
