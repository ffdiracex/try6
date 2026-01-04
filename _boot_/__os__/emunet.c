/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#if defined (__linux__)
#include "linux/emunet.c"
#else
#include "basic/emunet.c"
#endif
