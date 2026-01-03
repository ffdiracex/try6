# mempcpy.m4 serial 11

AC_DEFUN([gl_FUNC_MEMPCPY],
[
  dnl Persuade glibc <string.h> to declare mempcpy().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  dnl The mempcpy() declaration in lib/string.in.h uses 'restrict'.
  AC_REQUIRE([AC_C_RESTRICT])

  AC_REQUIRE([gl_HEADER_STRING_H_DEFAULTS])
  AC_CHECK_FUNCS([mempcpy])
  if test $ac_cv_func_mempcpy = no; then
    HAVE_MEMPCPY=0
  fi
])

# Prerequisites of lib/mempcpy.c.
AC_DEFUN([gl_PREREQ_MEMPCPY], [
  :
])
