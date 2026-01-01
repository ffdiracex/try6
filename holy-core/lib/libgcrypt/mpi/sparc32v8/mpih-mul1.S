! INPUT PARAMETERS
! res_ptr	o0
! s1_ptr	o1
! size		o2
! s2_limb	o3

#include "sysdep.h"

.text
	.align	8
	.global C_SYMBOL_NAME(_gcry_mpih_mul_1)
C_SYMBOL_NAME(_gcry_mpih_mul_1):
	sll	%o2,4,%g1
	and	%g1,(4-1)<<4,%g1
#if PIC
	mov	%o7,%g4 		! Save return address register
	call	1f
	add	%o7,LL-1f,%g3
1:	mov	%g4,%o7 		! Restore return address register
#else
	sethi	%hi(LL),%g3
	or	%g3,%lo(LL),%g3
#endif
	jmp	%g3+%g1
	ld	[%o1+0],%o4	! 1
LL:
LL00:	add	%o0,-4,%o0
	add	%o1,-4,%o1
	b	Loop00		/* 4, 8, 12, ... */
	orcc	%g0,%g0,%g2
LL01:	b	Loop01		/* 1, 5, 9, ... */
	orcc	%g0,%g0,%g2
	nop
	nop
LL10:	add	%o0,-12,%o0	/* 2, 6, 10, ... */
	add	%o1,4,%o1
	b	Loop10
	orcc	%g0,%g0,%g2
	nop
LL11:	add	%o0,-8,%o0	/* 3, 7, 11, ... */
	add	%o1,-8,%o1
	b	Loop11
	orcc	%g0,%g0,%g2

Loop:	addcc	%g3,%g2,%g3	! 1
	ld	[%o1+4],%o4	! 2
	st	%g3,[%o0+0]	! 1
	rd	%y,%g2		! 1
Loop00: umul	%o4,%o3,%g3	! 2
	addxcc	%g3,%g2,%g3	! 2
	ld	[%o1+8],%o4	! 3
	st	%g3,[%o0+4]	! 2
	rd	%y,%g2		! 2
Loop11: umul	%o4,%o3,%g3	! 3
	addxcc	%g3,%g2,%g3	! 3
	ld	[%o1+12],%o4	! 4
	add	%o1,16,%o1
	st	%g3,[%o0+8]	! 3
	rd	%y,%g2		! 3
Loop10: umul	%o4,%o3,%g3	! 4
	addxcc	%g3,%g2,%g3	! 4
	ld	[%o1+0],%o4	! 1
	st	%g3,[%o0+12]	! 4
	add	%o0,16,%o0
	rd	%y,%g2		! 4
	addx	%g0,%g2,%g2
Loop01: addcc	%o2,-4,%o2
	bg	Loop
	umul	%o4,%o3,%g3	! 1

	addcc	%g3,%g2,%g3	! 4
	st	%g3,[%o0+0]	! 4
	rd	%y,%g2		! 4

	retl
	addx	%g0,%g2,%o0


