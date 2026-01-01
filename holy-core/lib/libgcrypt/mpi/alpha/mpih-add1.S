	.set	noreorder
	.set	noat
.text
	.align	3
	.globl	_gcry_mpih_add_n
	.ent	_gcry_mpih_add_n
_gcry_mpih_add_n:
	.frame	$30,0,$26,0

	ldq	$3,0($17)
	ldq	$4,0($18)

	subq	$19,1,$19
	and	$19,4-1,$2	# number of limbs in first loop
	bis	$31,$31,$0
	beq	$2,.L0		# if multiple of 4 limbs, skip first loop

	subq	$19,$2,$19

.Loop0: subq	$2,1,$2
	ldq	$5,8($17)
	addq	$4,$0,$4
	ldq	$6,8($18)
	cmpult	$4,$0,$1
	addq	$3,$4,$4
	cmpult	$4,$3,$0
	stq	$4,0($16)
	or	$0,$1,$0

	addq	$17,8,$17
	addq	$18,8,$18
	bis	$5,$5,$3
	bis	$6,$6,$4
	addq	$16,8,$16
	bne	$2,.Loop0

.L0:	beq	$19,.Lend

	.align	3
.Loop:	subq	$19,4,$19

	ldq	$5,8($17)
	addq	$4,$0,$4
	ldq	$6,8($18)
	cmpult	$4,$0,$1
	addq	$3,$4,$4
	cmpult	$4,$3,$0
	stq	$4,0($16)
	or	$0,$1,$0

	ldq	$3,16($17)
	addq	$6,$0,$6
	ldq	$4,16($18)
	cmpult	$6,$0,$1
	addq	$5,$6,$6
	cmpult	$6,$5,$0
	stq	$6,8($16)
	or	$0,$1,$0

	ldq	$5,24($17)
	addq	$4,$0,$4
	ldq	$6,24($18)
	cmpult	$4,$0,$1
	addq	$3,$4,$4
	cmpult	$4,$3,$0
	stq	$4,16($16)
	or	$0,$1,$0

	ldq	$3,32($17)
	addq	$6,$0,$6
	ldq	$4,32($18)
	cmpult	$6,$0,$1
	addq	$5,$6,$6
	cmpult	$6,$5,$0
	stq	$6,24($16)
	or	$0,$1,$0

	addq	$17,32,$17
	addq	$18,32,$18
	addq	$16,32,$16
	bne	$19,.Loop

.Lend:	addq	$4,$0,$4
	cmpult	$4,$0,$1
	addq	$3,$4,$4
	cmpult	$4,$3,$0
	stq	$4,0($16)
	or	$0,$1,$0
	ret	$31,($26),1

	.end	_gcry_mpih_add_n

