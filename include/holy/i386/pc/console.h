/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_CONSOLE_MACHINE_HEADER
#define holy_CONSOLE_MACHINE_HEADER	1

#ifndef ASM_FILE

#include <holy/types.h>
#include <holy/symbol.h>
#include <holy/term.h>

/* Initialize the console system.  */
void holy_console_init (void);

/* Finish the console system.  */
void holy_console_fini (void);

#endif

#endif /* ! holy_CONSOLE_MACHINE_HEADER */
