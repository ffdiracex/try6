AC_DEFUN([gl_AC_HEADER_STDINT_H],
[
  AC_CACHE_CHECK([for stdint.h], [gl_cv_header_stdint_h],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
          [[#include <sys/types.h>
            #include <stdint.h>]],
          [[uintmax_t i = (uintmax_t) -1; return !i;]])],
       [gl_cv_header_stdint_h=yes],
       [gl_cv_header_stdint_h=no])])
  if test $gl_cv_header_stdint_h = yes; then
    AC_DEFINE_UNQUOTED([HAVE_STDINT_H_WITH_UINTMAX], [1],
      [Define if <stdint.h> exists, doesn't clash with <sys/types.h>,
       and declares uintmax_t. ])
  fi
])
