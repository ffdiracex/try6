! INPUT PARAMETERS
! res_ptr	o0
! s1_ptr	o1
! size		o2
! s2_limb	o3

#include "sysdep.h"

.text
	.align 4
	.global C_SYMBOL_NAME(_gcry_mpih_submul_1)
C_SYMBOL_NAME(_gcry_mpih_submul_1):
	sub	%g0,%o2,%o2		! negate ...
	sll	%o2,2,%o2		! ... and scale size
	sub	%o1,%o2,%o1		! o1 is offset s1_ptr
	sub	%o0,%o2,%g1		! g1 is offset res_ptr

	mov	0,%o0			! clear cy_limb

Loop:	ld	[%o1+%o2],%o4
	ld	[%g1+%o2],%g2
	umul	%o4,%o3,%o5
	rd	%y,%g3
	addcc	%o5,%o0,%o5
	addx	%g3,0,%o0
	subcc	%g2,%o5,%g2
	addx	%o0,0,%o0
	st	%g2,[%g1+%o2]

	addcc	%o2,4,%o2
	bne	Loop
	 nop

	retl
	 nop


