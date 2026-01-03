#include <holy/symbol.h>
#include <holy/dl.h>

	.file	"misc.S"
	.text
	.syntax	unified
#if !defined (__thumb2__)
	.arm
#else
	.thumb
#endif

	.align	2

FUNCTION(__muldi3)
FUNCTION(__aeabi_lmul)
        stmfd   sp!, {r4, fp}
        add     fp, sp, #4
        sub     sp, sp, #16
        str     r0, [fp, #-12]
        str     r1, [fp, #-8]
        str     r2, [fp, #-20]
        str     r3, [fp, #-16]
        ldr     r3, [fp, #-8]
        ldr     r2, [fp, #-20]
        mul     r2, r3, r2
        ldr     r3, [fp, #-16]
        ldr     r1, [fp, #-12]
        mul     r3, r1, r3
        add     r2, r2, r3
        ldr     r0, [fp, #-12]
        ldr     r1, [fp, #-20]
        umull   r3, r4, r0, r1
        add     r2, r2, r4
        mov     r4, r2
        mov     r0, r3
        mov     r1, r4
	mov     sp, fp
	sub     sp, sp, #4
        ldmfd   sp!, {r4, fp}
        bx      lr

	.macro division32 parent

        sub     sp, sp, #8      @ Allocate naturally aligned 64-bit space
        stmfd   sp!, {r3,lr}    @ Dummy r3 to maintain stack alignment
        add     r2, sp, #8      @ Set r2 to address of 64-bit space
        bl      \parent
        ldr     r1, [sp, #8]    @ Extract remainder
        ldmfd   sp!, {r3,lr}    @ Pop into an unused arg/scratch register
        add     sp, sp, #8
        bx      lr
	.endm

FUNCTION(__aeabi_uidivmod)
	division32 holy_divmod32
FUNCTION(__aeabi_idivmod)
	division32 holy_divmod32s

/*
 * Null divide-by-zero handler
 */
FUNCTION(__aeabi_unwind_cpp_pr0)
FUNCTION(raise)
	mov	r0, #0
	bx	lr
	
	END
