
AC_DEFUN([gl_INCLUDE_NEXT],
[
  AC_LANG_PREPROC_REQUIRE()
  AC_CACHE_CHECK([whether the preprocessor supports include_next],
    [gl_cv_have_include_next],
    [rm -rf conftestd1a conftestd1b conftestd2
     mkdir conftestd1a conftestd1b conftestd2
     cat <<EOF > conftestd1a/conftest.h
#define DEFINED_IN_CONFTESTD1
#include_next <conftest.h>
#ifdef DEFINED_IN_CONFTESTD2
int foo;
#else
#error "include_next doesn't work"
#endif
EOF
     cat <<EOF > conftestd1b/conftest.h
#define DEFINED_IN_CONFTESTD1
#include <stdio.h>
#include_next <conftest.h>
#ifdef DEFINED_IN_CONFTESTD2
int foo;
#else
#error "include_next doesn't work"
#endif
EOF
     cat <<EOF > conftestd2/conftest.h
#ifndef DEFINED_IN_CONFTESTD1
#error "include_next test doesn't work"
#endif
#define DEFINED_IN_CONFTESTD2
EOF
     gl_save_CPPFLAGS="$CPPFLAGS"
     CPPFLAGS="$gl_save_CPPFLAGS -Iconftestd1b -Iconftestd2"
dnl We intentionally avoid using AC_LANG_SOURCE here.
     AC_COMPILE_IFELSE([AC_LANG_DEFINES_PROVIDED[#include <conftest.h>]],
       [gl_cv_have_include_next=yes],
       [CPPFLAGS="$gl_save_CPPFLAGS -Iconftestd1a -Iconftestd2"
        AC_COMPILE_IFELSE([AC_LANG_DEFINES_PROVIDED[#include <conftest.h>]],
          [gl_cv_have_include_next=buggy],
          [gl_cv_have_include_next=no])
       ])
     CPPFLAGS="$gl_save_CPPFLAGS"
     rm -rf conftestd1a conftestd1b conftestd2
    ])
  PRAGMA_SYSTEM_HEADER=
  if test $gl_cv_have_include_next = yes; then
    INCLUDE_NEXT=include_next
    INCLUDE_NEXT_AS_FIRST_DIRECTIVE=include_next
    if test -n "$GCC"; then
      PRAGMA_SYSTEM_HEADER='#pragma GCC system_header'
    fi
  else
    if test $gl_cv_have_include_next = buggy; then
      INCLUDE_NEXT=include
      INCLUDE_NEXT_AS_FIRST_DIRECTIVE=include_next
    else
      INCLUDE_NEXT=include
      INCLUDE_NEXT_AS_FIRST_DIRECTIVE=include
    fi
  fi
  AC_SUBST([INCLUDE_NEXT])
  AC_SUBST([INCLUDE_NEXT_AS_FIRST_DIRECTIVE])
  AC_SUBST([PRAGMA_SYSTEM_HEADER])
  AC_CACHE_CHECK([whether system header files limit the line length],
    [gl_cv_pragma_columns],
    [dnl HP NonStop systems, which define __TANDEM, have this misfeature.
     AC_EGREP_CPP([choke me],
       [
#ifdef __TANDEM
choke me
#endif
       ],
       [gl_cv_pragma_columns=yes],
       [gl_cv_pragma_columns=no])
    ])
  if test $gl_cv_pragma_columns = yes; then
    PRAGMA_COLUMNS="#pragma COLUMNS 10000"
  else
    PRAGMA_COLUMNS=
  fi
  AC_SUBST([PRAGMA_COLUMNS])
])


AC_DEFUN([gl_CHECK_NEXT_HEADERS],
[
  gl_NEXT_HEADERS_INTERNAL([$1], [check])
])

AC_DEFUN([gl_NEXT_HEADERS],
[
  gl_NEXT_HEADERS_INTERNAL([$1], [assume])
])

# The guts of gl_CHECK_NEXT_HEADERS and gl_NEXT_HEADERS.
AC_DEFUN([gl_NEXT_HEADERS_INTERNAL],
[
  AC_REQUIRE([gl_INCLUDE_NEXT])
  AC_REQUIRE([AC_CANONICAL_HOST])

  m4_if([$2], [check],
    [AC_CHECK_HEADERS_ONCE([$1])
    ])

dnl FIXME: gl_next_header and gl_header_exists must be used unquoted
dnl until we can assume autoconf 2.64 or newer.
  m4_foreach_w([gl_HEADER_NAME], [$1],
    [AS_VAR_PUSHDEF([gl_next_header],
                    [gl_cv_next_]m4_defn([gl_HEADER_NAME]))
     if test $gl_cv_have_include_next = yes; then
       AS_VAR_SET(gl_next_header, ['<'gl_HEADER_NAME'>'])
     else
       AC_CACHE_CHECK(
         [absolute name of <]m4_defn([gl_HEADER_NAME])[>],
         m4_defn([gl_next_header]),
         [m4_if([$2], [check],
            [AS_VAR_PUSHDEF([gl_header_exists],
                            [ac_cv_header_]m4_defn([gl_HEADER_NAME]))
             if test AS_VAR_GET(gl_header_exists) = yes; then
             AS_VAR_POPDEF([gl_header_exists])
            ])
               AC_LANG_CONFTEST(
                 [AC_LANG_SOURCE(
                    [[#include <]]m4_dquote(m4_defn([gl_HEADER_NAME]))[[>]]
                  )])
               dnl AIX "xlc -E" and "cc -E" omit #line directives for header
               dnl files that contain only a #include of other header files and
               dnl no non-comment tokens of their own. This leads to a failure
               dnl to detect the absolute name of <dirent.h>, <signal.h>,
               dnl <poll.h> and others. The workaround is to force preservation
               dnl of comments through option -C. This ensures all necessary
               dnl #line directives are present. GCC supports option -C as well.
               case "$host_os" in
                 aix*) gl_absname_cpp="$ac_cpp -C" ;;
                 *)    gl_absname_cpp="$ac_cpp" ;;
               esac
changequote(,)
               case "$host_os" in
                 mingw*)
                   dnl For the sake of native Windows compilers (excluding gcc),
                   dnl treat backslash as a directory separator, like /.
                   dnl Actually, these compilers use a double-backslash as
                   dnl directory separator, inside the
                   dnl   # line "filename"
                   dnl directives.
                   gl_dirsep_regex='[/\\]'
                   ;;
                 *)
                   gl_dirsep_regex='\/'
                   ;;
               esac
               dnl A sed expression that turns a string into a basic regular
               dnl expression, for use within "/.../".
               gl_make_literal_regex_sed='s,[]$^\\.*/[],\\&,g'
changequote([,])
               gl_header_literal_regex=`echo ']m4_defn([gl_HEADER_NAME])[' \
                                        | sed -e "$gl_make_literal_regex_sed"`
               gl_absolute_header_sed="/${gl_dirsep_regex}${gl_header_literal_regex}/"'{
                   s/.*"\(.*'"${gl_dirsep_regex}${gl_header_literal_regex}"'\)".*/\1/
changequote(,)dnl
                   s|^/[^/]|//&|
changequote([,])dnl
                   p
                   q
                 }'
               dnl eval is necessary to expand gl_absname_cpp.
               dnl Ultrix and Pyramid sh refuse to redirect output of eval,
               dnl so use subshell.
               AS_VAR_SET(gl_next_header,
                 ['"'`(eval "$gl_absname_cpp conftest.$ac_ext") 2>&AS_MESSAGE_LOG_FD |
                      sed -n "$gl_absolute_header_sed"`'"'])
          m4_if([$2], [check],
            [else
               AS_VAR_SET(gl_next_header, ['<'gl_HEADER_NAME'>'])
             fi
            ])
         ])
     fi
     AC_SUBST(
       AS_TR_CPP([NEXT_]m4_defn([gl_HEADER_NAME])),
       [AS_VAR_GET(gl_next_header)])
     if test $gl_cv_have_include_next = yes || test $gl_cv_have_include_next = buggy; then
       # INCLUDE_NEXT_AS_FIRST_DIRECTIVE='include_next'
       gl_next_as_first_directive='<'gl_HEADER_NAME'>'
     else
       # INCLUDE_NEXT_AS_FIRST_DIRECTIVE='include'
       gl_next_as_first_directive=AS_VAR_GET(gl_next_header)
     fi
     AC_SUBST(
       AS_TR_CPP([NEXT_AS_FIRST_DIRECTIVE_]m4_defn([gl_HEADER_NAME])),
       [$gl_next_as_first_directive])
     AS_VAR_POPDEF([gl_next_header])])
])

# Autoconf 2.68 added warnings for our use of AC_COMPILE_IFELSE;
# this fallback is safe for all earlier autoconf versions.
m4_define_default([AC_LANG_DEFINES_PROVIDED])
