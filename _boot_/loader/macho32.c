/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/macho.h>
#include <holy/machoload.h>

#define SUFFIX(x) x ## 32
typedef struct holy_macho_header32 holy_macho_header_t;
typedef struct holy_macho_segment32 holy_macho_segment_t;
typedef holy_uint32_t holy_macho_addr_t;
typedef struct holy_macho_thread32 holy_macho_thread_t;
#define offsetXX offset32
#define ncmdsXX ncmds32
#define cmdsizeXX cmdsize32
#define cmdsXX cmds32
#define endXX end32
#define uncompressedXX uncompressed32
#define compressedXX compressed32
#define uncompressed_sizeXX uncompressed_size32
#define compressed_sizeXX compressed_size32
#define XX "32"
#define holy_MACHO_MAGIC holy_MACHO_MAGIC32
#define holy_MACHO_CMD_SEGMENT holy_MACHO_CMD_SEGMENT32
#include "machoXX.c"

