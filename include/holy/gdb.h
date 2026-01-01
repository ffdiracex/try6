/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_GDB_HEADER
#define holy_GDB_HEADER		1

#define SIGFPE		8
#define SIGTRAP		5
#define SIGABRT		6
#define SIGSEGV		11
#define SIGILL		4
#define SIGUSR1		30
/* We probably don't need other ones.  */

struct holy_serial_port;

extern struct holy_serial_port *holy_gdb_port;

void holy_gdb_breakpoint (void);
unsigned int holy_gdb_trap2sig (int);

#endif /* ! holy_GDB_HEADER */

