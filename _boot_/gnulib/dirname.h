/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef DIRNAME_H_
# define DIRNAME_H_ 1

# include <stdbool.h>
# include <stddef.h>
# include "dosname.h"

# ifndef DIRECTORY_SEPARATOR
#  define DIRECTORY_SEPARATOR '/'
# endif

# ifndef DOUBLE_SLASH_IS_DISTINCT_ROOT
#  define DOUBLE_SLASH_IS_DISTINCT_ROOT 0
# endif

# if GNULIB_DIRNAME
char *base_name (char const *file);
char *dir_name (char const *file);
# endif

char *mdir_name (char const *file);
size_t base_len (char const *file) _GL_ATTRIBUTE_PURE;
size_t dir_len (char const *file) _GL_ATTRIBUTE_PURE;
char *last_component (char const *file) _GL_ATTRIBUTE_PURE;

bool strip_trailing_slashes (char *file);

#endif /* not DIRNAME_H_ */
