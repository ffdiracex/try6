/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_CPU_CPUID_HEADER
#define holy_CPU_CPUID_HEADER 1

extern unsigned char holy_cpuid_has_longmode;
extern unsigned char holy_cpuid_has_pae;

#ifdef __x86_64__

static __inline int
holy_cpu_is_cpuid_supported (void)
{
  holy_uint64_t id_supported;

  __asm__ ("pushfq\n\t"
           "popq %%rax             /* Get EFLAGS into EAX */\n\t"
           "movq %%rax, %%rcx      /* Save original flags in ECX */\n\t"
           "xorq $0x200000, %%rax  /* Flip ID bit in EFLAGS */\n\t"
           "pushq %%rax            /* Store modified EFLAGS on stack */\n\t"
           "popfq                  /* Replace current EFLAGS */\n\t"
           "pushfq                 /* Read back the EFLAGS */\n\t"
           "popq %%rax             /* Get EFLAGS into EAX */\n\t"
           "xorq %%rcx, %%rax      /* Check if flag could be modified */\n\t"
           : "=a" (id_supported)
           : /* No inputs.  */
           : /* Clobbered:  */ "%rcx");

  return id_supported != 0;
}

#else

static __inline int
holy_cpu_is_cpuid_supported (void)
{
  holy_uint32_t id_supported;

  __asm__ ("pushfl\n\t"
           "popl %%eax             /* Get EFLAGS into EAX */\n\t"
           "movl %%eax, %%ecx      /* Save original flags in ECX */\n\t"
           "xorl $0x200000, %%eax  /* Flip ID bit in EFLAGS */\n\t"
           "pushl %%eax            /* Store modified EFLAGS on stack */\n\t"
           "popfl                  /* Replace current EFLAGS */\n\t"
           "pushfl                 /* Read back the EFLAGS */\n\t"
           "popl %%eax             /* Get EFLAGS into EAX */\n\t"
           "xorl %%ecx, %%eax      /* Check if flag could be modified */\n\t"
           : "=a" (id_supported)
           : /* No inputs.  */
           : /* Clobbered:  */ "%rcx");

  return id_supported != 0;
}

#endif

#ifdef __PIC__
#define holy_cpuid(num,a,b,c,d) \
  asm volatile ("xchgl %%ebx, %1; cpuid; xchgl %%ebx, %1" \
                : "=a" (a), "=r" (b), "=c" (c), "=d" (d)  \
                : "0" (num))
#else
#define holy_cpuid(num,a,b,c,d) \
  asm volatile ("cpuid" \
                : "=a" (a), "=b" (b), "=c" (c), "=d" (d)  \
                : "0" (num))
#endif

#endif
