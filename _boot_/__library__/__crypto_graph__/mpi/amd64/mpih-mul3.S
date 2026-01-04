#include "sysdep.h"
#include "asm-syntax.h"


/*******************
 * mpi_limb_t
 * _gcry_mpih_submul_1( mpi_ptr_t res_ptr,   (rdi)
 *		     mpi_ptr_t s1_ptr,	     (rsi)
 *		     mpi_size_t s1_size,     (rdx)
 *		     mpi_limb_t s2_limb)     (rcx)
 */
	TEXT
	GLOBL	C_SYMBOL_NAME(_gcry_mpih_submul_1)
C_SYMBOL_NAME(_gcry_mpih_submul_1:)

	movq	%rdx, %r11
	leaq	(%rsi,%r11,8), %rsi
	leaq	(%rdi,%r11,8), %rdi
	negq	%r11
	xorl	%r8d, %r8d

	ALIGN(3)			/* minimal alignment for claimed speed */
.Loop:	movq	(%rsi,%r11,8), %rax
	movq	(%rdi,%r11,8), %r10
	mulq	%rcx
	subq	%r8, %r10
	movl	$0, %r8d
	adcl	%r8d, %r8d
	subq	%rax, %r10
	adcq	%rdx, %r8
	movq	%r10, (%rdi,%r11,8)
	incq	%r11
	jne	.Loop

	movq	%r8, %rax
	ret
