/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EMUNET_HEADER
#define holy_EMUNET_HEADER	1

#include <holy/types.h>
#include <holy/symbol.h>

holy_ssize_t
EXPORT_FUNC(holy_emunet_send) (const void *packet, holy_size_t sz);

holy_ssize_t
EXPORT_FUNC(holy_emunet_receive) (void *packet, holy_size_t sz);

int
EXPORT_FUNC(holy_emunet_create) (holy_size_t *mtu);

void
EXPORT_FUNC(holy_emunet_close) (void);

#endif
