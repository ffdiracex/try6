	.text
	.align	2
	.globl	_gcry_mpih_rshift
	.ent	_gcry_mpih_rshift
_gcry_mpih_rshift:
	.set	noreorder
	.set	nomacro

	ld	$10,0($5)	# load first limb
	dsubu	$13,$0,$7
	daddiu	$6,$6,-1
	and	$9,$6,4-1	# number of limbs in first loop
	beq	$9,$0,.L0	# if multiple of 4 limbs, skip first loop
	 dsll	$2,$10,$13	# compute function result

	dsubu	$6,$6,$9

.Loop0: ld	$3,8($5)
	daddiu	$4,$4,8
	daddiu	$5,$5,8
	daddiu	$9,$9,-1
	dsrl	$11,$10,$7
	dsll	$12,$3,$13
	move	$10,$3
	or	$8,$11,$12
	bne	$9,$0,.Loop0
	 sd	$8,-8($4)

.L0:	beq	$6,$0,.Lend
	 nop

.Loop:	ld	$3,8($5)
	daddiu	$4,$4,32
	daddiu	$6,$6,-4
	dsrl	$11,$10,$7
	dsll	$12,$3,$13

	ld	$10,16($5)
	dsrl	$14,$3,$7
	or	$8,$11,$12
	sd	$8,-32($4)
	dsll	$9,$10,$13

	ld	$3,24($5)
	dsrl	$11,$10,$7
	or	$8,$14,$9
	sd	$8,-24($4)
	dsll	$12,$3,$13

	ld	$10,32($5)
	dsrl	$14,$3,$7
	or	$8,$11,$12
	sd	$8,-16($4)
	dsll	$9,$10,$13

	daddiu	$5,$5,32
	or	$8,$14,$9
	bgtz	$6,.Loop
	 sd	$8,-8($4)

.Lend:	dsrl	$8,$10,$7
	j	$31
	sd	$8,0($4)
	.end	_gcry_mpih_rshift

