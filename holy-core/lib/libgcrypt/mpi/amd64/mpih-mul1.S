#include "sysdep.h"
#include "asm-syntax.h"

/*******************
 * mpi_limb_t
 * _gcry_mpih_mul_1( mpi_ptr_t res_ptr,	(rdi)
 *		  mpi_ptr_t s1_ptr,	(rsi)
 *		  mpi_size_t s1_size,	(rdx)
 *		  mpi_limb_t s2_limb)	(rcx)
 */


	TEXT
	ALIGN(5)
	.byte	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

	GLOBL	C_SYMBOL_NAME(_gcry_mpih_mul_1)
C_SYMBOL_NAME(_gcry_mpih_mul_1:)

	movq	%rdx, %r11
	leaq	(%rsi,%rdx,8), %rsi
	leaq	(%rdi,%rdx,8), %rdi
	negq	%r11
	xorl	%r8d, %r8d

.Loop:	movq	(%rsi,%r11,8), %rax
	mulq	%rcx
	addq	%r8, %rax
	movl	$0, %r8d
	adcq	%rdx, %r8
	movq	%rax, (%rdi,%r11,8)
	incq	%r11
	jne	.Loop

	movq	%r8, %rax
	ret
