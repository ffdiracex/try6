#include <holy/symbol.h>
#include <holy/dl.h>

	.file	"setjmp.S"
holy_MOD_LICENSE "GPLv2+"
	.text

/*
 * int holy_setjmp (holy_jmp_buf env)
 */
FUNCTION(holy_setjmp)
	stp	x19, x20, [x0], #16
	stp	x21, x22, [x0], #16
	stp	x23, x24, [x0], #16
	stp	x25, x26, [x0], #16
	stp	x27, x28, [x0], #16
	stp	x29, x30, [x0], #16
	mov	x1, sp
	str	x1, [x0]
	mov	x0, #0
	ret

/*
 * int holy_longjmp (holy_jmp_buf env, int val)
 */
FUNCTION(holy_longjmp)
	ldp	x19, x20, [x0], #16
	ldp	x21, x22, [x0], #16
	ldp	x23, x24, [x0], #16
	ldp	x25, x26, [x0], #16
	ldp	x27, x28, [x0], #16
	ldp	x29, x30, [x0], #16
	ldr	x2, [x0]
	mov	sp, x2
	mov	x0, #1
	cmp	x1, #0
	csel	x0, x1, x0, ne
	ret
