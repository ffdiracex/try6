/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if defined _LIBC || defined HAVE_FEATURES_H
# include <features.h>
#endif

#ifndef __USE_EXTERN_INLINES
# define __USE_EXTERN_INLINES   1
#endif
#ifdef _LIBC
# define ARGP_EI
#else
# define ARGP_EI _GL_EXTERN_INLINE
#endif
#undef __OPTIMIZE__
#define __OPTIMIZE__ 1
#include "argp.h"

/* Add weak aliases.  */
#if _LIBC - 0 && defined (weak_alias)

weak_alias (__argp_usage, argp_usage)
weak_alias (__option_is_short, _option_is_short)
weak_alias (__option_is_end, _option_is_end)

#endif
