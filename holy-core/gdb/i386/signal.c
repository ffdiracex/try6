/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/machine/memory.h>
#include <holy/misc.h>
#include <holy/cpu/gdb.h>
#include <holy/gdb.h>

/* Converting CPU trap number to UNIX signal number as
   described in System V ABI i386 Processor Supplement, 3-25.  */
unsigned int
holy_gdb_trap2sig (int trap_no)
{
  const int signals[] = {
    SIGFPE,			/* 0:   Divide error fault              */
    SIGTRAP,			/* 1:   Single step trap fault          */
    SIGABRT,			/* 2:   # Nonmaskable interrupt         */
    SIGTRAP,			/* 3:   Breakpoint trap                 */
    SIGSEGV,			/* 4:   Overflow trap                   */
    SIGSEGV,			/* 5:   Bounds check fault              */
    SIGILL,			/* 6:   Invalid opcode fault            */
    SIGFPE,			/* 7:   No coprocessor fault            */
    SIGABRT,			/* 8:   # Double fault abort            */
    SIGSEGV,			/* 9:   Coprocessor overrun abort       */
    SIGSEGV,			/* 10:  Invalid TSS fault               */
    SIGSEGV,			/* 11:  Segment not present fault       */
    SIGSEGV,			/* 12:  Stack exception fault           */
    SIGSEGV,			/* 13:  General protection fault abort  */
    SIGSEGV,			/* 14:  Page fault                      */
    SIGABRT,			/* 15:  (reserved)                      */
    SIGFPE,			/* 16:  Coprocessor error fault         */
    SIGUSR1			/* other                                */
  };

  return signals[trap_no < 17 ? trap_no : 17];
}

