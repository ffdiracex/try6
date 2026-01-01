/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MEMORY_CPU_HEADER
#include <holy/efi/memory.h>

#if defined (__code_model_large__)
#define holy_EFI_MAX_USABLE_ADDRESS 0xffffffff
#else
#define holy_EFI_MAX_USABLE_ADDRESS 0x7fffffff
#endif

#endif /* ! holy_MEMORY_CPU_HEADER */
