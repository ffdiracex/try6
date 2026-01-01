/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_PROCFS_HEADER
#define holy_PROCFS_HEADER	1

#include <holy/list.h>
#include <holy/types.h>

struct holy_procfs_entry
{
  struct holy_procfs_entry *next;
  struct holy_procfs_entry **prev;

  const char *name;
  char * (*get_contents) (holy_size_t *sz);
};

extern struct holy_procfs_entry *holy_procfs_entries;

static inline void
holy_procfs_register (const char *name __attribute__ ((unused)),
		      struct holy_procfs_entry *entry)
{
  holy_list_push (holy_AS_LIST_P (&holy_procfs_entries),
		  holy_AS_LIST (entry));
}

static inline void
holy_procfs_unregister (struct holy_procfs_entry *entry)
{
  holy_list_remove (holy_AS_LIST (entry));
}


#endif
