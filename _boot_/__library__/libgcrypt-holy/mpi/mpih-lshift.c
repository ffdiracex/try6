/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpi-internal.h"

/* Shift U (pointed to by UP and USIZE digits long) CNT bits to the left
 * and store the USIZE least significant digits of the result at WP.
 * Return the bits shifted out from the most significant digit.
 *
 * Argument constraints:
 * 1. 0 < CNT < BITS_PER_MP_LIMB
 * 2. If the result is to be written over the input, WP must be >= UP.
 */

mpi_limb_t
_gcry_mpih_lshift( mpi_ptr_t wp, mpi_ptr_t up, mpi_size_t usize,
					    unsigned int cnt)
{
  mpi_limb_t high_limb, low_limb;
  unsigned sh_1, sh_2;
  mpi_size_t i;
  mpi_limb_t retval;

  sh_1 = cnt;
  wp += 1;
  sh_2 = BITS_PER_MPI_LIMB - sh_1;
  i = usize - 1;
  low_limb = up[i];
  retval = low_limb >> sh_2;
  high_limb = low_limb;
  while ( --i >= 0 ) 
    {
      low_limb = up[i];
      wp[i] = (high_limb << sh_1) | (low_limb >> sh_2);
      high_limb = low_limb;
    }
  wp[i] = high_limb << sh_1;

  return retval;
}


