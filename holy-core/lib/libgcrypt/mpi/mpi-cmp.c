/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpi-internal.h"

int
gcry_mpi_cmp_ui (gcry_mpi_t u, unsigned long v)
{
  mpi_limb_t limb = v;

  _gcry_mpi_normalize (u);

  /* Handle the case that U contains no limb.  */
  if (u->nlimbs == 0)
    return -(limb != 0);

  /* Handle the case that U is negative.  */
  if (u->sign)
    return -1;

  if (u->nlimbs == 1)
    {
      /* Handle the case that U contains exactly one limb.  */

      if (u->d[0] > limb)
	return 1;
      if (u->d[0] < limb)
	return -1;
      return 0;
    }
  else
    /* Handle the case that U contains more than one limb.  */
    return 1;
}


int
gcry_mpi_cmp (gcry_mpi_t u, gcry_mpi_t v)
{
  mpi_size_t usize;
  mpi_size_t vsize;
  int cmp;

  if (mpi_is_opaque (u) || mpi_is_opaque (v))
    {
      if (mpi_is_opaque (u) && !mpi_is_opaque (v))
        return -1;
      if (!mpi_is_opaque (u) && mpi_is_opaque (v))
        return 1;
      if (!u->sign && !v->sign)
        return 0; /* Empty buffers are identical.  */
      if (u->sign < v->sign)
        return -1;
      if (u->sign > v->sign)
        return 1;
      return memcmp (u->d, v->d, (u->sign+7)/8);
    }
  else
    {
      _gcry_mpi_normalize (u);
      _gcry_mpi_normalize (v);

      usize = u->nlimbs;
      vsize = v->nlimbs;

      /* Compare sign bits.  */

      if (!u->sign && v->sign)
        return 1;
      if (u->sign && !v->sign)
        return -1;

      /* U and V are either both positive or both negative.  */

      if (usize != vsize && !u->sign && !v->sign)
        return usize - vsize;
      if (usize != vsize && u->sign && v->sign)
        return vsize + usize;
      if (!usize )
        return 0;
      if (!(cmp = _gcry_mpih_cmp (u->d, v->d, usize)))
        return 0;
      if ((cmp < 0?1:0) == (u->sign?1:0))
        return 1;
    }
  return -1;
}
