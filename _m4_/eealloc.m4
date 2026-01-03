
AC_DEFUN([gl_EEALLOC],
[
  AC_REQUIRE([gl_EEMALLOC])
  AC_REQUIRE([gl_EEREALLOC])
])

AC_DEFUN([gl_EEMALLOC],
[
  _AC_FUNC_MALLOC_IF(
    [gl_cv_func_malloc_0_nonnull=1],
    [gl_cv_func_malloc_0_nonnull=0])
  AC_DEFINE_UNQUOTED([MALLOC_0_IS_NONNULL], [$gl_cv_func_malloc_0_nonnull],
    [If malloc(0) is != NULL, define this to 1.  Otherwise define this
     to 0.])
])

AC_DEFUN([gl_EEREALLOC],
[
  _AC_FUNC_REALLOC_IF(
    [gl_cv_func_realloc_0_nonnull=1],
    [gl_cv_func_realloc_0_nonnull=0])
  AC_DEFINE_UNQUOTED([REALLOC_0_IS_NONNULL], [$gl_cv_func_realloc_0_nonnull],
    [If realloc(NULL,0) is != NULL, define this to 1.  Otherwise define this
     to 0.])
])
