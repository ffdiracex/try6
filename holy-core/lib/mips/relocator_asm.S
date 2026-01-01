#include <holy/symbol.h>
	
	.p2align	4	/* force 16-byte alignment */

	.set noreorder
	.set nomacro

VARIABLE (holy_relocator_forward_start)
	move $a0, $9
	move $a1, $10

copycont1:	
	lb $11,0($8)
	sb $11,0($9)
	addiu $8, $8, 1
	addiu $10, $10, -1
	bne $10, $0, copycont1
	 addiu $9, $9, 1

#include "../../kern/mips/cache_flush.S"

VARIABLE (holy_relocator_forward_end)

VARIABLE (holy_relocator_backward_start)
	move $a0, $9
	move $a1, $10

	addu $9, $9, $10
	addu $8, $8, $10
	/* Backward movsl is implicitly off-by-one.  compensate that.  */
	addiu $9, $9, -1
	addiu $8, $8, -1
copycont2:	
	lb $11,0($8)
	sb $11,0($9)
	addiu $8, $8, -1
	addiu $10, $10, -1
	bne $10, $0, copycont2
	 addiu $9, $9, -1

#include "../../kern/mips/cache_flush.S"

VARIABLE (holy_relocator_backward_end)
