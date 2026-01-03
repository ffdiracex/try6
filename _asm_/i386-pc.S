	.code16
	jmp far
	.org 4
	jmp nearer
	.org 6
	movl nearer, %ebx
	.org 11
	.space 100
nearer:
	.space 200
far:
	.byte 0
