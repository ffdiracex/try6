/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define holy_TARGET_WORDSIZE 32
#define XX		32
#define holy_le_to_cpu_addr holy_le_to_cpu32
#define ehdrXX ehdr32
#define holy_xen_get_infoXX holy_xen_get_info32
#define FOR_ELF_PHDRS FOR_ELF32_PHDRS
#include "xen_fileXX.c"
