
AC_DEFUN([gl_DIRNAME],
[
  AC_REQUIRE([gl_DIRNAME_LGPL])
])

AC_DEFUN([gl_DIRNAME_LGPL],
[
  dnl Prerequisites of lib/dirname.h.
  AC_REQUIRE([gl_DOUBLE_SLASH_ROOT])

  dnl No prerequisites of lib/basename-lgpl.c, lib/dirname-lgpl.c,
  dnl lib/stripslash.c.
])
