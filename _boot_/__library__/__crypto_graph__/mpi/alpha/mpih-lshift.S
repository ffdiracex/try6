	.set	noreorder
	.set	noat
.text
	.align	3
	.globl	_gcry_mpih_lshift
	.ent	_gcry_mpih_lshift
_gcry_mpih_lshift:
	.frame	$30,0,$26,0

	s8addq	$18,$17,$17	# make r17 point at end of s1
	ldq	$4,-8($17)	# load first limb
	subq	$17,8,$17
	subq	$31,$19,$7
	s8addq	$18,$16,$16	# make r16 point at end of RES
	subq	$18,1,$18
	and	$18,4-1,$20	# number of limbs in first loop
	srl	$4,$7,$0	# compute function result

	beq	$20,.L0
	subq	$18,$20,$18

	.align	3
.Loop0:
	ldq	$3,-8($17)
	subq	$16,8,$16
	subq	$17,8,$17
	subq	$20,1,$20
	sll	$4,$19,$5
	srl	$3,$7,$6
	bis	$3,$3,$4
	bis	$5,$6,$8
	stq	$8,0($16)
	bne	$20,.Loop0

.L0:	beq	$18,.Lend

	.align	3
.Loop:	ldq	$3,-8($17)
	subq	$16,32,$16
	subq	$18,4,$18
	sll	$4,$19,$5
	srl	$3,$7,$6

	ldq	$4,-16($17)
	sll	$3,$19,$1
	bis	$5,$6,$8
	stq	$8,24($16)
	srl	$4,$7,$2

	ldq	$3,-24($17)
	sll	$4,$19,$5
	bis	$1,$2,$8
	stq	$8,16($16)
	srl	$3,$7,$6

	ldq	$4,-32($17)
	sll	$3,$19,$1
	bis	$5,$6,$8
	stq	$8,8($16)
	srl	$4,$7,$2

	subq	$17,32,$17
	bis	$1,$2,$8
	stq	$8,0($16)

	bgt	$18,.Loop

.Lend:	sll	$4,$19,$8
	stq	$8,-8($16)
	ret	$31,($26),1
	.end	_gcry_mpih_lshift


