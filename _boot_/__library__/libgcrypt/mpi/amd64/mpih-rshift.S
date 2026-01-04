#include "sysdep.h"
#include "asm-syntax.h"

/*******************
 * mpi_limb_t
 * _gcry_mpih_rshift( mpi_ptr_t wp,	rdi
 *		   mpi_ptr_t up,	rsi
 *		   mpi_size_t usize,	rdx
 *		   unsigned cnt)	rcx
 */

.text
	.globl C_SYMBOL_NAME(_gcry_mpih_rshift)
C_SYMBOL_NAME(_gcry_mpih_rshift:)
	movq	(%rsi), %mm7
	movd	%ecx, %mm1
	movl	$64, %eax
	subl	%ecx, %eax
	movd	%eax, %mm0
	movq	%mm7, %mm3
	psllq	%mm0, %mm7
	movd	%mm7, %rax
	leaq	(%rsi,%rdx,8), %rsi
	leaq	(%rdi,%rdx,8), %rdi
	negq	%rdx
	addq	$2, %rdx
	jg	.Lendo

	ALIGN(8)			/* minimal alignment for claimed speed */
.Loop:	movq	-8(%rsi,%rdx,8), %mm6
	movq	%mm6, %mm2
	psllq	%mm0, %mm6
	psrlq	%mm1, %mm3
	por	%mm6, %mm3
	movq	%mm3, -16(%rdi,%rdx,8)
	je	.Lende
	movq	(%rsi,%rdx,8), %mm7
	movq	%mm7, %mm3
	psllq	%mm0, %mm7
	psrlq	%mm1, %mm2
	por	%mm7, %mm2
	movq	%mm2, -8(%rdi,%rdx,8)
	addq	$2, %rdx
	jle	.Loop

.Lendo:	movq	%mm3, %mm2
.Lende:	psrlq	%mm1, %mm2
	movq	%mm2, -8(%rdi)
	emms
	ret
