/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_CONSOLE_MACHINE_HEADER
#define holy_CONSOLE_MACHINE_HEADER 1

#include <holy/symbol.h>

/* Initialize the console system.  */
void holy_console_init_early (void);
void holy_console_init_lately (void);

/* Finish the console system.  */
void holy_console_fini (void);

#endif /* ! holy_CONSOLE_MACHINE_HEADER */
