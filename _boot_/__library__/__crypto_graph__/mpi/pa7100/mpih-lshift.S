	.code
	.export 	_gcry_mpih_lshift
	.label		_gcry_mpih_lshift
	.proc
	.callinfo	frame=64,no_calls
	.entry

	sh2add		%r24,%r25,%r25
	sh2add		%r24,%r26,%r26
	ldws,mb 	-4(0,%r25),%r22
	subi		32,%r23,%r1
	mtsar		%r1
	addib,= 	-1,%r24,L$0004
	vshd		%r0,%r22,%r28		; compute carry out limb
	ldws,mb 	-4(0,%r25),%r29
	addib,<=	-5,%r24,L$rest
	vshd		%r22,%r29,%r20

	.label	L$loop
	ldws,mb 	-4(0,%r25),%r22
	stws,mb 	%r20,-4(0,%r26)
	vshd		%r29,%r22,%r20
	ldws,mb 	-4(0,%r25),%r29
	stws,mb 	%r20,-4(0,%r26)
	vshd		%r22,%r29,%r20
	ldws,mb 	-4(0,%r25),%r22
	stws,mb 	%r20,-4(0,%r26)
	vshd		%r29,%r22,%r20
	ldws,mb 	-4(0,%r25),%r29
	stws,mb 	%r20,-4(0,%r26)
	addib,> 	-4,%r24,L$loop
	vshd		%r22,%r29,%r20

	.label	L$rest
	addib,= 	4,%r24,L$end1
	nop
	.label	L$eloop
	ldws,mb 	-4(0,%r25),%r22
	stws,mb 	%r20,-4(0,%r26)
	addib,<=	-1,%r24,L$end2
	vshd		%r29,%r22,%r20
	ldws,mb 	-4(0,%r25),%r29
	stws,mb 	%r20,-4(0,%r26)
	addib,> 	-1,%r24,L$eloop
	vshd		%r22,%r29,%r20

	.label	L$end1
	stws,mb 	%r20,-4(0,%r26)
	vshd		%r29,%r0,%r20
	bv		0(%r2)
	stw		%r20,-4(0,%r26)
	.label	L$end2
	stws,mb 	%r20,-4(0,%r26)
	.label	L$0004
	vshd		%r22,%r0,%r20
	bv		0(%r2)
	stw		%r20,-4(0,%r26)

	.exit
	.procend



