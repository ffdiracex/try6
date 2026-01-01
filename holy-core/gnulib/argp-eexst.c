/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <sysexits.h>

#include "argp.h"

/* The exit status that argp will use when exiting due to a parsing error.
   If not defined or set by the user program, this defaults to EX_USAGE from
   <sysexits.h>.  */
error_t argp_err_exit_status = EX_USAGE;
