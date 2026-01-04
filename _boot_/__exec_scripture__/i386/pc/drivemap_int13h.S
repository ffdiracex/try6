#include <holy/symbol.h>

#define INT13H_OFFSET(x) ((x) - LOCAL (base))

.code16

/* Copy starts here.  When deployed, this code must be segment-aligned.  */

/* The replacement int13 handler.   Preserve all registers.  */
FUNCTION(holy_drivemap_handler)
LOCAL (base):
	/* Save %dx for future restore. */
	push	%dx
	/* Push flags. Used to simulate interrupt with original flags. */
	pushf

	/* Map the drive number (always in DL).  */
	push	%ax
	push	%bx
#ifdef __APPLE__
	LOCAL(mapstart_offset) = INT13H_OFFSET(LOCAL (mapstart)) 
	movw	$LOCAL(mapstart_offset), %bx
#else
	movw	$INT13H_OFFSET(LOCAL (mapstart)), %bx
#endif

more_remaining:
	movw	%cs:(%bx), %ax
	cmpb	%ah, %al
	jz	not_found /* DRV=DST => map end - drive not remapped, keep DL.  */
	inc	%bx
	inc	%bx
	cmpb	%dl, %al
	jnz	more_remaining /* Not found, but more remaining, loop.  */
	movb	%ah, %dl /* Found - drive remapped, modify DL.  */

not_found:
	pop	%bx
	pop	%ax

	/* If the call isn't ah=0x8 or ah=0x15 we must restore %dx.  */
	cmpb	$0x8, %ah
	jz	norestore
	cmpb	$0x15, %ah
	jz	norestore

	/* Restore flags.  */
	popf
	pushf

#ifdef __APPLE__
	LOCAL(oldhandler_offset) = INT13H_OFFSET (LOCAL (oldhandler))
	lcall *%cs:LOCAL(oldhandler_offset)
#else
	lcall *%cs:INT13H_OFFSET (LOCAL (oldhandler))
#endif

	push	%bp
	mov	%sp, %bp

tail:
	/* Save new flags below %esp so the caller will recieve new flags.  */
	pushf
	pop	%dx
	mov	%dx, 8(%bp)

	pop	%bp

	/* Restore %dx.  */
	pop	%dx
	iret

norestore:

	/* Restore flags.  */
	popf
	pushf

#ifdef __APPLE__
	lcall *%cs:LOCAL(oldhandler_offset)
#else
	lcall *%cs:INT13H_OFFSET (LOCAL (oldhandler))
#endif

	push	%bp
	mov	%sp, %bp

	/* Save %dx. So it won't be restored to original value.  */
	mov	%dx, 2(%bp)

	jmp tail

/* Far pointer to the old handler.  Stored as a CS:IP in the style of real-mode
   IVT entries (thus PI:SC in mem).  */
VARIABLE(holy_drivemap_oldhandler)
LOCAL (oldhandler):
	.word 0x0, 0x0

/* This label MUST be at the end of the copied block, since the installer code
   reserves additional space for mappings at runtime and copies them over it.  */
	.align 2
	
VARIABLE(holy_drivemap_mapstart)
LOCAL (mapstart):
	.byte 0
