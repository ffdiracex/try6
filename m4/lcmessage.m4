# lcmessage.m4 serial 7 (gettext-0.18.2)

# Check whether LC_MESSAGES is available in <locale.h>.

AC_DEFUN([gt_LC_MESSAGES],
[
  AC_CACHE_CHECK([for LC_MESSAGES], [gt_cv_val_LC_MESSAGES],
    [AC_LINK_IFELSE(
       [AC_LANG_PROGRAM(
          [[#include <locale.h>]],
          [[return LC_MESSAGES]])],
       [gt_cv_val_LC_MESSAGES=yes],
       [gt_cv_val_LC_MESSAGES=no])])
  if test $gt_cv_val_LC_MESSAGES = yes; then
    AC_DEFINE([HAVE_LC_MESSAGES], [1],
      [Define if your <locale.h> file defines LC_MESSAGES.])
  fi
])
