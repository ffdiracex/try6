AC_DEFUN([gl_CONFIGMAKE_PREP],
[
   if test "x$datarootdir" = x; then
    AC_SUBST([datarootdir], ['${datadir}'])
  fi
  dnl Copy the approach used in autoconf 2.60.
  if test "x$docdir" = x; then
    AC_SUBST([docdir], [m4_ifset([AC_PACKAGE_TARNAME],
      ['${datarootdir}/doc/${PACKAGE_TARNAME}'],
      ['${datarootdir}/doc/${PACKAGE}'])])
  fi
  dnl The remaining variables missing from autoconf 2.59 are easier.
  if test "x$htmldir" = x; then
    AC_SUBST([htmldir], ['${docdir}'])
  fi
  if test "x$dvidir" = x; then
    AC_SUBST([dvidir], ['${docdir}'])
  fi
  if test "x$pdfdir" = x; then
    AC_SUBST([pdfdir], ['${docdir}'])
  fi
  if test "x$psdir" = x; then
    AC_SUBST([psdir], ['${docdir}'])
  fi
  if test "x$lispdir" = x; then
    AC_SUBST([lispdir], ['${datarootdir}/emacs/site-lisp'])
  fi
  if test "x$localedir" = x; then
    AC_SUBST([localedir], ['${datarootdir}/locale'])
  fi

  AC_SUBST([pkglibexecdir], ['${libexecdir}/${PACKAGE}'])
])
