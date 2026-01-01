	.text
	.align	2
	.globl	_gcry_mpih_sub_n
	.ent	_gcry_mpih_sub_n
_gcry_mpih_sub_n:
	.set	noreorder
	.set	nomacro

	ld	$10,0($5)
	ld	$11,0($6)

	daddiu	$7,$7,-1
	and	$9,$7,4-1	# number of limbs in first loop
	beq	$9,$0,.L0	# if multiple of 4 limbs, skip first loop
	move   $2,$0

	dsubu	$7,$7,$9

.Loop0: daddiu	$9,$9,-1
	ld	$12,8($5)
	daddu	$11,$11,$2
	ld	$13,8($6)
	sltu	$8,$11,$2
	dsubu	$11,$10,$11
	sltu	$2,$10,$11
	sd	$11,0($4)
	or	$2,$2,$8

	daddiu	$5,$5,8
	daddiu	$6,$6,8
	move	$10,$12
	move	$11,$13
	bne	$9,$0,.Loop0
	 daddiu $4,$4,8

.L0:	beq	$7,$0,.Lend
	 nop

.Loop:	daddiu	$7,$7,-4

	ld	$12,8($5)
	daddu	$11,$11,$2
	ld	$13,8($6)
	sltu	$8,$11,$2
	dsubu	$11,$10,$11
	sltu	$2,$10,$11
	sd	$11,0($4)
	or	$2,$2,$8

	ld	$10,16($5)
	daddu	$13,$13,$2
	ld	$11,16($6)
	sltu	$8,$13,$2
	dsubu	$13,$12,$13
	sltu	$2,$12,$13
	sd	$13,8($4)
	or	$2,$2,$8

	ld	$12,24($5)
	daddu	$11,$11,$2
	ld	$13,24($6)
	sltu	$8,$11,$2
	dsubu	$11,$10,$11
	sltu	$2,$10,$11
	sd	$11,16($4)
	or	$2,$2,$8

	ld	$10,32($5)
	daddu	$13,$13,$2
	ld	$11,32($6)
	sltu	$8,$13,$2
	dsubu	$13,$12,$13
	sltu	$2,$12,$13
	sd	$13,24($4)
	or	$2,$2,$8

	daddiu	$5,$5,32
	daddiu	$6,$6,32

	bne	$7,$0,.Loop
	 daddiu $4,$4,32

.Lend:	daddu	$11,$11,$2
	sltu	$8,$11,$2
	dsubu	$11,$10,$11
	sltu	$2,$10,$11
	sd	$11,0($4)
	j	$31
	or	$2,$2,$8

	.end	_gcry_mpih_sub_n

