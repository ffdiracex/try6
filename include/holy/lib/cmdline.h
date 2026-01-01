/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_CMDLINE_HEADER
#define holy_CMDLINE_HEADER	1

#include <holy/types.h>

#define LINUX_IMAGE "BOOT_IMAGE="

unsigned int holy_loader_cmdline_size (int argc, char *argv[]);
int holy_create_loader_cmdline (int argc, char *argv[], char *buf,
				holy_size_t size);

#endif /* ! holy_CMDLINE_HEADER */
