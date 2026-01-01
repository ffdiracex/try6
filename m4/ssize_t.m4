AC_DEFUN([gt_TYPE_SSIZE_T],
[
  AC_CACHE_CHECK([for ssize_t], [gt_cv_ssize_t],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
          [[#include <sys/types.h>]],
          [[int x = sizeof (ssize_t *) + sizeof (ssize_t);
            return !x;]])],
       [gt_cv_ssize_t=yes], [gt_cv_ssize_t=no])])
  if test $gt_cv_ssize_t = no; then
    AC_DEFINE([ssize_t], [int],
              [Define as a signed type of the same size as size_t.])
  fi
])
