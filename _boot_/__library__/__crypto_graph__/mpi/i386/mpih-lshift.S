#include "sysdep.h"
#include "asm-syntax.h"


/*******************
 * mpi_limb_t
 * _gcry_mpih_lshift( mpi_ptr_t wp,	(sp + 4)
 *		   mpi_ptr_t up,	(sp + 8)
 *		   mpi_size_t usize,	(sp + 12)
 *		   unsigned cnt)	(sp + 16)
 */

.text
	ALIGN (3)
	.globl C_SYMBOL_NAME(_gcry_mpih_lshift)
C_SYMBOL_NAME(_gcry_mpih_lshift:)
	pushl	%edi
	pushl	%esi
	pushl	%ebx

	movl	16(%esp),%edi		/* res_ptr */
	movl	20(%esp),%esi		/* s_ptr */
	movl	24(%esp),%edx		/* size */
	movl	28(%esp),%ecx		/* cnt */

	subl	$4,%esi 		/* adjust s_ptr */

	movl	(%esi,%edx,4),%ebx	/* read most significant limb */
	xorl	%eax,%eax
	shldl	%cl,%ebx,%eax		/* compute carry limb */
	decl	%edx
	jz	Lend
	pushl	%eax			/* push carry limb onto stack */
	testb	$1,%dl
	jnz	L1			/* enter loop in the middle */
	movl	%ebx,%eax

	ALIGN (3)
Loop:	movl	(%esi,%edx,4),%ebx	/* load next lower limb */
	shldl	%cl,%ebx,%eax		/* compute result limb */
	movl	%eax,(%edi,%edx,4)	/* store it */
	decl	%edx
L1:	movl	(%esi,%edx,4),%eax
	shldl	%cl,%eax,%ebx
	movl	%ebx,(%edi,%edx,4)
	decl	%edx
	jnz	Loop

	shll	%cl,%eax		/* compute least significant limb */
	movl	%eax,(%edi)		/* store it */

	popl	%eax			/* pop carry limb */

	popl	%ebx
	popl	%esi
	popl	%edi
	ret

Lend:	shll	%cl,%ebx		/* compute least significant limb */
	movl	%ebx,(%edi)		/* store it */

	popl	%ebx
	popl	%esi
	popl	%edi
	ret

