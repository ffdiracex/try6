/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/macho.h>
#include <holy/machoload.h>

#define SUFFIX(x) x ## 64
typedef struct holy_macho_header64 holy_macho_header_t;
typedef struct holy_macho_segment64 holy_macho_segment_t;
typedef holy_uint64_t holy_macho_addr_t;
typedef struct holy_macho_thread64 holy_macho_thread_t;
#define offsetXX offset64
#define ncmdsXX ncmds64
#define cmdsizeXX cmdsize64
#define cmdsXX cmds64
#define endXX end64
#define uncompressedXX uncompressed64
#define compressedXX compressed64
#define uncompressed_sizeXX uncompressed_size64
#define compressed_sizeXX compressed_size64
#define XX "64"
#define holy_MACHO_MAGIC holy_MACHO_MAGIC64
#define holy_MACHO_CMD_SEGMENT holy_MACHO_CMD_SEGMENT64
#include "machoXX.c"

