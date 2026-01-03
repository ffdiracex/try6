# this:
#
#     ./configure CC="gcc -arch i386 -arch x86_64 -arch ppc -arch ppc64" \
#                 CXX="g++ -arch i386 -arch x86_64 -arch ppc -arch ppc64" \
#                 CPP="gcc -E" CXXCPP="g++ -E"
#
# Detect this situation and set APPLE_UNIVERSAL_BUILD accordingly.

AC_DEFUN_ONCE([gl_MULTIARCH],
[
  dnl Code similar to autoconf-2.63 AC_C_BIGENDIAN.
  gl_cv_c_multiarch=no
  AC_COMPILE_IFELSE(
    [AC_LANG_SOURCE(
      [[#ifndef __APPLE_CC__
         not a universal capable compiler
        #endif
        typedef int dummy;
      ]])],
    [
     dnl Check for potential -arch flags.  It is not universal unless
     dnl there are at least two -arch flags with different values.
     arch=
     prev=
     for word in ${CC} ${CFLAGS} ${CPPFLAGS} ${LDFLAGS}; do
       if test -n "$prev"; then
         case $word in
           i?86 | x86_64 | ppc | ppc64)
             if test -z "$arch" || test "$arch" = "$word"; then
               arch="$word"
             else
               gl_cv_c_multiarch=yes
             fi
             ;;
         esac
         prev=
       else
         if test "x$word" = "x-arch"; then
           prev=arch
         fi
       fi
     done
    ])
  if test $gl_cv_c_multiarch = yes; then
    APPLE_UNIVERSAL_BUILD=1
  else
    APPLE_UNIVERSAL_BUILD=0
  fi
  AC_SUBST([APPLE_UNIVERSAL_BUILD])
])
