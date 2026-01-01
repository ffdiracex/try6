/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MULTIBOOT_LOADER_HEADER
#define holy_MULTIBOOT_LOADER_HEADER 1

/* Provided by the core ("rescue mode").  */
void holy_rescue_cmd_multiboot_loader (int argc, char *argv[]);
void holy_rescue_cmd_module_loader (int argc, char *argv[]);

#endif /* ! holy_MULTIBOOT_LOADER_HEADER */
