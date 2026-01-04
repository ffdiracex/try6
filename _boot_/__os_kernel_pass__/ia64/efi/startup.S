#include <config.h>
#include <holy/symbol.h>
#include <holy/offsets.h>

	.text
	.psr abi64
	.psr lsb
	.lsb

	.global _start
	.proc _start
_start:
	alloc loc0=ar.pfs,2,4,0,0
	mov loc1=rp
	addl loc2=@gprel(holy_efi_image_handle),gp
	addl loc3=@gprel(holy_efi_system_table),gp
	;;
	st8 [loc2]=in0
	st8 [loc3]=in1
	br.call.sptk.few rp=holy_main
	;;
	mov ar.pfs=loc0
	mov rp=loc1
	;;
	br.ret.sptk.few rp

	.endp _start
