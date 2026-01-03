/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/command.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/term.h>
#include <holy/backtrace.h>

#define MAX_STACK_FRAME 102400

void
holy_backtrace_pointer (void *ebp)
{
  void *ptr, *nptr;
  unsigned i;

  ptr = ebp;
  while (1)
    {
      holy_printf ("%p: ", ptr);
      holy_backtrace_print_address (((void **) ptr)[1]);
      holy_printf (" (");
      for (i = 0; i < 2; i++)
	holy_printf ("%p,", ((void **)ptr) [i + 2]);
      holy_printf ("%p)\n", ((void **)ptr) [i + 2]);
      nptr = *(void **)ptr;
      if (nptr < ptr || (void **) nptr - (void **) ptr > MAX_STACK_FRAME
	  || nptr == ptr)
	{
	  holy_printf ("Invalid stack frame at %p (%p)\n", ptr, nptr);
	  break;
	}
      ptr = nptr;
    }
}

void
holy_backtrace (void)
{
#ifdef __x86_64__
  asm volatile ("movq %%rbp, %%rdi\n"
		"callq *%%rax": :"a"(holy_backtrace_pointer));
#else
  asm volatile ("movl %%ebp, %%eax\n"
		"calll *%%ecx": :"c"(holy_backtrace_pointer));
#endif
}

