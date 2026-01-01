/* This file was automatically imported with 
   import_gcry.py. Please don't modify it */
/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpi-internal.h"

/****************
 * Find the greatest common divisor G of A and B.
 * Return: true if this 1, false in all other cases
 */
int
gcry_mpi_gcd( gcry_mpi_t g, gcry_mpi_t xa, gcry_mpi_t xb )
{
    gcry_mpi_t a, b;

    a = mpi_copy(xa);
    b = mpi_copy(xb);

    /* TAOCP Vol II, 4.5.2, Algorithm A */
    a->sign = 0;
    b->sign = 0;
    while( gcry_mpi_cmp_ui( b, 0 ) ) {
	_gcry_mpi_fdiv_r( g, a, b ); /* g used as temorary variable */
	mpi_set(a,b);
	mpi_set(b,g);
    }
    mpi_set(g, a);

    mpi_free(a);
    mpi_free(b);
    return !gcry_mpi_cmp_ui( g, 1);
}
