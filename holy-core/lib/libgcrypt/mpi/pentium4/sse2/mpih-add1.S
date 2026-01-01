#include "sysdep.h"
#include "asm-syntax.h"


	/*******************
 *  mpi_limb_t
 *  _gcry_mpih_add_n( mpi_ptr_t res_ptr,	(sp + 4)
 *		   mpi_ptr_t s1_ptr,	(sp + 8)
 *		   mpi_ptr_t s2_ptr,	(sp + 12)
 *		   mpi_size_t size)	(sp + 16)
 *
 * P4 Willamette, Northwood: 4.0 cycles/limb if dst!=src1 and dst!=src2
 *			    6.0 cycles/limb if dst==src1 or dst==src2
 * P4 Prescott:		    >= 5 cycles/limb
 *
 * The 4 c/l achieved here isn't particularly good, but is better than 9 c/l
 * for a basic adc loop.
 */

	TEXT
	ALIGN (3)
	GLOBL C_SYMBOL_NAME(_gcry_mpih_add_n)
C_SYMBOL_NAME(_gcry_mpih_add_n:)

	pxor	%mm0, %mm0
	
	movl	8(%esp), %eax		/* s1_ptr */
	movl	%ebx, 8(%esp)		/* re-use parameter space */
	movl	12(%esp), %ebx		/* res_ptr */
	movl	4(%esp), %edx		/* s2_ptr */
	movl	16(%esp), %ecx		/* size */

	leal	(%eax,%ecx,4), %eax	/* src1 end */
	leal	(%ebx,%ecx,4), %ebx	/* src2 end */
	leal	(%edx,%ecx,4), %edx	/* dst end */
	negl	%ecx			/* -size */

Ltop:
/*
	C eax	src1 end
	C ebx	src2 end
	C ecx	counter, limbs, negative
	C edx	dst end
	C mm0	carry bit
*/

	movd	(%eax,%ecx,4), %mm1
	movd	(%ebx,%ecx,4), %mm2
	paddq	%mm2, %mm1

	paddq	%mm1, %mm0
	movd	%mm0, (%edx,%ecx,4)

	psrlq	$32, %mm0

	addl	$1, %ecx
	jnz	Ltop


	movd	%mm0, %eax
	movl	8(%esp), %ebx	/* restore saved EBX */
	emms
	ret
