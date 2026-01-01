/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_DEVICE_HEADER
#define holy_DEVICE_HEADER	1

#include <holy/symbol.h>
#include <holy/err.h>

struct holy_disk;
struct holy_net;

struct holy_device
{
  struct holy_disk *disk;
  struct holy_net *net;
};
typedef struct holy_device *holy_device_t;

typedef int (*holy_device_iterate_hook_t) (const char *name, void *data);

holy_device_t EXPORT_FUNC(holy_device_open) (const char *name);
holy_err_t EXPORT_FUNC(holy_device_close) (holy_device_t device);
int EXPORT_FUNC(holy_device_iterate) (holy_device_iterate_hook_t hook,
				      void *hook_data);

#endif /* ! holy_DEVICE_HEADER */
