
AC_DEFUN([gl_ERROR],
[
  dnl We don't use AC_FUNC_ERROR_AT_LINE any more, because it is no longer
  dnl maintained in Autoconf and because it invokes AC_LIBOBJ.
  AC_CACHE_CHECK([for error_at_line], [ac_cv_lib_error_at_line],
    [AC_LINK_IFELSE(
       [AC_LANG_PROGRAM(
          [[#include <error.h>]],
          [[error_at_line (0, 0, "", 0, "an error occurred");]])],
       [ac_cv_lib_error_at_line=yes],
       [ac_cv_lib_error_at_line=no])])
])

# Prerequisites of lib/error.c.
AC_DEFUN([gl_PREREQ_ERROR],
[
  AC_REQUIRE([AC_FUNC_STRERROR_R])
  :
])
