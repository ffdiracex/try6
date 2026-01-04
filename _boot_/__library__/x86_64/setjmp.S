#include <holy/symbol.h>
#include <holy/dl.h>

	.file	"setjmp.S"

holy_MOD_LICENSE "GPLv2+"

	.text

/*
 *  jmp_buf:
 *   rbx rsp rbp r12 r13 r14 r15 rip
 *   0   8   16  24  32  40  48  56
 */

/*
 * int holy_setjmp (holy_jmp_buf env)
 */
FUNCTION(holy_setjmp)
	pop	%rsi		/* Return address, and adjust the stack */
	xorq	%rax, %rax
	movq	%rbx, 0(%rdi)	/* RBX */
	movq	%rsp, 8(%rdi)   /* RSP */
	push	%rsi
	movq	%rbp, 16(%rdi)	/* RBP */
	movq	%r12, 24(%rdi)	/* R12 */
	movq	%r13, 32(%rdi)	/* R13 */
	movq	%r14, 40(%rdi)	/* R14 */
	movq	%r15, 48(%rdi)	/* R15 */
	movq	%rsi, 56(%rdi)	/* RSI */
	ret

/*
 * int holy_longjmp (holy_jmp_buf env, int val)
 */
FUNCTION(holy_longjmp)
	movl	%esi, %eax
	orl	%eax, %eax
	jnz	1f
	incl	%eax
1:

	movq	(%rdi), %rbx
	movq	8(%rdi), %rsp
	movq	16(%rdi), %rbp
	movq	24(%rdi), %r12
	movq    32(%rdi), %r13
	movq    40(%rdi), %r14
	movq    48(%rdi), %r15
	jmp	*56(%rdi)
