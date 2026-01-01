	.set mips3
	sync
	ld $t2, 0($t1)

a:
	addiu $t1, $s0, (b - a)
b:	nop
