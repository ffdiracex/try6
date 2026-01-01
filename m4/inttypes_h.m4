
AC_DEFUN([gl_AC_HEADER_INTTYPES_H],
[
  AC_CACHE_CHECK([for inttypes.h], [gl_cv_header_inttypes_h],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
          [[
#include <sys/types.h>
#include <inttypes.h>
          ]],
          [[uintmax_t i = (uintmax_t) -1; return !i;]])],
       [gl_cv_header_inttypes_h=yes],
       [gl_cv_header_inttypes_h=no])])
  if test $gl_cv_header_inttypes_h = yes; then
    AC_DEFINE_UNQUOTED([HAVE_INTTYPES_H_WITH_UINTMAX], [1],
      [Define if <inttypes.h> exists, doesn't clash with <sys/types.h>,
       and declares uintmax_t. ])
  fi
])
