/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_BACKTRACE_HEADER
#define holy_BACKTRACE_HEADER	1

void holy_backtrace (void);
void holy_backtrace_pointer (void *ptr);
void holy_backtrace_print_address (void *addr);

#endif
