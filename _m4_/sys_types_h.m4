AC_DEFUN_ONCE([gl_SYS_TYPES_H],
[
  AC_REQUIRE([gl_SYS_TYPES_H_DEFAULTS])
  gl_NEXT_HEADERS([sys/types.h])

  dnl Ensure the type pid_t gets defined.
  AC_REQUIRE([AC_TYPE_PID_T])

  dnl Ensure the type mode_t gets defined.
  AC_REQUIRE([AC_TYPE_MODE_T])

  dnl Whether to override the 'off_t' type.
  AC_REQUIRE([gl_TYPE_OFF_T])
])

AC_DEFUN([gl_SYS_TYPES_H_DEFAULTS],
[
])
