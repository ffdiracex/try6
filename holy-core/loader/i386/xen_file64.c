/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define holy_TARGET_WORDSIZE 64
#define XX		64
#define holy_le_to_cpu_addr holy_le_to_cpu64
#define ehdrXX ehdr64
#define holy_xen_get_infoXX holy_xen_get_info64
#define FOR_ELF_PHDRS FOR_ELF64_PHDRS
#include "xen_fileXX.c"
