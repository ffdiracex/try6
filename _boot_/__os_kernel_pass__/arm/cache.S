#include <holy/symbol.h>

	.file	"cache.S"
	.text
	.syntax	unified
#if !defined (__thumb2__) || !defined (ARMV7)
	.arm
#else
	.thumb
#endif

#if !defined (ARMV6) && !defined (ARMV7)
# error Unsupported architecture version!
#endif

	.align	2

/*
 * Simple cache maintenance functions
 */

@ r0 - *beg (inclusive)
@ r1 - *end (exclusive)
@void holy_arm_clean_dcache_range (holy_addr_t start, holy_addr_t end, holy_addr_t dlinesz)
#ifdef ARMV6
FUNCTION(holy_arm_clean_dcache_range_armv6)
#else
FUNCTION(holy_arm_clean_dcache_range_armv7)
#endif
	DSB
	@ Clean data cache for range to point-of-unification
1:	cmp	r0, r1
	bge	2f
#ifdef ARMV6
	mcr	p15, 0, r0, c7, c10, 1	@ Clean data cache line by MVA
#else
	mcr	p15, 0, r0, c7, c11, 1	@ DCCMVAU
#endif
	add	r0, r0, r2		@ Next line
	b	1b
2:	DSB
	bx	lr

@ r0 - *beg (inclusive)
@ r1 - *end (exclusive)
#ifdef ARMV6
FUNCTION(holy_arm_invalidate_icache_range_armv6)
#else
FUNCTION(holy_arm_invalidate_icache_range_armv7)
#endif
	@ Invalidate instruction cache for range to point-of-unification
1:	cmp	r0, r1
	bge	2f
	mcr	p15, 0, r0, c7, c5, 1	@ ICIMVAU
	add	r0, r0, r2		@ Next line
	b	1b
	@ Branch predictor invalidate all
2:	mcr	p15, 0, r0, c7,	c5, 6	@ BPIALL
	DSB
	ISB
	bx	lr

#ifdef ARMV6
FUNCTION(holy_arm_disable_caches_mmu_armv6)
#else
FUNCTION(holy_arm_disable_caches_mmu_armv7)
#endif

	push	{r4, lr}

	@ disable D-cache
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(1 << 2)
	mcr	p15, 0, r0, c1, c0, 0
	DSB
	ISB

	@ clean/invalidate D-cache
	bl	clean_invalidate_dcache

	@ disable I-cache
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(1 << 12)
	mcr	p15, 0, r0, c1, c0, 0
	DSB
	ISB

	@ invalidate I-cache (also invalidates branch predictors)
	mcr	p15, 0, r0, c7, c5, 0
	DSB
	ISB

	@ clear SCTLR M bit
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(1 << 0)
	mcr	p15, 0, r0, c1, c0, 0

	mcr	p15, 0, r0, c8, c7, 0	@ invalidate TLB
	mcr	p15, 0, r0, c7, c5, 6	@ invalidate branch predictor
	DSB
	ISB

	pop	{r4, lr}
	bx	lr

