	li 5, 0
1:	icbi 5, 3
	addi 5, 5, 32
	cmpw 5, 4
	blt 1b
