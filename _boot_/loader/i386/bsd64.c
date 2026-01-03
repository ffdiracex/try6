/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define SUFFIX(x) x ## 64
#define holy_TARGET_WORDSIZE 64
#define OBJSYM 1
#include <holy/types.h>
typedef holy_uint64_t holy_freebsd_addr_t;
#include "bsdXX.c"
