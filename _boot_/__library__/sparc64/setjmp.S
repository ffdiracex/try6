#include <holy/symbol.h>
#include <holy/dl.h>

        .file   "setjmp.S"

holy_MOD_LICENSE "GPLv2+"

        .text

/*
 * int holy_setjmp (holy_jmp_buf env)
 */
FUNCTION(holy_setjmp)
	stx	%o7, [%o0 + 0x00]
	stx	%sp, [%o0 + 0x08]
	stx	%fp, [%o0 + 0x10]
	retl
	 clr	%o0

/*
 * int holy_longjmp (holy_jmp_buf env, int val)
 */
FUNCTION(holy_longjmp)
	ldx	[%o0 + 0x10], %g1
	movrz	%o1, 1, %o1

	save %sp, -64, %sp
	flushw
	restore

	ldx	[%o0 + 0x00], %o7
	ldx	[%o0 + 0x08], %fp
	sub	%fp, 192, %sp
	stx	%g1, [%sp + 2047 + (14 * 8)]
	retl
	 restore %o1, 0, %o0
