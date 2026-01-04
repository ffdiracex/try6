/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpi-internal.h"


/* Shift U (pointed to by UP and USIZE limbs long) CNT bits to the right
 * and store the USIZE least significant limbs of the result at WP.
 * The bits shifted out to the right are returned.
 *
 * Argument constraints:
 * 1. 0 < CNT < BITS_PER_MP_LIMB
 * 2. If the result is to be written over the input, WP must be <= UP.
 */

mpi_limb_t
_gcry_mpih_rshift( mpi_ptr_t wp, mpi_ptr_t up, mpi_size_t usize, unsigned cnt)
{
  mpi_limb_t high_limb, low_limb;
  unsigned sh_1, sh_2;
  mpi_size_t i;
  mpi_limb_t retval;

  sh_1 = cnt;
  wp -= 1;
  sh_2 = BITS_PER_MPI_LIMB - sh_1;
  high_limb = up[0];
  retval = high_limb << sh_2;
  low_limb = high_limb;
  for (i=1; i < usize; i++)
    {
      high_limb = up[i];
      wp[i] = (low_limb >> sh_1) | (high_limb << sh_2);
      low_limb = high_limb;
    }
  wp[i] = low_limb >> sh_1;

  return retval;
}

