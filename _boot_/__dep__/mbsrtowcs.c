/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

/* Specification.  */
#include <wchar.h>

#include <errno.h>
#include <limits.h>
#include <stdlib.h>

#include "strnlen1.h"


extern mbstate_t _gl_mbsrtowcs_state;

#include "mbsrtowcs-impl.h"
