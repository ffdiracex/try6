AC_DEFUN([gl_FUNC_RAWMEMCHR],
[
  dnl Persuade glibc <string.h> to declare rawmemchr().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_REQUIRE([gl_HEADER_STRING_H_DEFAULTS])
  AC_CHECK_FUNCS([rawmemchr])
  if test $ac_cv_func_rawmemchr = no; then
    HAVE_RAWMEMCHR=0
  fi
])

# Prerequisites of lib/strchrnul.c.
AC_DEFUN([gl_PREREQ_RAWMEMCHR], [:])
