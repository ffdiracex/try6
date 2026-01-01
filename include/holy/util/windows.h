/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_WINDOWS_UTIL_HEADER
#define holy_WINDOWS_UTIL_HEADER	1

#include <windows.h>

LPTSTR
holy_util_get_windows_path (const char *unix_path);

char *
holy_util_tchar_to_utf8 (LPCTSTR in);

TCHAR *
holy_get_mount_point (const TCHAR *path);

#endif
