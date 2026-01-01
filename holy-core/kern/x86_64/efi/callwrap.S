#include <config.h>
#include <holy/symbol.h>

/*
 * x86_64 uses registry to pass parameters. Unfortunately, gcc and efi use
 * different call conversion, so we need to do some conversion.
 *
 * gcc:
 *   %rdi,  %rsi,  %rdx,  %rcx, %r8, %r9, 8(%rsp), 16(%rsp), ...
 *
 * efi:
 *   %rcx,  %rdx,  %r8,  %r9,  32(%rsp), 40(%rsp), 48(%rsp), ...
 *
 */

        .file   "callwrap.S"
        .text

FUNCTION(efi_wrap_0)
	subq $40, %rsp
	call *%rdi
	addq $40, %rsp
	ret

FUNCTION(efi_wrap_1)
	subq $40, %rsp
	mov  %rsi, %rcx
	call *%rdi
	addq $40, %rsp
	ret

FUNCTION(efi_wrap_2)
	subq $40, %rsp
	mov  %rsi, %rcx
	call *%rdi
	addq $40, %rsp
	ret

FUNCTION(efi_wrap_3)
	subq $40, %rsp
	mov  %rcx, %r8
	mov  %rsi, %rcx
	call *%rdi
	addq $40, %rsp
	ret

FUNCTION(efi_wrap_4)
	subq $40, %rsp
	mov %r8, %r9
	mov %rcx, %r8
	mov %rsi, %rcx
	call *%rdi
	addq $40, %rsp
	ret

FUNCTION(efi_wrap_5)
	subq $40, %rsp
	mov %r9, 32(%rsp)
	mov %r8, %r9
	mov %rcx, %r8
	mov %rsi, %rcx
	call *%rdi
	addq $40, %rsp
	ret

FUNCTION(efi_wrap_6)
	subq $56, %rsp
	mov 56+8(%rsp), %rax
	mov %rax, 40(%rsp)
	mov %r9, 32(%rsp)
	mov %r8, %r9
	mov %rcx, %r8
	mov %rsi, %rcx
	call *%rdi
	addq $56, %rsp
	ret

FUNCTION(efi_wrap_7)
	subq $88, %rsp
	mov 88+16(%rsp), %rax
	mov %rax, 48(%rsp)
	mov 88+8(%rsp), %rax
	mov %rax, 40(%rsp)
	mov %r9, 32(%rsp)
	mov %r8, %r9
	mov %rcx, %r8
	mov %rsi, %rcx
	call *%rdi
	addq $88, %rsp
	ret

FUNCTION(efi_wrap_10)
	subq $88, %rsp
	mov 88+40(%rsp), %rax
	mov %rax, 72(%rsp)
	mov 88+32(%rsp), %rax
	mov %rax, 64(%rsp)
	mov 88+24(%rsp), %rax
	mov %rax, 56(%rsp)
	mov 88+16(%rsp), %rax
	mov %rax, 48(%rsp)
	mov 88+8(%rsp), %rax
	mov %rax, 40(%rsp)
	mov %r9, 32(%rsp)
	mov %r8, %r9
	mov %rcx, %r8
	mov %rsi, %rcx
	call *%rdi
	addq $88, %rsp
	ret
