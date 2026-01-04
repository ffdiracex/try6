#include "sysdep.h"
#include "asm-syntax.h"

/*******************
 * mpi_limb_t
 * _gcry_mpih_addmul_1( mpi_ptr_t res_ptr,      (sp + 4)
 *		     mpi_ptr_t s1_ptr,	     (sp + 8)
 *		     mpi_size_t s1_size,     (sp + 12)
 *		     mpi_limb_t s2_limb)     (sp + 16)
 */


	TEXT
	ALIGN
	GLOBL	C_SYMBOL_NAME(_gcry_mpih_addmul_1)

C_SYMBOL_NAME(_gcry_mpih_addmul_1:)
PROLOG(_gcry_mpih_addmul_1)

#define res_ptr a0
#define s1_ptr a1
#define s1_size d2
#define s2_limb d4

/* Save used registers on the stack.  */
	moveml	R(d2)-R(d5),MEM_PREDEC(sp)

/* Copy the arguments to registers.  Better use movem?	*/
	movel	MEM_DISP(sp,20),R(res_ptr)
	movel	MEM_DISP(sp,24),R(s1_ptr)
	movel	MEM_DISP(sp,28),R(s1_size)
	movel	MEM_DISP(sp,32),R(s2_limb)

	eorw	#1,R(s1_size)
	clrl	R(d1)
	clrl	R(d5)
	lsrl	#1,R(s1_size)
	bcc	L(L1)
	subql	#1,R(s1_size)
	subl	R(d0),R(d0)		/* (d0,cy) <= (0,0) */

L(Loop:)
	movel	MEM_POSTINC(s1_ptr),R(d3)
	mulul	R(s2_limb),R(d1):R(d3)
	addxl	R(d0),R(d3)
	addxl	R(d5),R(d1)
	addl	R(d3),MEM_POSTINC(res_ptr)
L(L1:)	movel	MEM_POSTINC(s1_ptr),R(d3)
	mulul	R(s2_limb),R(d0):R(d3)
	addxl	R(d1),R(d3)
	addxl	R(d5),R(d0)
	addl	R(d3),MEM_POSTINC(res_ptr)

	dbf	R(s1_size),L(Loop)
	addxl	R(d5),R(d0)
	subl	#0x10000,R(s1_size)
	bcc	L(Loop)

/* Restore used registers from stack frame.  */
	moveml	MEM_POSTINC(sp),R(d2)-R(d5)

	rts
EPILOG(_gcry_mpih_addmul_1)

