/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef _UNITYPES_H
#define _UNITYPES_H

/* Get uint8_t, uint16_t, uint32_t.  */
#include <stdint.h>

/* Type representing a Unicode character.  */
typedef uint32_t ucs4_t;

/* Attribute of a function whose result depends only on the arguments
   (not pointers!) and which has no side effects.  */
#ifndef _UC_ATTRIBUTE_CONST
# if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#  define _UC_ATTRIBUTE_CONST __attribute__ ((__const__))
# else
#  define _UC_ATTRIBUTE_CONST
# endif
#endif

/* Attribute of a function whose result depends only on the arguments
   (possibly pointers) and global memory, and which has no side effects.  */
#ifndef _UC_ATTRIBUTE_PURE
# if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#  define _UC_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define _UC_ATTRIBUTE_PURE
# endif
#endif

#endif /* _UNITYPES_H */
