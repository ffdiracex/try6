#include "sysdep.h"
#include "asm-syntax.h"


/*******************
 * mpi_limb_t
 * _gcry_mpih_rshift( mpi_ptr_t wp,	(sp + 4)
 *		   mpi_ptr_t up,	(sp + 8)
 *		   mpi_size_t usize,	(sp + 12)
 *		   unsigned cnt)	(sp + 16)
 */

.text
	ALIGN (3)
	.globl C_SYMBOL_NAME(_gcry_mpih_rshift)
C_SYMBOL_NAME(_gcry_mpih_rshift:)
	pushl	%edi
	pushl	%esi
	pushl	%ebx

	movl	16(%esp),%edi		/* wp */
	movl	20(%esp),%esi		/* up */
	movl	24(%esp),%edx		/* usize */
	movl	28(%esp),%ecx		/* cnt */

	leal	-4(%edi,%edx,4),%edi
	leal	(%esi,%edx,4),%esi
	negl	%edx

	movl	(%esi,%edx,4),%ebx	/* read least significant limb */
	xorl	%eax,%eax
	shrdl	%cl,%ebx,%eax		/* compute carry limb */
	incl	%edx
	jz	Lend2
	pushl	%eax			/* push carry limb onto stack */
	testb	$1,%dl
	jnz	L2			/* enter loop in the middle */
	movl	%ebx,%eax

	ALIGN (3)
Loop2:	 movl	 (%esi,%edx,4),%ebx	 /* load next higher limb */
	shrdl	%cl,%ebx,%eax		/* compute result limb */
	movl	%eax,(%edi,%edx,4)	/* store it */
	incl	%edx
L2:	movl	(%esi,%edx,4),%eax
	shrdl	%cl,%eax,%ebx
	movl	%ebx,(%edi,%edx,4)
	incl	%edx
	jnz	Loop2

	shrl	%cl,%eax		/* compute most significant limb */
	movl	%eax,(%edi)		/* store it */

	popl	%eax			/* pop carry limb */

	popl	%ebx
	popl	%esi
	popl	%edi
	ret

Lend2:	shrl	%cl,%ebx		/* compute most significant limb */
	movl	%ebx,(%edi)		/* store it */

	popl	%ebx
	popl	%esi
	popl	%edi
	ret

