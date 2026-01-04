#include "sysdep.h"
#include "asm-syntax.h"

/*******************
 * mpi_limb_t
 * _gcry_mpih_lshift( mpi_ptr_t wp,	rdi
 *		   mpi_ptr_t up,	rsi
 *		   mpi_size_t usize,	rdx
 *		   unsigned cnt)	rcx
 */

.text
	.globl C_SYMBOL_NAME(_gcry_mpih_lshift)
C_SYMBOL_NAME(_gcry_mpih_lshift:)
	movq	-8(%rsi,%rdx,8), %mm7
	movd	%ecx, %mm1
	movl	$64, %eax
	subl	%ecx, %eax
	movd	%eax, %mm0
	movq	%mm7, %mm3
	psrlq	%mm0, %mm7
	movd	%mm7, %rax
	subq	$2, %rdx
	jl	.Lendo

	ALIGN(4)			/* minimal alignment for claimed speed */
.Loop:	movq	(%rsi,%rdx,8), %mm6
	movq	%mm6, %mm2
	psrlq	%mm0, %mm6
	psllq	%mm1, %mm3
	por	%mm6, %mm3
	movq	%mm3, 8(%rdi,%rdx,8)
	je	.Lende
	movq	-8(%rsi,%rdx,8), %mm7
	movq	%mm7, %mm3
	psrlq	%mm0, %mm7
	psllq	%mm1, %mm2
	por	%mm7, %mm2
	movq	%mm2, (%rdi,%rdx,8)
	subq	$2, %rdx
	jge	.Loop

.Lendo:	movq	%mm3, %mm2
.Lende:	psllq	%mm1, %mm2
	movq	%mm2, (%rdi)
	emms
	ret
