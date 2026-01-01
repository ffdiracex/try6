#include "sysdep.h"
#include "asm-syntax.h"


/*******************
 *  mpi_limb_t
 *  _gcry_mpih_add_n( mpi_ptr_t res_ptr,	(sp + 4)
 *		   mpi_ptr_t s1_ptr,	(sp + 8)
 *		   mpi_ptr_t s2_ptr,	(sp + 16)
 *		   mpi_size_t size)	(sp + 12)
 */


	TEXT
	ALIGN
	GLOBL	C_SYMBOL_NAME(_gcry_mpih_add_n)

C_SYMBOL_NAME(_gcry_mpih_add_n:)
PROLOG(_gcry_mpih_add_n)
	/* Save used registers on the stack.  */
	movel	R(d2),MEM_PREDEC(sp)
	movel	R(a2),MEM_PREDEC(sp)

	/* Copy the arguments to registers.  Better use movem?	*/
	movel	MEM_DISP(sp,12),R(a2)
	movel	MEM_DISP(sp,16),R(a0)
	movel	MEM_DISP(sp,20),R(a1)
	movel	MEM_DISP(sp,24),R(d2)

	eorw	#1,R(d2)
	lsrl	#1,R(d2)
	bcc	L(L1)
	subql	#1,R(d2)	/* clears cy as side effect */

L(Loop:)
	movel	MEM_POSTINC(a0),R(d0)
	movel	MEM_POSTINC(a1),R(d1)
	addxl	R(d1),R(d0)
	movel	R(d0),MEM_POSTINC(a2)
L(L1:)	movel	MEM_POSTINC(a0),R(d0)
	movel	MEM_POSTINC(a1),R(d1)
	addxl	R(d1),R(d0)
	movel	R(d0),MEM_POSTINC(a2)

	dbf	R(d2),L(Loop)		/* loop until 16 lsb of %4 == -1 */
	subxl	R(d0),R(d0)	/* d0 <= -cy; save cy as 0 or -1 in d0 */
	subl	#0x10000,R(d2)
	bcs	L(L2)
	addl	R(d0),R(d0)	/* restore cy */
	bra	L(Loop)

L(L2:)
	negl	R(d0)

	/* Restore used registers from stack frame.  */
	movel	MEM_POSTINC(sp),R(a2)
	movel	MEM_POSTINC(sp),R(d2)

	rts
EPILOG(_gcry_mpih_add_n)


