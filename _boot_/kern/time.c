/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/time.h>

typedef holy_uint64_t (*get_time_ms_func_t) (void);

/* Function pointer to the implementation in use.  */
static get_time_ms_func_t get_time_ms_func;

holy_uint64_t
holy_get_time_ms (void)
{
  return get_time_ms_func ();
}

void
holy_install_get_time_ms (get_time_ms_func_t func)
{
  get_time_ms_func = func;
}
