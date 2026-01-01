AC_DEFUN([AC_TYPE_MBSTATE_T],
[
   AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS]) dnl for HP-UX 11.11

   AC_CACHE_CHECK([for mbstate_t], [ac_cv_type_mbstate_t],
     [AC_COMPILE_IFELSE(
        [AC_LANG_PROGRAM(
           [AC_INCLUDES_DEFAULT[
/* Tru64 with Desktop Toolkit C has a bug: <stdio.h> must be included before
   <wchar.h>.
   BSD/OS 4.0.1 has a bug: <stddef.h>, <stdio.h> and <time.h> must be
   included before <wchar.h>.  */
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>]],
           [[mbstate_t x; return sizeof x;]])],
        [ac_cv_type_mbstate_t=yes],
        [ac_cv_type_mbstate_t=no])])
   if test $ac_cv_type_mbstate_t = yes; then
     AC_DEFINE([HAVE_MBSTATE_T], [1],
               [Define to 1 if <wchar.h> declares mbstate_t.])
   else
     AC_DEFINE([mbstate_t], [int],
               [Define to a type if <wchar.h> does not define.])
   fi
])
