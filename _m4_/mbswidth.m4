AC_DEFUN([gl_MBSWIDTH],
[
  AC_CHECK_HEADERS_ONCE([wchar.h])
  AC_CHECK_FUNCS_ONCE([isascii mbsinit])

  AC_CACHE_CHECK([whether mbswidth is declared in <wchar.h>],
    [ac_cv_have_decl_mbswidth],
    [AC_COMPILE_IFELSE(
       [AC_LANG_PROGRAM(
          [[
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>
          ]],
          [[
  char *p = (char *) mbswidth;
  return !p;
          ]])],
       [ac_cv_have_decl_mbswidth=yes],
       [ac_cv_have_decl_mbswidth=no])])
  if test $ac_cv_have_decl_mbswidth = yes; then
    ac_val=1
  else
    ac_val=0
  fi
  AC_DEFINE_UNQUOTED([HAVE_DECL_MBSWIDTH_IN_WCHAR_H], [$ac_val],
    [Define to 1 if you have a declaration of mbswidth() in <wchar.h>, and to 0 otherwise.])

  AC_TYPE_MBSTATE_T
])
