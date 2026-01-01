	.code
	.export 	_gcry_mpih_add_n
	.label		_gcry_mpih_add_n
	.proc
	.callinfo	frame=0,no_calls
	.entry

	ldws,ma 	4(0,%r25),%r20
	ldws,ma 	4(0,%r24),%r19

	addib,= 	-1,%r23,L$end	; check for (SIZE == 1)
	 add		%r20,%r19,%r28	; add first limbs ignoring cy

	.label L$loop
	ldws,ma 	4(0,%r25),%r20
	ldws,ma 	4(0,%r24),%r19
	stws,ma 	%r28,4(0,%r26)
	addib,<>	-1,%r23,L$loop
	 addc		%r20,%r19,%r28

	.label L$end
	stws		%r28,0(0,%r26)
	bv		0(%r2)
	 addc		%r0,%r0,%r28

	.exit
	.procend
