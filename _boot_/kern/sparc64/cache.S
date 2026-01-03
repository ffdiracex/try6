#include <holy/symbol.h>

        .file   "cache.S"

        .text

/*
 * void holy_arch_sync_caches (void *address, holy_size_t len)
 */
FUNCTION(holy_arch_sync_caches)
	brz,pn		%o1, 2f
	 add		%o0, %o1, %o1
	add		%o1, 7, %o1
	andn		%o1, 7, %o1
	andn		%o0, 7, %o0
	sub		%o1, %o0, %o1
1:	subcc		%o1, 8, %o1
	bne,pt		%icc, 1b
	 flush		%o0 + %o1
2:	retl
	 nop

