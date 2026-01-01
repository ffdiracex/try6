#include "sysdep.h"
#include "asm-syntax.h"


#ifndef USE_PPC_PATCHES



	.toc
	.csect ._gcry_mpih_addmul_1[PR]
	.align 2
	.globl _gcry_mpih_addmul_1
	.globl ._gcry_mpih_addmul_1
	.csect _gcry_mpih_addmul_1[DS]
_gcry_mpih_addmul_1:
	.long ._gcry_mpih_addmul_1[PR], TOC[tc0], 0
	.csect ._gcry_mpih_addmul_1[PR]
._gcry_mpih_addmul_1:
	mtctr	5

	lwz	0,0(4)
	mullw	7,0,6
	mulhwu	10,0,6
	lwz	9,0(3)
	addc	8,7,9
	addi	3,3,-4
	bdz	Lend

Loop:	lwzu	0,4(4)
	stwu	8,4(3)
	mullw	8,0,6
	adde	7,8,10
	mulhwu	10,0,6
	lwz	9,4(3)
	addze	10,10
	addc	8,7,9
	bdnz	Loop

Lend:	stw	8,4(3)
	addze	3,10
	blr

#else


/* mp_limb_t mpn_addmul_1 (mp_ptr res_ptr, mp_srcptr s1_ptr,
			   mp_size_t s1_size, mp_limb_t s2_limb)
   Calculate res+s1*s2 and put result back in res; return carry.  */
ENTRY(_gcry_mpih_addmul_1)
       mtctr   %r5

       lwz     %r0,0(%r4)
       mullw   %r7,%r0,%r6
       mulhwu  %r10,%r0,%r6
       lwz     %r9,0(%r3)
       addc    %r8,%r7,%r9
       addi    %r3,%r3,-4	       /* adjust res_ptr */
       bdz     1f

0:     lwzu    %r0,4(%r4)
       stwu    %r8,4(%r3)
       mullw   %r8,%r0,%r6
       adde    %r7,%r8,%r10
       mulhwu  %r10,%r0,%r6
       lwz     %r9,4(%r3)
       addze   %r10,%r10
       addc    %r8,%r7,%r9
       bdnz    0b

1:     stw     %r8,4(%r3)
       addze   %r3,%r10
       blr
END(_gcry_mpih_addmul_1)
#endif
