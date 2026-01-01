/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_SEARCH_HEADER
#define holy_SEARCH_HEADER 1

void holy_search_fs_file (const char *key, const char *var, int no_floppy,
			  char **hints, unsigned nhints);
void holy_search_fs_uuid (const char *key, const char *var, int no_floppy,
			  char **hints, unsigned nhints);
void holy_search_label (const char *key, const char *var, int no_floppy,
			char **hints, unsigned nhints);
void holy_search_part_uuid (const char *key, const char *var, int no_floppy,
			    char **hints, unsigned nhints);
void holy_search_part_label (const char *key, const char *var, int no_floppy,
			     char **hints, unsigned nhints);
void holy_search_disk_uuid (const char *key, const char *var, int no_floppy,
			    char **hints, unsigned nhints);

#endif
