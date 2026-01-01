	.code
	.export 	_gcry_mpih_rshift
	.label		_gcry_mpih_rshift
	.proc
	.callinfo	frame=64,no_calls
	.entry

	ldws,ma 	4(0,%r25),%r22
	mtsar		%r23
	addib,= 	-1,%r24,L$r004
	vshd		%r22,%r0,%r28		; compute carry out limb
	ldws,ma 	4(0,%r25),%r29
	addib,<=	-5,%r24,L$rrest
	vshd		%r29,%r22,%r20

	.label	L$roop
	ldws,ma 	4(0,%r25),%r22
	stws,ma 	%r20,4(0,%r26)
	vshd		%r22,%r29,%r20
	ldws,ma 	4(0,%r25),%r29
	stws,ma 	%r20,4(0,%r26)
	vshd		%r29,%r22,%r20
	ldws,ma 	4(0,%r25),%r22
	stws,ma 	%r20,4(0,%r26)
	vshd		%r22,%r29,%r20
	ldws,ma 	4(0,%r25),%r29
	stws,ma 	%r20,4(0,%r26)
	addib,> 	-4,%r24,L$roop
	vshd		%r29,%r22,%r20

	.label	L$rrest
	addib,= 	4,%r24,L$rend1
	nop
	.label	L$eroop
	ldws,ma 	4(0,%r25),%r22
	stws,ma 	%r20,4(0,%r26)
	addib,<=	-1,%r24,L$rend2
	vshd		%r22,%r29,%r20
	ldws,ma 	4(0,%r25),%r29
	stws,ma 	%r20,4(0,%r26)
	addib,> 	-1,%r24,L$eroop
	vshd		%r29,%r22,%r20

	.label	L$rend1
	stws,ma		%r20,4(0,%r26)
	vshd		%r0,%r29,%r20
	bv		0(%r2)
	stw		%r20,0(0,%r26)
	.label	L$rend2
	stws,ma		%r20,4(0,%r26)
	.label	L$r004
	vshd		%r0,%r22,%r20
	bv		0(%r2)
	stw		%r20,0(0,%r26)

	.exit
	.procend


