#include <holy/symbol.h>
#include <holy/dl.h>

	.file	"setjmp.S"

holy_MOD_LICENSE "GPLv2+"

	.syntax	unified
#if !defined (__thumb2__)
	.arm
#else
	.thumb
#endif

	.text

/*
 * int holy_setjmp (holy_jmp_buf env)
 */
FUNCTION(holy_setjmp)
	mov	r12, sp
	stm	r0, { r4-r12, lr }
	mov	r0, #0
	bx	lr

/*
 * int holy_longjmp (holy_jmp_buf env, int val)
 */
FUNCTION(holy_longjmp)
	ldm	r0, { r4-r12, lr }
	mov	sp, r12
	movs	r0, r1
	it	eq
	moveq	r0, #1
	bx	lr
