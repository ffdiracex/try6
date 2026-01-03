AC_DEFUN([gl_LOCALCHARSET],
[
  dnl Prerequisites of lib/localcharset.c.
  AC_REQUIRE([AM_LANGINFO_CODESET])
  AC_REQUIRE([gl_FCNTL_O_FLAGS])
  AC_CHECK_DECLS_ONCE([getc_unlocked])

  dnl Prerequisites of the lib/Makefile.am snippet.
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_REQUIRE([gl_GLIBC21])
])
