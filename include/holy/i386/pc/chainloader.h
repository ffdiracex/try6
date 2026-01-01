/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_CHAINLOADER_MACHINE_HEADER
#define holy_CHAINLOADER_MACHINE_HEADER	1

#include <holy/dl.h>

void
holy_chainloader_patch_bpb (void *bs, holy_device_t dev, holy_uint8_t dl);

#endif /* holy_CHAINLOADER_MACHINE_HEADER */
