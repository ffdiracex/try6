#include "sysdep.h"
#include "asm-syntax.h"


#ifndef USE_PPC_PATCHES


	.toc
	.csect ._gcry_mpih_mul_1[PR]
	.align 2
	.globl _gcry_mpih_mul_1
	.globl ._gcry_mpih_mul_1
	.csect _gcry_mpih_mul_1[DS]
_gcry_mpih_mul_1:
	.long ._gcry_mpih_mul_1[PR], TOC[tc0], 0
	.csect ._gcry_mpih_mul_1[PR]
._gcry_mpih_mul_1:
	mtctr	5

	lwz	0,0(4)
	mullw	7,0,6
	mulhwu	10,0,6
	addi	3,3,-4		# adjust res_ptr
	addic	5,5,0		# clear cy with dummy insn
	bdz	Lend

Loop:	lwzu	0,4(4)
	stwu	7,4(3)
	mullw	8,0,6
	adde	7,8,10
	mulhwu	10,0,6
	bdnz	Loop

Lend:	stw	7,4(3)
	addze	3,10
	blr

#else


/* mp_limb_t mpn_mul_1 (mp_ptr res_ptr, mp_srcptr s1_ptr,
			mp_size_t s1_size, mp_limb_t s2_limb)
   Calculate s1*s2 and put result in res_ptr; return carry.  */

ENTRY(_gcry_mpih_mul_1)
       mtctr   %r5

       lwz     %r0,0(%r4)
       mullw   %r7,%r0,%r6
       mulhwu  %r10,%r0,%r6
       addi    %r3,%r3,-4	       # adjust res_ptr
       addic   %r5,%r5,0	       # clear cy with dummy insn
       bdz     1f

0:     lwzu    %r0,4(%r4)
       stwu    %r7,4(%r3)
       mullw   %r8,%r0,%r6
       adde    %r7,%r8,%r10
       mulhwu  %r10,%r0,%r6
       bdnz    0b

1:     stw     %r7,4(%r3)
       addze   %r3,%r10
       blr
END(_gcry_mpih_mul_1)
#endif
