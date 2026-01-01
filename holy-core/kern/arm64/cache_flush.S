#include <holy/symbol.h>

	.file	"cache_flush.S"
	.text

/*
 * Simple cache maintenance functions
 */

// x0 - *beg (inclusive)
// x1 - *end (exclusive)
// x2 - line size
FUNCTION(holy_arch_clean_dcache_range)
	// Clean data cache for range to point-of-unification
1:	cmp	x0, x1
	b.ge	2f
	dc	cvau, x0		// Clean Virtual Address to PoU
	add	x0, x0, x2		// Next line
	b	1b
2:	dsb	ish
	isb
	ret

// x0 - *beg (inclusive)
// x1 - *end (exclusive)
// x2 - line size
FUNCTION(holy_arch_invalidate_icache_range)
	// Invalidate instruction cache for range to point-of-unification
1:	cmp	x0, x1
	b.ge	2f
	ic	ivau, x0		// Invalidate Virtual Address to PoU
	add	x0, x0, x2		// Next line
	b	1b
	// Branch predictor invalidation not needed on AArch64
2:	dsb	ish
	isb
	ret
