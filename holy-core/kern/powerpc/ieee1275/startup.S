#include <holy/symbol.h>
#include <holy/offsets.h>

.extern __bss_start
.extern _end

	.text
	.align	2
	.globl	start, _start
start:
_start:
	li      2, 0
	li      13, 0

	/* Stage1 won't zero BSS for us. In other cases, why not do it again?  */
	lis	6, (__bss_start - 4)@h
	ori	6, 6, (__bss_start - 4)@l

2:	stb     2, 4(6)
	addi    6, 6, 1
	andi.   7, 6, 3
	cmpi    0, 1, 7, 0
	bne	2b
	
	lis	7, (_end - 4)@h
	ori	7, 7, (_end - 4)@l
	subf	7, 6, 7
	subi    8, 7, 1
	andi.   8, 8, 3
	addi    8, 8, 1
	sub	7, 7, 8

	srwi	7, 7, 2 /* We store 4 bytes at a time.  */
	mtctr	7
2:	stwu	2, 4(6) /* We know r2 is already 0 from above.  */
	bdnz	2b

	mtctr   8
2:	stb	2, 4(6) /* We know r2 is already 0 from above.  */
	addi    6, 6, 1
	bdnz	2b

	/* Store r5 in holy_ieee1275_entry_fn.  */
	lis	9, holy_ieee1275_entry_fn@ha
	stw	5, holy_ieee1275_entry_fn@l(9)

	bl	holy_main
1:	b	1b
