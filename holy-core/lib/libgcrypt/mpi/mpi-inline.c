/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>

/* put the inline functions as real functions into the lib */
#define G10_MPI_INLINE_DECL

#include "mpi-internal.h"

/* always include the header because it is only
 * included by mpi-internal if __GCC__ is defined but we
 * need it here in all cases and the above definition of
 * of the macro allows us to do so
 */
#include "mpi-inline.h"
