	.text

	.align 2
	.globl holy_arch_sync_caches
holy_arch_sync_caches:
#include "cache_flush.S"
	blr
