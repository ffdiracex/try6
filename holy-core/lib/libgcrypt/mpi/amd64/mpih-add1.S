#include "sysdep.h"
#include "asm-syntax.h"


/*******************
 *  mpi_limb_t
 *  _gcry_mpih_add_n( mpi_ptr_t res_ptr,	rdi
 *		   mpi_ptr_t s1_ptr,		rsi
 *		   mpi_ptr_t s2_ptr,		rdx
 *		   mpi_size_t size)		rcx
 */

.text
	.globl C_SYMBOL_NAME(_gcry_mpih_add_n)
C_SYMBOL_NAME(_gcry_mpih_add_n:)
	leaq	(%rsi,%rcx,8), %rsi
	leaq	(%rdi,%rcx,8), %rdi
	leaq	(%rdx,%rcx,8), %rdx
	negq	%rcx
	xorl	%eax, %eax		/* clear cy */

	ALIGN(4)			/* minimal alignment for claimed speed */
.Loop:	movq	(%rsi,%rcx,8), %rax
	movq	(%rdx,%rcx,8), %r10
	adcq	%r10, %rax
	movq	%rax, (%rdi,%rcx,8)
	incq	%rcx
	jne	.Loop

	movq	%rcx, %rax		/* zero %rax */
	adcq	%rax, %rax
	ret
	
