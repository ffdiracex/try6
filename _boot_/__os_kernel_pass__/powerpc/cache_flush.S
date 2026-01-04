#undef CACHE_LINE_BYTES
#define CACHE_LINE_BYTES 32

	/* `address' may not be CACHE_LINE_BYTES-aligned.  */
	andi. 6, 3, CACHE_LINE_BYTES - 1 /* Find the misalignment.  */
	add 4, 4, 6 /* Adjust `size' to compensate.  */

	/* Force the dcache lines to memory.  */
	li 5, 0
1:	dcbst 5, 3
	addi 5, 5, CACHE_LINE_BYTES
	cmpw 5, 4
	blt 1b
	sync            /* Force all dcbsts to complete.  */

	/* Invalidate the icache lines.  */
	li 5, 0
1:	icbi 5, 3
	addi 5, 5, CACHE_LINE_BYTES
	cmpw 5, 4
	blt 1b
	sync            /* Force all icbis to complete.  */
	isync           /* Discard partially executed instructions that were
		           loaded from the invalid icache.  */
