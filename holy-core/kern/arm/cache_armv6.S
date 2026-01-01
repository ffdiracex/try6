#include <holy/symbol.h>

	.file	"cache_armv6.S"
	.text
	.syntax	unified
	.arm

# define DMB	mcr	p15, 0, r0, c7, c10, 5
# define DSB	mcr	p15, 0, r0, c7, c10, 4
# define ISB	mcr	p15, 0, r0, c7, c5, 4
#define ARMV6 1

clean_invalidate_dcache:
	mcr	p15, 0, r0, c7, c14, 0	@ Clean/Invalidate D-cache
	bx	lr

#include "cache.S"

FUNCTION(holy_arm_main_id)
	mrc 	p15, 0, r0, c0, c0, 0
	bx lr

FUNCTION(holy_arm_cache_type)
	mrc 	p15, 0, r0, c0, c0, 1
	bx lr

FUNCTION(holy_arm_clear_mmu_v6)
	mov	r0, #0
	mcr	p15, 0, r0, c2, c0, 2
	bx	lr

FUNCTION(holy_arm_enable_mmu)
	mcr	p15, 0, r0, c2, c0, 0

	mvn	r0, #0
	mcr	p15, 0, r0, c3, c0, 0

	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(1 << 23)
	mcr	p15, 0, r0, c1, c0, 0

	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #(1 << 0)
	mcr	p15, 0, r0, c1, c0, 0

	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #(1 << 2)
	mcr	p15, 0, r0, c1, c0, 0

	mrc	p15, 0, r0, c1, c0, 0
	orr	r0, r0, #(1 << 12)
	mcr	p15, 0, r0, c1, c0, 0

	bx	lr
