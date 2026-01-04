#include <holy/symbol.h>
	
	.p2align	4	/* force 16-byte alignment */

VARIABLE (holy_relocator_forward_start)
	mr 3, 9
	mr 4, 10

copycont1:	
	lbz 11,0(8)
	stb 11,0(9)
	addi 8, 8, 0x1
	addi 9, 9, 0x1
	addi 10, 10, -1
	cmpwi 10, 0
	bne copycont1

#include "../../kern/powerpc/cache_flush.S"

VARIABLE (holy_relocator_forward_end)

VARIABLE (holy_relocator_backward_start)
	mr 3, 9
	mr 4, 10

	add 9, 9, 10
	add 8, 8, 10
	/* Backward movsl is implicitly off-by-one.  compensate that.  */
	addi 9, 9, -1
	addi 8, 8, -1
copycont2:	
	lbz 11,0(8)
	stb 11,0(9)
	addi 8, 8, -1
	addi 9, 9, -1
	addi 10, 10, -1
	cmpwi 10, 0
	bne copycont2

#include "../../kern/powerpc/cache_flush.S"

VARIABLE (holy_relocator_backward_end)
