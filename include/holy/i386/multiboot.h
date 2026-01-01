/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MULTIBOOT_CPU_HEADER
#define holy_MULTIBOOT_CPU_HEADER	1

#define MULTIBOOT_INITIAL_STATE  { .eax = MULTIBOOT_BOOTLOADER_MAGIC,	\
    .ecx = 0,								\
    .edx = 0,								\
    /* Set esp to some random location in low memory to avoid breaking */ \
    /* non-compliant kernels.  */					\
    .esp = 0x7ff00							\
      }
#define MULTIBOOT_ENTRY_REGISTER eip
#define MULTIBOOT_MBI_REGISTER ebx
#define MULTIBOOT_ARCHITECTURE_CURRENT MULTIBOOT_ARCHITECTURE_I386

#ifdef holy_MACHINE_EFI
#ifdef __x86_64__
#define MULTIBOOT_EFI_INITIAL_STATE  { .rax = MULTIBOOT_BOOTLOADER_MAGIC,	\
    .rcx = 0,									\
    .rdx = 0,									\
      }
#define MULTIBOOT_EFI_ENTRY_REGISTER rip
#define MULTIBOOT_EFI_MBI_REGISTER rbx
#endif
#endif

#define MULTIBOOT_ELF32_MACHINE EM_386
#define MULTIBOOT_ELF64_MACHINE EM_X86_64

#endif /* ! holy_MULTIBOOT_CPU_HEADER */
