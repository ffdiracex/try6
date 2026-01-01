#include <holy/symbol.h>
#include <holy/dl.h>

	.file	"setjmp.S"

holy_MOD_LICENSE "GPLv2+"

	/* The following two entry points are the traditional entry points: */

	.text
	
	.proc EXT_C(holy_setjmp)
FUNCTION(holy_setjmp)
	alloc r8=ar.pfs,2,0,0,0
	mov in1=1
	br.cond.sptk.many __sigsetjmp
	.endp EXT_C(holy_setjmp)

	/* __sigsetjmp(__jmp_buf buf, int savemask) */

	.proc __sigsetjmp
__sigsetjmp:
	//.prologue ASM_UNW_PRLG_RP|ASM_UNW_PRLG_PFS, ASM_UNW_PRLG_GRSAVE(2)
	alloc loc1=ar.pfs,2,2,2,0
	mov r16=ar.unat
	;;
	mov r17=ar.fpsr
	mov r2=in0
	add r3=8,in0
	;;
	st8.spill.nta [r2]=sp,16	// r12 (sp)
	st8.spill.nta [r3]=gp,16	// r1 (gp)
	;;
	st8.nta [r2]=r16,16		// save caller's unat
	st8.nta [r3]=r17,16		// save fpsr
	add r8=0xa0,in0
	;;
	st8.spill.nta [r2]=r4,16	// r4
	st8.spill.nta [r3]=r5,16	// r5
	add r9=0xb0,in0
	;;
	stf.spill.nta [r8]=f2,32
	stf.spill.nta [r9]=f3,32
	mov loc0=rp
	.body
	;;
	stf.spill.nta [r8]=f4,32
	stf.spill.nta [r9]=f5,32
	mov r17=b1
	;;
	stf.spill.nta [r8]=f16,32
	stf.spill.nta [r9]=f17,32
	mov r18=b2
	;;
	stf.spill.nta [r8]=f18,32
	stf.spill.nta [r9]=f19,32
	mov r19=b3
	;;
	stf.spill.nta [r8]=f20,32
	stf.spill.nta [r9]=f21,32
	mov r20=b4
	;;
	stf.spill.nta [r8]=f22,32
	stf.spill.nta [r9]=f23,32
	mov r21=b5
	;;
	stf.spill.nta [r8]=f24,32
	stf.spill.nta [r9]=f25,32
	mov r22=ar.lc
	;;
	stf.spill.nta [r8]=f26,32
	stf.spill.nta [r9]=f27,32
	mov r24=pr
	;;
	stf.spill.nta [r8]=f28,32
	stf.spill.nta [r9]=f29,32
	;;
	stf.spill.nta [r8]=f30
	stf.spill.nta [r9]=f31

	st8.spill.nta [r2]=r6,16	// r6
	st8.spill.nta [r3]=r7,16	// r7
	;;
	mov r23=ar.bsp
	mov r25=ar.unat
	mov out0=in0

	st8.nta [r2]=loc0,16		// b0
	st8.nta [r3]=r17,16		// b1
	mov out1=in1
	;;
	st8.nta [r2]=r18,16		// b2
	st8.nta [r3]=r19,16		// b3
	;;
	st8.nta [r2]=r20,16		// b4
	st8.nta [r3]=r21,16		// b5
	;;
	st8.nta [r2]=loc1,16		// ar.pfs
	st8.nta [r3]=r22,16		// ar.lc
	;;
	st8.nta [r2]=r24,16		// pr
	st8.nta [r3]=r23,16		// ar.bsp
	;;
	st8.nta [r2]=r25		// ar.unat
	st8.nta [r3]=in0		// &__jmp_buf
	mov r8=0
	mov rp=loc0
	mov ar.pfs=loc1
	br.ret.sptk.many rp

	.endp __sigsetjmp
