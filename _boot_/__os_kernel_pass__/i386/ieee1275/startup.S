#include <holy/symbol.h>
#include <holy/offsets.h>
#include <holy/cpu/linux.h>
#include <multiboot.h>
#include <multiboot2.h>

/*
 * Note: holy is compiled with the options -mrtd and -mregparm=3.
 *       So the first three arguments are passed in %eax, %edx, and %ecx,
 *       respectively, and if a function has a fixed number of arguments
 *       and the number if greater than three, the function must return
 *       with "ret $N" where N is ((the number of arguments) - 3) * 4.
 */

	.file	"startup.S"
	.text
	.globl	start, _start

start:
_start:
	movl %eax, EXT_C(holy_ieee1275_entry_fn)
	jmp EXT_C(holy_main)

