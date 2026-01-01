/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef _LOCALCHARSET_H
#define _LOCALCHARSET_H


#ifdef __cplusplus
extern "C" {
#endif


/* Determine the current locale's character encoding, and canonicalize it
   into one of the canonical names listed in config.charset.
   The result must not be freed; it is statically allocated.
   If the canonical name cannot be determined, the result is a non-canonical
   name.  */
extern const char * locale_charset (void);


#ifdef __cplusplus
}
#endif


#endif /* _LOCALCHARSET_H */
