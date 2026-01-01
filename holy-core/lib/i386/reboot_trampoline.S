#include <holy/symbol.h>
#include <holy/i386/reboot.h>

	.p2align 4

VARIABLE(holy_reboot_start)
	.code16
	
	/* set 0x472 to 0x0000 for cold boot (0x1234 for warm boot) */
	movw	$0x0472, %di
	xorw	%ax, %ax
	movw	%ax, (%di)
	ljmp	$0xf000, $0xfff0

	.code32
VARIABLE(holy_reboot_end)
