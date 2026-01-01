/* This file was automatically imported with 
   import_gcry.py. Please don't modify it */
/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef G10_MPI_INLINE_H
#define G10_MPI_INLINE_H

/* Starting with gcc 4.3 "extern inline" conforms in c99 mode to the
   c99 semantics.  To keep the useful old semantics we use an
   attribute.  */
#ifndef G10_MPI_INLINE_DECL
# ifdef __GNUC_STDC_INLINE__
#  define G10_MPI_INLINE_DECL  extern inline __attribute__ ((__gnu_inline__))
# else
#  define G10_MPI_INLINE_DECL  extern __inline__
# endif
#endif

G10_MPI_INLINE_DECL  mpi_limb_t
_gcry_mpih_add_1( mpi_ptr_t res_ptr, mpi_ptr_t s1_ptr,
	       mpi_size_t s1_size, mpi_limb_t s2_limb)
{
    mpi_limb_t x;

    x = *s1_ptr++;
    s2_limb += x;
    *res_ptr++ = s2_limb;
    if( s2_limb < x ) { /* sum is less than the left operand: handle carry */
	while( --s1_size ) {
	    x = *s1_ptr++ + 1;	/* add carry */
	    *res_ptr++ = x;	/* and store */
	    if( x )		/* not 0 (no overflow): we can stop */
		goto leave;
	}
	return 1; /* return carry (size of s1 to small) */
    }

  leave:
    if( res_ptr != s1_ptr ) { /* not the same variable */
	mpi_size_t i;	       /* copy the rest */
	for( i=0; i < s1_size-1; i++ )
	    res_ptr[i] = s1_ptr[i];
    }
    return 0; /* no carry */
}



G10_MPI_INLINE_DECL mpi_limb_t
_gcry_mpih_add(mpi_ptr_t res_ptr, mpi_ptr_t s1_ptr, mpi_size_t s1_size,
			       mpi_ptr_t s2_ptr, mpi_size_t s2_size)
{
    mpi_limb_t cy = 0;

    if( s2_size )
	cy = _gcry_mpih_add_n( res_ptr, s1_ptr, s2_ptr, s2_size );

    if( s1_size - s2_size )
	cy = _gcry_mpih_add_1( res_ptr + s2_size, s1_ptr + s2_size,
			    s1_size - s2_size, cy);
    return cy;
}


G10_MPI_INLINE_DECL mpi_limb_t
_gcry_mpih_sub_1(mpi_ptr_t res_ptr,  mpi_ptr_t s1_ptr,
	      mpi_size_t s1_size, mpi_limb_t s2_limb )
{
    mpi_limb_t x;

    x = *s1_ptr++;
    s2_limb = x - s2_limb;
    *res_ptr++ = s2_limb;
    if( s2_limb > x ) {
	while( --s1_size ) {
	    x = *s1_ptr++;
	    *res_ptr++ = x - 1;
	    if( x )
		goto leave;
	}
	return 1;
    }

  leave:
    if( res_ptr != s1_ptr ) {
	mpi_size_t i;
	for( i=0; i < s1_size-1; i++ )
	    res_ptr[i] = s1_ptr[i];
    }
    return 0;
}



G10_MPI_INLINE_DECL   mpi_limb_t
_gcry_mpih_sub( mpi_ptr_t res_ptr, mpi_ptr_t s1_ptr, mpi_size_t s1_size,
				mpi_ptr_t s2_ptr, mpi_size_t s2_size)
{
    mpi_limb_t cy = 0;

    if( s2_size )
	cy = _gcry_mpih_sub_n(res_ptr, s1_ptr, s2_ptr, s2_size);

    if( s1_size - s2_size )
	cy = _gcry_mpih_sub_1(res_ptr + s2_size, s1_ptr + s2_size,
				      s1_size - s2_size, cy);
    return cy;
}

/****************
 * Compare OP1_PTR/OP1_SIZE with OP2_PTR/OP2_SIZE.
 * There are no restrictions on the relative sizes of
 * the two arguments.
 * Return 1 if OP1 > OP2, 0 if they are equal, and -1 if OP1 < OP2.
 */
G10_MPI_INLINE_DECL int
_gcry_mpih_cmp( mpi_ptr_t op1_ptr, mpi_ptr_t op2_ptr, mpi_size_t size )
{
    mpi_size_t i;
    mpi_limb_t op1_word, op2_word;

    for( i = size - 1; i >= 0 ; i--) {
	op1_word = op1_ptr[i];
	op2_word = op2_ptr[i];
	if( op1_word != op2_word )
	    goto diff;
    }
    return 0;

  diff:
    /* This can *not* be simplified to
     *	 op2_word - op2_word
     * since that expression might give signed overflow.  */
    return (op1_word > op2_word) ? 1 : -1;
}


#endif /*G10_MPI_INLINE_H*/
