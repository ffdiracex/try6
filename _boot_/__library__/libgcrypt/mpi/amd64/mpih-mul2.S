#include "sysdep.h"
#include "asm-syntax.h"




#include "sysdep.h"
#include "asm-syntax.h"


/*******************
 * mpi_limb_t
 * _gcry_mpih_addmul_1( mpi_ptr_t res_ptr,   (rdi)
 *		     mpi_ptr_t s1_ptr,	     (rsi)
 *		     mpi_size_t s1_size,     (rdx)
 *		     mpi_limb_t s2_limb)     (rcx)
 */
	TEXT
	GLOBL	C_SYMBOL_NAME(_gcry_mpih_addmul_1)
C_SYMBOL_NAME(_gcry_mpih_addmul_1:)
	movq	%rdx, %r11
	leaq	(%rsi,%rdx,8), %rsi
	leaq	(%rdi,%rdx,8), %rdi
	negq	%r11
	xorl	%r8d, %r8d
	xorl	%r10d, %r10d

	ALIGN(3)			/* minimal alignment for claimed speed */
.Loop:	movq	(%rsi,%r11,8), %rax
	mulq	%rcx
	addq	(%rdi,%r11,8), %rax
	adcq	%r10, %rdx
	addq	%r8, %rax
	movq	%r10, %r8
	movq	%rax, (%rdi,%r11,8)
	adcq	%rdx, %r8
	incq	%r11
	jne	.Loop

	movq	%r8, %rax
	ret
