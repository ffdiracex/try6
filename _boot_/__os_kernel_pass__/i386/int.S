FUNCTION(holy_bios_interrupt)
	pushf
	cli
	popf
	pushl    %ebp
	pushl    %ecx
	pushl    %eax
	pushl    %ebx
	pushl    %esi
	pushl    %edi	
	pushl    %edx
	
	movb     %al, intno
	movl	 (%edx), %eax
	movl	 %eax, LOCAL(bios_register_eax)
	movw	 4(%edx), %ax
	movw	 %ax, LOCAL(bios_register_es)
	movw	 6(%edx), %ax
	movw	 %ax, LOCAL(bios_register_ds)
	movw	 8(%edx), %ax
	movw	 %ax, LOCAL(bios_register_flags)

	movl 	12(%edx), %ebx
	movl 	16(%edx), %ecx
	movl 	20(%edx), %edi
	movl 	24(%edx), %esi
	movl 	28(%edx), %edx

	/*
	Via C3 CPUs have cache coherence problems, so we need to call
	wbinvd at these 2 points. As wbinvd slows down boot, don't do
	it on non-VIA. 9090 is nop nop. */
VARIABLE(holy_bios_via_workaround1)
	.byte 0x90, 0x90

	PROT_TO_REAL
	.code16
	pushf
	cli
	
	mov	%ds, %ax
	push	%ax

	/* movw imm16, %ax*/
	.byte	0xb8
LOCAL(bios_register_es):
	.short 	0
	movw	%ax, %es
	/* movw imm16, %ax*/
	.byte	0xb8
LOCAL(bios_register_ds):
	.short 	0
	movw	%ax, %ds

	/* movw imm16, %ax*/
	.byte	0xb8
LOCAL(bios_register_flags):
	.short 	0
	push	%ax
	popf

	/* movl imm32, %eax*/
	.byte	0x66, 0xb8
LOCAL(bios_register_eax):
	.long 	0
	
	/* int imm8.  */
	.byte   0xcd
intno:	
	.byte   0

	movl 	%eax, %cs:LOCAL(bios_register_eax)
	movw	%ds, %ax
	movw 	%ax, %cs:LOCAL(bios_register_ds)
	pop 	%ax
	mov	%ax, %ds
	pushf
	pop	%ax
	movw	%ax, LOCAL(bios_register_flags)
	mov 	%es, %ax
	movw	%ax, LOCAL(bios_register_es)

	popf

VARIABLE(holy_bios_via_workaround2)
	.byte 0x90, 0x90

	REAL_TO_PROT
	.code32

	popl    %eax

	movl 	%ebx, 12(%eax)
	movl 	%ecx, 16(%eax)
	movl 	%edi, 20(%eax)
	movl 	%esi, 24(%eax)
	movl 	%edx, 28(%eax)

	movl     %eax, %edx

	movl	 LOCAL(bios_register_eax), %eax
	movl	 %eax, (%edx)
	movw	 LOCAL(bios_register_es), %ax
	movw	 %ax, 4(%edx)
	movw	 LOCAL(bios_register_ds), %ax
	movw	 %ax, 6(%edx)
	movw	 LOCAL(bios_register_flags), %ax
	movw	 %ax, 8(%edx)

	popl 	%edi
	popl 	%esi
	popl    %ebx
	popl    %eax
	popl    %ecx
	popl    %ebp
	ret
