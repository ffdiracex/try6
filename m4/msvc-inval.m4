
AC_DEFUN([gl_MSVC_INVAL],
[
  AC_CHECK_FUNCS_ONCE([_set_invalid_parameter_handler])
  if test $ac_cv_func__set_invalid_parameter_handler = yes; then
    HAVE_MSVC_INVALID_PARAMETER_HANDLER=1
    AC_DEFINE([HAVE_MSVC_INVALID_PARAMETER_HANDLER], [1],
      [Define to 1 on MSVC platforms that have the "invalid parameter handler"
       concept.])
  else
    HAVE_MSVC_INVALID_PARAMETER_HANDLER=0
  fi
  AC_SUBST([HAVE_MSVC_INVALID_PARAMETER_HANDLER])
])
