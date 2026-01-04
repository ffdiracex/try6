#include <holy/symbol.h>
#include <holy/dl.h>

	.file	"setjmp.S"

holy_MOD_LICENSE "GPLv2+"

	.text

/*
 * int holy_setjmp (holy_jmp_buf env)
 */
FUNCTION(holy_setjmp)
	stw	1, 0(3)
	stw	14, 4(3)
	stw	15, 8(3)
	stw	16, 12(3)
	stw	17, 16(3)
	stw	18, 20(3)
	stw	19, 24(3)
	stw	20, 28(3)
	stw	21, 32(3)
	stw	22, 36(3)
	stw	23, 40(3)
	stw	24, 44(3)
	stw	25, 48(3)
	stw	26, 52(3)
	stw	27, 56(3)
	stw	28, 60(3)
	stw	29, 64(3)
	stw	30, 68(3)
	stw	31, 72(3)
	mflr	4
	stw	4, 76(3)
	mfcr	4
	stw	4, 80(3)
	li	3, 0
	blr

/*
 * int holy_longjmp (holy_jmp_buf env, int val)
 */
FUNCTION(holy_longjmp)
	lwz	1, 0(3)
	lwz	14, 4(3)
	lwz	15, 8(3)
	lwz	16, 12(3)
	lwz	17, 16(3)
	lwz	18, 20(3)
	lwz	19, 24(3)
	lwz	20, 28(3)
	lwz	21, 32(3)
	lwz	22, 36(3)
	lwz	23, 40(3)
	lwz	24, 44(3)
	lwz	25, 48(3)
	lwz	26, 52(3)
	lwz	27, 56(3)
	lwz	28, 60(3)
	lwz	29, 64(3)
	lwz	30, 68(3)
	lwz	31, 72(3)
	lwz	5, 76(3)
	mtlr	5
	lwz	5, 80(3)
	mtcr	5
	mr.	3, 4
	bne	1f
	li	3, 1
1:	blr

