m4_version_prereq([2.63.263], [],
[m4_define([AC][_DEFUN_ONCE],
  [AC][_DEFUN([$1],
    [AC_REQUIRE([_gl_DEFUN_ONCE([$1])],
      [m4_indir([_gl_DEFUN_ONCE([$1])])])])]dnl
[AC][_DEFUN([_gl_DEFUN_ONCE([$1])], [$2])])])

# gl_00GNULIB
# -----------
# Witness macro that this file has been included.  Needed to force
# Automake to include this file prior to all other gnulib .m4 files.
AC_DEFUN([gl_00GNULIB])
