/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_FILEID_HEADER
#define holy_FILEID_HEADER	1

#include <holy/elfload.h>

int
holy_file_check_netbsd32 (holy_elf_t elf);
int
holy_file_check_netbsd64 (holy_elf_t elf);

#endif
