/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_LEGACY_PARSE_HEADER
#define holy_LEGACY_PARSE_HEADER 1

#include <holy/types.h>

char *holy_legacy_parse (const char *buf, char **entryname, char **suffix);
char *holy_legacy_escape (const char *in, holy_size_t len);

/* Entered has to be holy_AUTH_MAX_PASSLEN long, zero-padded.  */
int
holy_legacy_check_md5_password (int argc, char **args,
				char *entered);

#endif
