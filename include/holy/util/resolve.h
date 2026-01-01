/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_UTIL_RESOLVE_HEADER
#define holy_UTIL_RESOLVE_HEADER	1

struct holy_util_path_list
{
  const char *name;
  struct holy_util_path_list *next;
};

/* Resolve the dependencies of the modules MODULES using the information
   in the file DEP_LIST_FILE. The directory PREFIX is used to find files.  */
struct holy_util_path_list *
holy_util_resolve_dependencies (const char *prefix,
				const char *dep_list_file,
				char *modules[]);
void holy_util_free_path_list (struct holy_util_path_list *path_list);

#endif /* ! holy_UTIL_RESOLVE_HEADER */
