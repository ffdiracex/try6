	.set	noreorder
	.set	noat
.text
	.align	3
	.globl	_gcry_mpih_mul_1
	.ent	_gcry_mpih_mul_1 2
_gcry_mpih_mul_1:
	.frame	$30,0,$26

	ldq	$2,0($17)	# $2 = s1_limb
	subq	$18,1,$18	# size--
	mulq	$2,$19,$3	# $3 = prod_low
	bic	$31,$31,$4	# clear cy_limb
	umulh	$2,$19,$0	# $0 = prod_high
	beq	$18,Lend1	# jump if size was == 1
	ldq	$2,8($17)	# $2 = s1_limb
	subq	$18,1,$18	# size--
	stq	$3,0($16)
	beq	$18,Lend2	# jump if size was == 2

	.align	3
Loop:	mulq	$2,$19,$3	# $3 = prod_low
	addq	$4,$0,$0	# cy_limb = cy_limb + 'cy'
	subq	$18,1,$18	# size--
	umulh	$2,$19,$4	# $4 = cy_limb
	ldq	$2,16($17)	# $2 = s1_limb
	addq	$17,8,$17	# s1_ptr++
	addq	$3,$0,$3	# $3 = cy_limb + prod_low
	stq	$3,8($16)
	cmpult	$3,$0,$0	# $0 = carry from (cy_limb + prod_low)
	addq	$16,8,$16	# res_ptr++
	bne	$18,Loop

Lend2:	mulq	$2,$19,$3	# $3 = prod_low
	addq	$4,$0,$0	# cy_limb = cy_limb + 'cy'
	umulh	$2,$19,$4	# $4 = cy_limb
	addq	$3,$0,$3	# $3 = cy_limb + prod_low
	cmpult	$3,$0,$0	# $0 = carry from (cy_limb + prod_low)
	stq	$3,8($16)
	addq	$4,$0,$0	# cy_limb = prod_high + cy
	ret	$31,($26),1
Lend1:	stq	$3,0($16)
	ret	$31,($26),1

	.end	_gcry_mpih_mul_1


