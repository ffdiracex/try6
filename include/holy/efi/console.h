/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EFI_CONSOLE_HEADER
#define holy_EFI_CONSOLE_HEADER	1

#include <holy/types.h>
#include <holy/symbol.h>

/* Initialize the console system.  */
void holy_console_init (void);

/* Finish the console system.  */
void holy_console_fini (void);

#endif /* ! holy_EFI_CONSOLE_HEADER */
