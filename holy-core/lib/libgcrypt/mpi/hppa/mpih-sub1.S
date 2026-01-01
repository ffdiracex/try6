#include "sysdep.h"
#include "asm-syntax.h"


/*******************
 *  mpi_limb_t
 *  _gcry_mpih_sub_n( mpi_ptr_t res_ptr,	(gr26)
 *		   mpi_ptr_t s1_ptr,	(gr25)
 *		   mpi_ptr_t s2_ptr,	(gr24)
 *		   mpi_size_t size)	(gr23)
 *
 * One might want to unroll this as for other processors, but it turns
 * out that the data cache contention after a store makes such
 * unrolling useless.  We can't come under 5 cycles/limb anyway.
 */


	.code
	.export 	_gcry_mpih_sub_n
	.label		_gcry_mpih_sub_n
	.proc
	.callinfo	frame=0,no_calls
	.entry

	ldws,ma 	4(0,%r25),%r20
	ldws,ma 	4(0,%r24),%r19

	addib,= 	-1,%r23,L$end	; check for (SIZE == 1)
	 sub		%r20,%r19,%r28	; subtract first limbs ignoring cy

	.label	L$loop
	ldws,ma 	4(0,%r25),%r20
	ldws,ma 	4(0,%r24),%r19
	stws,ma 	%r28,4(0,%r26)
	addib,<>	-1,%r23,L$loop
	 subb		%r20,%r19,%r28

	.label	L$end
	stws		%r28,0(0,%r26)
	addc		%r0,%r0,%r28
	bv		0(%r2)
	 subi		1,%r28,%r28

	.exit
	.procend



