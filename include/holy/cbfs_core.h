/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef _CBFS_CORE_H_
#define _CBFS_CORE_H_

#include <holy/types.h>

/** These are standard values for the known compression
    alogrithms that coreboot knows about for stages and
    payloads.  Of course, other CBFS users can use whatever
    values they want, as long as they understand them. */

#define CBFS_COMPRESS_NONE  0
#define CBFS_COMPRESS_LZMA  1

/** These are standard component types for well known
    components (i.e - those that coreboot needs to consume.
    Users are welcome to use any other value for their
    components */

#define CBFS_TYPE_STAGE      0x10
#define CBFS_TYPE_PAYLOAD    0x20
#define CBFS_TYPE_OPTIONROM  0x30
#define CBFS_TYPE_BOOTSPLASH 0x40
#define CBFS_TYPE_RAW        0x50
#define CBFS_TYPE_VSA        0x51
#define CBFS_TYPE_MBI        0x52
#define CBFS_TYPE_MICROCODE  0x53
#define CBFS_COMPONENT_CMOS_DEFAULT 0xaa
#define CBFS_COMPONENT_CMOS_LAYOUT 0x01aa

#define CBFS_HEADER_MAGIC  0x4F524243
#define CBFS_HEADER_VERSION1 0x31313131
#define CBFS_HEADER_VERSION2 0x31313132
#define CBFS_HEADER_VERSION  CBFS_HEADER_VERSION2

#define CBFS_HEADER_INVALID_ADDRESS	((void*)(0xffffffff))

/** this is the master cbfs header - it need to be located somewhere available
    to bootblock (to load romstage).  Where it actually lives is up to coreboot.
    On x86, a pointer to this header will live at 0xFFFFFFFC.
    For other platforms, you need to define CONFIG_CBFS_HEADER_ROM_OFFSET */

struct cbfs_header {
	holy_uint32_t magic;
	holy_uint32_t version;
	holy_uint32_t romsize;
	holy_uint32_t bootblocksize;
	holy_uint32_t align;
	holy_uint32_t offset;
	holy_uint32_t architecture;
	holy_uint32_t pad[1];
} holy_PACKED;

/* "Unknown" refers to CBFS headers version 1,
 * before the architecture was defined (i.e., x86 only).
 */
#define CBFS_ARCHITECTURE_UNKNOWN  0xFFFFFFFF
#define CBFS_ARCHITECTURE_X86      0x00000001
#define CBFS_ARCHITECTURE_ARMV7    0x00000010

/** This is a component header - every entry in the CBFS
    will have this header.

    This is how the component is arranged in the ROM:

    --------------   <- 0
    component header
    --------------   <- sizeof(struct component)
    component name
    --------------   <- offset
    data
    ...
    --------------   <- offset + len
*/

#define CBFS_FILE_MAGIC "LARCHIVE"

struct cbfs_file {
	char magic[8];
	holy_uint32_t len;
	holy_uint32_t type;
	holy_uint32_t checksum;
	holy_uint32_t offset;
} holy_PACKED;

/*** Component sub-headers ***/

/* Following are component sub-headers for the "standard"
   component types */

/** This is the sub-header for stage components.  Stages are
    loaded by coreboot during the normal boot process */

struct cbfs_stage {
	holy_uint32_t compression;  /** Compression type */
	holy_uint64_t entry;  /** entry point */
	holy_uint64_t load;   /** Where to load in memory */
	holy_uint32_t len;          /** length of data to load */
	holy_uint32_t memlen;	   /** total length of object in memory */
} holy_PACKED;

/** this is the sub-header for payload components.  Payloads
    are loaded by coreboot at the end of the boot process */

struct cbfs_payload_segment {
	holy_uint32_t type;
	holy_uint32_t compression;
	holy_uint32_t offset;
	holy_uint64_t load_addr;
	holy_uint32_t len;
	holy_uint32_t mem_len;
} holy_PACKED;

struct cbfs_payload {
	struct cbfs_payload_segment segments;
};

#define PAYLOAD_SEGMENT_CODE   0x45444F43
#define PAYLOAD_SEGMENT_DATA   0x41544144
#define PAYLOAD_SEGMENT_BSS    0x20535342
#define PAYLOAD_SEGMENT_PARAMS 0x41524150
#define PAYLOAD_SEGMENT_ENTRY  0x52544E45

struct cbfs_optionrom {
	holy_uint32_t compression;
	holy_uint32_t len;
} holy_PACKED;

#endif
