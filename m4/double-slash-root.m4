# double-slash-root.m4 serial 4   -*- Autoconf -*-

AC_DEFUN([gl_DOUBLE_SLASH_ROOT],
[
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_CACHE_CHECK([whether // is distinct from /], [gl_cv_double_slash_root],
    [ if test x"$cross_compiling" = xyes ; then
        # When cross-compiling, there is no way to tell whether // is special
        # short of a list of hosts.
        case $host in
          *-cygwin | i370-ibm-openedition)
            gl_cv_double_slash_root=yes ;;
          *)
            # Be optimistic and assume that / and // are the same when we
            # don't know.
            gl_cv_double_slash_root='unknown, assuming no' ;;
        esac
      else
        set x `ls -di / // 2>/dev/null`
        if test "$[2]" = "$[4]" && wc //dev/null >/dev/null 2>&1; then
          gl_cv_double_slash_root=no
        else
          gl_cv_double_slash_root=yes
        fi
      fi])
  if test "$gl_cv_double_slash_root" = yes; then
    AC_DEFINE([DOUBLE_SLASH_IS_DISTINCT_ROOT], [1],
      [Define to 1 if // is a file system root distinct from /.])
  fi
])
