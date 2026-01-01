/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EMU_EXEC_H
#define holy_EMU_EXEC_H 1

/*#include <config.h> */
#include <stdarg.h>
#include <holy/symbol.h>
#include <sys/types.h>
#include "config.h"
pid_t
holy_util_exec_pipe (const char *const *argv, int *fd);
pid_t
holy_util_exec_pipe_stderr (const char *const *argv, int *fd);

int
holy_util_exec_redirect_all (const char *const *argv, const char *stdin_file,
			     const char *stdout_file, const char *stderr_file);
int
holy_util_exec (const char *const *argv);
int
holy_util_exec_redirect (const char *const *argv, const char *stdin_file,
			 const char *stdout_file);
int
holy_util_exec_redirect_null (const char *const *argv);

#endif
