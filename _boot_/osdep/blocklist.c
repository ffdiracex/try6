/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifdef __linux__
#include "linux/blocklist.c"
#elif defined (__MINGW32__) || defined (__CYGWIN__)
#include "windows/blocklist.c"
#else
#include "generic/blocklist.c"
#endif
