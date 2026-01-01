/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define SUFFIX(x) x ## 32
#define holy_TARGET_WORDSIZE 32
#define OBJSYM 0
#include <holy/types.h>
typedef holy_uint32_t holy_freebsd_addr_t;
#include "bsdXX.c"
