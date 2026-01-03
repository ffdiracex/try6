/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

/* Specification.  */
#include <float.h>

void
_Qp_itoq (long double *result, int a)
{
  /* Convert from 'int' to 'double', then from 'double' to 'long double'.  */
  *result = (double) a;
}
