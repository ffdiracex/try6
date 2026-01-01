/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_UDF_H
#define holy_UDF_H	1

#include <holy/types.h>

#ifdef holy_UTIL
#include <holy/disk.h>

holy_disk_addr_t
holy_udf_get_cluster_sector (holy_disk_t disk, holy_uint64_t *sec_per_lcn);
#endif
#endif
