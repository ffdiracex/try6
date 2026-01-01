/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EFI_API_HEADER
#define holy_EFI_API_HEADER	1

#include <holy/types.h>
#include <holy/symbol.h>

/* For consistency and safety, we name the EFI-defined types differently.
   All names are transformed into lower case, _t appended, and
   holy_efi_ prepended.  */

/* Constants.  */
#define holy_EFI_EVT_TIMER				0x80000000
#define holy_EFI_EVT_RUNTIME				0x40000000
#define holy_EFI_EVT_RUNTIME_CONTEXT			0x20000000
#define holy_EFI_EVT_NOTIFY_WAIT			0x00000100
#define holy_EFI_EVT_NOTIFY_SIGNAL			0x00000200
#define holy_EFI_EVT_SIGNAL_EXIT_BOOT_SERVICES		0x00000201
#define holy_EFI_EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE	0x60000202

#define holy_EFI_TPL_APPLICATION	4
#define holy_EFI_TPL_CALLBACK		8
#define holy_EFI_TPL_NOTIFY		16
#define holy_EFI_TPL_HIGH_LEVEL		31

#define holy_EFI_MEMORY_UC	0x0000000000000001LL
#define holy_EFI_MEMORY_WC	0x0000000000000002LL
#define holy_EFI_MEMORY_WT	0x0000000000000004LL
#define holy_EFI_MEMORY_WB	0x0000000000000008LL
#define holy_EFI_MEMORY_UCE	0x0000000000000010LL
#define holy_EFI_MEMORY_WP	0x0000000000001000LL
#define holy_EFI_MEMORY_RP	0x0000000000002000LL
#define holy_EFI_MEMORY_XP	0x0000000000004000LL
#define holy_EFI_MEMORY_NV	0x0000000000008000LL
#define holy_EFI_MEMORY_MORE_RELIABLE	0x0000000000010000LL
#define holy_EFI_MEMORY_RO	0x0000000000020000LL
#define holy_EFI_MEMORY_RUNTIME	0x8000000000000000LL

#define holy_EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL	0x00000001
#define holy_EFI_OPEN_PROTOCOL_GET_PROTOCOL		0x00000002
#define holy_EFI_OPEN_PROTOCOL_TEST_PROTOCOL		0x00000004
#define holy_EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER	0x00000008
#define holy_EFI_OPEN_PROTOCOL_BY_DRIVER		0x00000010
#define holy_EFI_OPEN_PROTOCOL_BY_EXCLUSIVE		0x00000020

#define holy_EFI_OS_INDICATIONS_BOOT_TO_FW_UI	0x0000000000000001ULL

#define holy_EFI_VARIABLE_NON_VOLATILE		0x0000000000000001
#define holy_EFI_VARIABLE_BOOTSERVICE_ACCESS	0x0000000000000002
#define holy_EFI_VARIABLE_RUNTIME_ACCESS	0x0000000000000004

#define holy_EFI_TIME_ADJUST_DAYLIGHT	0x01
#define holy_EFI_TIME_IN_DAYLIGHT	0x02

#define holy_EFI_UNSPECIFIED_TIMEZONE	0x07FF

#define holy_EFI_OPTIONAL_PTR	0x00000001

#define holy_EFI_LOADED_IMAGE_GUID	\
  { 0x5b1b31a1, 0x9562, 0x11d2, \
    { 0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define holy_EFI_DISK_IO_GUID	\
  { 0xce345171, 0xba0b, 0x11d2, \
    { 0x8e, 0x4f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define holy_EFI_BLOCK_IO_GUID	\
  { 0x964e5b21, 0x6459, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define holy_EFI_SERIAL_IO_GUID \
  { 0xbb25cf6f, 0xf1d4, 0x11d2, \
    { 0x9a, 0x0c, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0xfd } \
  }

#define holy_EFI_SIMPLE_NETWORK_GUID	\
  { 0xa19832b9, 0xac25, 0x11d3, \
    { 0x9a, 0x2d, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define holy_EFI_PXE_GUID	\
  { 0x03c4e603, 0xac28, 0x11d3, \
    { 0x9a, 0x2d, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define holy_EFI_DEVICE_PATH_GUID	\
  { 0x09576e91, 0x6d3f, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define holy_EFI_SIMPLE_TEXT_INPUT_PROTOCOL_GUID \
  { 0x387477c1, 0x69c7, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define holy_EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID \
  { 0xdd9e7534, 0x7762, 0x4698, \
    { 0x8c, 0x14, 0xf5, 0x85, 0x17, 0xa6, 0x25, 0xaa } \
  }

#define holy_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID \
  { 0x387477c2, 0x69c7, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define holy_EFI_SIMPLE_POINTER_PROTOCOL_GUID \
  { 0x31878c87, 0xb75, 0x11d5, \
    { 0x9a, 0x4f, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define holy_EFI_ABSOLUTE_POINTER_PROTOCOL_GUID \
  { 0x8D59D32B, 0xC655, 0x4AE9, \
    { 0x9B, 0x15, 0xF2, 0x59, 0x04, 0x99, 0x2A, 0x43 } \
  }

#define holy_EFI_DRIVER_BINDING_PROTOCOL_GUID \
  { 0x18A031AB, 0xB443, 0x4D1A, \
    { 0xA5, 0xC0, 0x0C, 0x09, 0x26, 0x1E, 0x9F, 0x71 } \
  }

#define holy_EFI_LOADED_IMAGE_PROTOCOL_GUID \
  { 0x5B1B31A1, 0x9562, 0x11d2, \
    { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } \
  }

#define holy_EFI_LOAD_FILE_PROTOCOL_GUID \
  { 0x56EC3091, 0x954C, 0x11d2, \
    { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } \
  }

#define holy_EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
  { 0x0964e5b22, 0x6459, 0x11d2, \
    { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } \
  }

#define holy_EFI_TAPE_IO_PROTOCOL_GUID \
  { 0x1e93e633, 0xd65a, 0x459e, \
    { 0xab, 0x84, 0x93, 0xd9, 0xec, 0x26, 0x6d, 0x18 } \
  }

#define holy_EFI_UNICODE_COLLATION_PROTOCOL_GUID \
  { 0x1d85cd7f, 0xf43d, 0x11d2, \
    { 0x9a, 0x0c, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define holy_EFI_SCSI_IO_PROTOCOL_GUID \
  { 0x932f47e6, 0x2362, 0x4002, \
    { 0x80, 0x3e, 0x3c, 0xd5, 0x4b, 0x13, 0x8f, 0x85 } \
  }

#define holy_EFI_USB2_HC_PROTOCOL_GUID \
  { 0x3e745226, 0x9818, 0x45b6, \
    { 0xa2, 0xac, 0xd7, 0xcd, 0x0e, 0x8b, 0xa2, 0xbc } \
  }

#define holy_EFI_DEBUG_SUPPORT_PROTOCOL_GUID \
  { 0x2755590C, 0x6F3C, 0x42FA, \
    { 0x9E, 0xA4, 0xA3, 0xBA, 0x54, 0x3C, 0xDA, 0x25 } \
  }

#define holy_EFI_DEBUGPORT_PROTOCOL_GUID \
  { 0xEBA4E8D2, 0x3858, 0x41EC, \
    { 0xA2, 0x81, 0x26, 0x47, 0xBA, 0x96, 0x60, 0xD0 } \
  }

#define holy_EFI_DECOMPRESS_PROTOCOL_GUID \
  { 0xd8117cfe, 0x94a6, 0x11d4, \
    { 0x9a, 0x3a, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define holy_EFI_DEVICE_PATH_TO_TEXT_PROTOCOL_GUID \
  { 0x8b843e20, 0x8132, 0x4852, \
    { 0x90, 0xcc, 0x55, 0x1a, 0x4e, 0x4a, 0x7f, 0x1c } \
  }

#define holy_EFI_DEVICE_PATH_UTILITIES_PROTOCOL_GUID \
  { 0x379be4e, 0xd706, 0x437d, \
    { 0xb0, 0x37, 0xed, 0xb8, 0x2f, 0xb7, 0x72, 0xa4 } \
  }

#define holy_EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL_GUID \
  { 0x5c99a21, 0xc70f, 0x4ad2, \
    { 0x8a, 0x5f, 0x35, 0xdf, 0x33, 0x43, 0xf5, 0x1e } \
  }

#define holy_EFI_ACPI_TABLE_PROTOCOL_GUID \
  { 0xffe06bdd, 0x6107, 0x46a6, \
    { 0x7b, 0xb2, 0x5a, 0x9c, 0x7e, 0xc5, 0x27, 0x5c} \
  }

#define holy_EFI_HII_CONFIG_ROUTING_PROTOCOL_GUID \
  { 0x587e72d7, 0xcc50, 0x4f79, \
    { 0x82, 0x09, 0xca, 0x29, 0x1f, 0xc1, 0xa1, 0x0f } \
  }

#define holy_EFI_HII_DATABASE_PROTOCOL_GUID \
  { 0xef9fc172, 0xa1b2, 0x4693, \
    { 0xb3, 0x27, 0x6d, 0x32, 0xfc, 0x41, 0x60, 0x42 } \
  }

#define holy_EFI_HII_STRING_PROTOCOL_GUID \
  { 0xfd96974, 0x23aa, 0x4cdc, \
    { 0xb9, 0xcb, 0x98, 0xd1, 0x77, 0x50, 0x32, 0x2a } \
  }

#define holy_EFI_HII_IMAGE_PROTOCOL_GUID \
  { 0x31a6406a, 0x6bdf, 0x4e46, \
    { 0xb2, 0xa2, 0xeb, 0xaa, 0x89, 0xc4, 0x9, 0x20 } \
  }

#define holy_EFI_HII_FONT_PROTOCOL_GUID \
  { 0xe9ca4775, 0x8657, 0x47fc, \
    { 0x97, 0xe7, 0x7e, 0xd6, 0x5a, 0x8, 0x43, 0x24 } \
  }

#define holy_EFI_HII_CONFIGURATION_ACCESS_PROTOCOL_GUID \
  { 0x330d4706, 0xf2a0, 0x4e4f, \
    { 0xa3, 0x69, 0xb6, 0x6f, 0xa8, 0xd5, 0x43, 0x85 } \
  }

#define holy_EFI_COMPONENT_NAME2_PROTOCOL_GUID \
  { 0x6a7a5cff, 0xe8d9, 0x4f70, \
    { 0xba, 0xda, 0x75, 0xab, 0x30, 0x25, 0xce, 0x14} \
  }

#define holy_EFI_USB_IO_PROTOCOL_GUID \
  { 0x2B2F68D6, 0x0CD2, 0x44cf, \
    { 0x8E, 0x8B, 0xBB, 0xA2, 0x0B, 0x1B, 0x5B, 0x75 } \
  }

#define holy_EFI_TIANO_CUSTOM_DECOMPRESS_GUID \
  { 0xa31280ad, 0x481e, 0x41b6, \
    { 0x95, 0xe8, 0x12, 0x7f, 0x4c, 0x98, 0x47, 0x79 } \
  }

#define holy_EFI_CRC32_GUIDED_SECTION_EXTRACTION_GUID \
  { 0xfc1bcdb0, 0x7d31, 0x49aa, \
    { 0x93, 0x6a, 0xa4, 0x60, 0x0d, 0x9d, 0xd0, 0x83 } \
  }

#define holy_EFI_LZMA_CUSTOM_DECOMPRESS_GUID \
  { 0xee4e5898, 0x3914, 0x4259, \
    { 0x9d, 0x6e, 0xdc, 0x7b, 0xd7, 0x94, 0x03, 0xcf } \
  }

#define holy_EFI_TSC_FREQUENCY_GUID \
  { 0xdba6a7e3, 0xbb57, 0x4be7, \
    { 0x8a, 0xf8, 0xd5, 0x78, 0xdb, 0x7e, 0x56, 0x87 } \
  }

#define holy_EFI_SYSTEM_RESOURCE_TABLE_GUID \
  { 0xb122a263, 0x3661, 0x4f68, \
    { 0x99, 0x29, 0x78, 0xf8, 0xb0, 0xd6, 0x21, 0x80 } \
  }

#define holy_EFI_DXE_SERVICES_TABLE_GUID \
  { 0x05ad34ba, 0x6f02, 0x4214, \
    { 0x95, 0x2e, 0x4d, 0xa0, 0x39, 0x8e, 0x2b, 0xb9 } \
  }

#define holy_EFI_HOB_LIST_GUID \
  { 0x7739f24c, 0x93d7, 0x11d4, \
    { 0x9a, 0x3a, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define holy_EFI_MEMORY_TYPE_INFORMATION_GUID \
  { 0x4c19049f, 0x4137, 0x4dd3, \
    { 0x9c, 0x10, 0x8b, 0x97, 0xa8, 0x3f, 0xfd, 0xfa } \
  }

#define holy_EFI_DEBUG_IMAGE_INFO_TABLE_GUID \
  { 0x49152e77, 0x1ada, 0x4764, \
    { 0xb7, 0xa2, 0x7a, 0xfe, 0xfe, 0xd9, 0x5e, 0x8b } \
  }

#define holy_EFI_MPS_TABLE_GUID	\
  { 0xeb9d2d2f, 0x2d88, 0x11d3, \
    { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define holy_EFI_ACPI_TABLE_GUID	\
  { 0xeb9d2d30, 0x2d88, 0x11d3, \
    { 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define holy_EFI_ACPI_20_TABLE_GUID	\
  { 0x8868e871, 0xe4f1, 0x11d3, \
    { 0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } \
  }

#define holy_EFI_SMBIOS_TABLE_GUID	\
  { 0xeb9d2d31, 0x2d88, 0x11d3, \
    { 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define holy_EFI_SMBIOS3_TABLE_GUID	\
  { 0xf2fd1544, 0x9794, 0x4a2c, \
    { 0x99, 0x2e, 0xe5, 0xbb, 0xcf, 0x20, 0xe3, 0x94 } \
  }

#define holy_EFI_SAL_TABLE_GUID \
  { 0xeb9d2d32, 0x2d88, 0x11d3, \
      { 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define holy_EFI_HCDP_TABLE_GUID \
  { 0xf951938d, 0x620b, 0x42ef, \
      { 0x82, 0x79, 0xa8, 0x4b, 0x79, 0x61, 0x78, 0x98 } \
  }

#define holy_EFI_DEVICE_TREE_GUID \
  { 0xb1b621d5, 0xf19c, 0x41a5, \
      { 0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0 } \
  }

#define holy_EFI_VENDOR_APPLE_GUID \
  { 0x2B0585EB, 0xD8B8, 0x49A9,	\
      { 0x8B, 0x8C, 0xE2, 0x1B, 0x01, 0xAE, 0xF2, 0xB7 } \
  }

struct holy_efi_sal_system_table
{
  holy_uint32_t signature;
  holy_uint32_t total_table_len;
  holy_uint16_t sal_rev;
  holy_uint16_t entry_count;
  holy_uint8_t checksum;
  holy_uint8_t reserved1[7];
  holy_uint16_t sal_a_version;
  holy_uint16_t sal_b_version;
  holy_uint8_t oem_id[32];
  holy_uint8_t product_id[32];
  holy_uint8_t reserved2[8];
  holy_uint8_t entries[0];
};

enum
  {
    holy_EFI_SAL_SYSTEM_TABLE_TYPE_ENTRYPOINT_DESCRIPTOR = 0,
    holy_EFI_SAL_SYSTEM_TABLE_TYPE_MEMORY_DESCRIPTOR = 1,
    holy_EFI_SAL_SYSTEM_TABLE_TYPE_PLATFORM_FEATURES = 2,
    holy_EFI_SAL_SYSTEM_TABLE_TYPE_TRANSLATION_REGISTER_DESCRIPTOR = 3,
    holy_EFI_SAL_SYSTEM_TABLE_TYPE_PURGE_TRANSLATION_COHERENCE = 4,
    holy_EFI_SAL_SYSTEM_TABLE_TYPE_AP_WAKEUP = 5
  };

struct holy_efi_sal_system_table_entrypoint_descriptor
{
  holy_uint8_t type;
  holy_uint8_t pad[7];
  holy_uint64_t pal_proc_addr;
  holy_uint64_t sal_proc_addr;
  holy_uint64_t global_data_ptr;
  holy_uint64_t reserved[2];
};

struct holy_efi_sal_system_table_memory_descriptor
{
  holy_uint8_t type;
  holy_uint8_t sal_used;
  holy_uint8_t attr;
  holy_uint8_t ar;
  holy_uint8_t attr_mask;
  holy_uint8_t mem_type;
  holy_uint8_t usage;
  holy_uint8_t unknown;
  holy_uint64_t addr;
  holy_uint64_t len;
  holy_uint64_t unknown2;
};

struct holy_efi_sal_system_table_platform_features
{
  holy_uint8_t type;
  holy_uint8_t flags;
  holy_uint8_t reserved[14];
};

struct holy_efi_sal_system_table_translation_register_descriptor
{
  holy_uint8_t type;
  holy_uint8_t register_type;
  holy_uint8_t register_number;
  holy_uint8_t reserved[5];
  holy_uint64_t addr;
  holy_uint64_t page_size;
  holy_uint64_t reserver;
};

struct holy_efi_sal_system_table_purge_translation_coherence
{
  holy_uint8_t type;
  holy_uint8_t reserved[3];
  holy_uint32_t ndomains;
  holy_uint64_t coherence;
};

struct holy_efi_sal_system_table_ap_wakeup
{
  holy_uint8_t type;
  holy_uint8_t mechanism;
  holy_uint8_t reserved[6];
  holy_uint64_t vector;
};

enum
  {
    holy_EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_BUSLOCK = 1,
    holy_EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_IRQREDIRECT = 2,
    holy_EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_IPIREDIRECT = 4,
    holy_EFI_SAL_SYSTEM_TABLE_PLATFORM_FEATURE_ITCDRIFT = 8,
  };

typedef enum holy_efi_parity_type
  {
    holy_EFI_SERIAL_DEFAULT_PARITY,
    holy_EFI_SERIAL_NO_PARITY,
    holy_EFI_SERIAL_EVEN_PARITY,
    holy_EFI_SERIAL_ODD_PARITY
  }
holy_efi_parity_type_t;

typedef enum holy_efi_stop_bits
  {
    holy_EFI_SERIAL_DEFAULT_STOP_BITS,
    holy_EFI_SERIAL_1_STOP_BIT,
    holy_EFI_SERIAL_1_5_STOP_BITS,
    holy_EFI_SERIAL_2_STOP_BITS
  }
  holy_efi_stop_bits_t;

/* Enumerations.  */
enum holy_efi_timer_delay
  {
    holy_EFI_TIMER_CANCEL,
    holy_EFI_TIMER_PERIODIC,
    holy_EFI_TIMER_RELATIVE
  };
typedef enum holy_efi_timer_delay holy_efi_timer_delay_t;

enum holy_efi_allocate_type
  {
    holy_EFI_ALLOCATE_ANY_PAGES,
    holy_EFI_ALLOCATE_MAX_ADDRESS,
    holy_EFI_ALLOCATE_ADDRESS,
    holy_EFI_MAX_ALLOCATION_TYPE
  };
typedef enum holy_efi_allocate_type holy_efi_allocate_type_t;

enum holy_efi_memory_type
  {
    holy_EFI_RESERVED_MEMORY_TYPE,
    holy_EFI_LOADER_CODE,
    holy_EFI_LOADER_DATA,
    holy_EFI_BOOT_SERVICES_CODE,
    holy_EFI_BOOT_SERVICES_DATA,
    holy_EFI_RUNTIME_SERVICES_CODE,
    holy_EFI_RUNTIME_SERVICES_DATA,
    holy_EFI_CONVENTIONAL_MEMORY,
    holy_EFI_UNUSABLE_MEMORY,
    holy_EFI_ACPI_RECLAIM_MEMORY,
    holy_EFI_ACPI_MEMORY_NVS,
    holy_EFI_MEMORY_MAPPED_IO,
    holy_EFI_MEMORY_MAPPED_IO_PORT_SPACE,
    holy_EFI_PAL_CODE,
    holy_EFI_PERSISTENT_MEMORY,
    holy_EFI_MAX_MEMORY_TYPE
  };
typedef enum holy_efi_memory_type holy_efi_memory_type_t;

enum holy_efi_interface_type
  {
    holy_EFI_NATIVE_INTERFACE
  };
typedef enum holy_efi_interface_type holy_efi_interface_type_t;

enum holy_efi_locate_search_type
  {
    holy_EFI_ALL_HANDLES,
    holy_EFI_BY_REGISTER_NOTIFY,
    holy_EFI_BY_PROTOCOL
  };
typedef enum holy_efi_locate_search_type holy_efi_locate_search_type_t;

enum holy_efi_reset_type
  {
    holy_EFI_RESET_COLD,
    holy_EFI_RESET_WARM,
    holy_EFI_RESET_SHUTDOWN
  };
typedef enum holy_efi_reset_type holy_efi_reset_type_t;

/* Types.  */
typedef char holy_efi_boolean_t;
#if holy_CPU_SIZEOF_VOID_P == 8
typedef holy_int64_t holy_efi_intn_t;
typedef holy_uint64_t holy_efi_uintn_t;
#else
typedef holy_int32_t holy_efi_intn_t;
typedef holy_uint32_t holy_efi_uintn_t;
#endif
typedef holy_int8_t holy_efi_int8_t;
typedef holy_uint8_t holy_efi_uint8_t;
typedef holy_int16_t holy_efi_int16_t;
typedef holy_uint16_t holy_efi_uint16_t;
typedef holy_int32_t holy_efi_int32_t;
typedef holy_uint32_t holy_efi_uint32_t;
typedef holy_int64_t holy_efi_int64_t;
typedef holy_uint64_t holy_efi_uint64_t;
typedef holy_uint8_t holy_efi_char8_t;
typedef holy_uint16_t holy_efi_char16_t;

typedef holy_efi_intn_t holy_efi_status_t;

#define holy_EFI_ERROR_CODE(value)	\
  ((((holy_efi_status_t) 1) << (sizeof (holy_efi_status_t) * 8 - 1)) | (value))

#define holy_EFI_WARNING_CODE(value)	(value)

#define holy_EFI_SUCCESS		0

#define holy_EFI_LOAD_ERROR		holy_EFI_ERROR_CODE (1)
#define holy_EFI_INVALID_PARAMETER	holy_EFI_ERROR_CODE (2)
#define holy_EFI_UNSUPPORTED		holy_EFI_ERROR_CODE (3)
#define holy_EFI_BAD_BUFFER_SIZE	holy_EFI_ERROR_CODE (4)
#define holy_EFI_BUFFER_TOO_SMALL	holy_EFI_ERROR_CODE (5)
#define holy_EFI_NOT_READY		holy_EFI_ERROR_CODE (6)
#define holy_EFI_DEVICE_ERROR		holy_EFI_ERROR_CODE (7)
#define holy_EFI_WRITE_PROTECTED	holy_EFI_ERROR_CODE (8)
#define holy_EFI_OUT_OF_RESOURCES	holy_EFI_ERROR_CODE (9)
#define holy_EFI_VOLUME_CORRUPTED	holy_EFI_ERROR_CODE (10)
#define holy_EFI_VOLUME_FULL		holy_EFI_ERROR_CODE (11)
#define holy_EFI_NO_MEDIA		holy_EFI_ERROR_CODE (12)
#define holy_EFI_MEDIA_CHANGED		holy_EFI_ERROR_CODE (13)
#define holy_EFI_NOT_FOUND		holy_EFI_ERROR_CODE (14)
#define holy_EFI_ACCESS_DENIED		holy_EFI_ERROR_CODE (15)
#define holy_EFI_NO_RESPONSE		holy_EFI_ERROR_CODE (16)
#define holy_EFI_NO_MAPPING		holy_EFI_ERROR_CODE (17)
#define holy_EFI_TIMEOUT		holy_EFI_ERROR_CODE (18)
#define holy_EFI_NOT_STARTED		holy_EFI_ERROR_CODE (19)
#define holy_EFI_ALREADY_STARTED	holy_EFI_ERROR_CODE (20)
#define holy_EFI_ABORTED		holy_EFI_ERROR_CODE (21)
#define holy_EFI_ICMP_ERROR		holy_EFI_ERROR_CODE (22)
#define holy_EFI_TFTP_ERROR		holy_EFI_ERROR_CODE (23)
#define holy_EFI_PROTOCOL_ERROR		holy_EFI_ERROR_CODE (24)
#define holy_EFI_INCOMPATIBLE_VERSION	holy_EFI_ERROR_CODE (25)
#define holy_EFI_SECURITY_VIOLATION	holy_EFI_ERROR_CODE (26)
#define holy_EFI_CRC_ERROR		holy_EFI_ERROR_CODE (27)

#define holy_EFI_WARN_UNKNOWN_GLYPH	holy_EFI_WARNING_CODE (1)
#define holy_EFI_WARN_DELETE_FAILURE	holy_EFI_WARNING_CODE (2)
#define holy_EFI_WARN_WRITE_FAILURE	holy_EFI_WARNING_CODE (3)
#define holy_EFI_WARN_BUFFER_TOO_SMALL	holy_EFI_WARNING_CODE (4)

typedef void *holy_efi_handle_t;
typedef void *holy_efi_event_t;
typedef holy_efi_uint64_t holy_efi_lba_t;
typedef holy_efi_uintn_t holy_efi_tpl_t;
typedef holy_uint8_t holy_efi_mac_address_t[32];
typedef holy_uint8_t holy_efi_ipv4_address_t[4];
typedef holy_uint16_t holy_efi_ipv6_address_t[8];
typedef holy_uint8_t holy_efi_ip_address_t[8] __attribute__ ((aligned(4)));
typedef holy_efi_uint64_t holy_efi_physical_address_t;
typedef holy_efi_uint64_t holy_efi_virtual_address_t;

struct holy_efi_guid
{
  holy_uint32_t data1;
  holy_uint16_t data2;
  holy_uint16_t data3;
  holy_uint8_t data4[8];
} __attribute__ ((aligned(8)));
typedef struct holy_efi_guid holy_efi_guid_t;

struct holy_efi_packed_guid
{
  holy_uint32_t data1;
  holy_uint16_t data2;
  holy_uint16_t data3;
  holy_uint8_t data4[8];
} holy_PACKED;
typedef struct holy_efi_packed_guid holy_efi_packed_guid_t;

/* XXX although the spec does not specify the padding, this actually
   must have the padding!  */
struct holy_efi_memory_descriptor
{
  holy_efi_uint32_t type;
  holy_efi_uint32_t padding;
  holy_efi_physical_address_t physical_start;
  holy_efi_virtual_address_t virtual_start;
  holy_efi_uint64_t num_pages;
  holy_efi_uint64_t attribute;
} holy_PACKED;
typedef struct holy_efi_memory_descriptor holy_efi_memory_descriptor_t;

/* Device Path definitions.  */
struct holy_efi_device_path
{
  holy_efi_uint8_t type;
  holy_efi_uint8_t subtype;
  holy_efi_uint16_t length;
} holy_PACKED;
typedef struct holy_efi_device_path holy_efi_device_path_t;
/* XXX EFI does not define EFI_DEVICE_PATH_PROTOCOL but uses it.
   It seems to be identical to EFI_DEVICE_PATH.  */
typedef struct holy_efi_device_path holy_efi_device_path_protocol_t;

#define holy_EFI_DEVICE_PATH_TYPE(dp)		((dp)->type & 0x7f)
#define holy_EFI_DEVICE_PATH_SUBTYPE(dp)	((dp)->subtype)
#define holy_EFI_DEVICE_PATH_LENGTH(dp)		((dp)->length)

/* The End of Device Path nodes.  */
#define holy_EFI_END_DEVICE_PATH_TYPE			(0xff & 0x7f)

#define holy_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE		0xff
#define holy_EFI_END_THIS_DEVICE_PATH_SUBTYPE		0x01

#define holy_EFI_END_ENTIRE_DEVICE_PATH(dp)	\
  (holy_EFI_DEVICE_PATH_TYPE (dp) == holy_EFI_END_DEVICE_PATH_TYPE \
   && (holy_EFI_DEVICE_PATH_SUBTYPE (dp) \
       == holy_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE))

#define holy_EFI_NEXT_DEVICE_PATH(dp)	\
  ((holy_efi_device_path_t *) ((char *) (dp) \
                               + holy_EFI_DEVICE_PATH_LENGTH (dp)))

/* Hardware Device Path.  */
#define holy_EFI_HARDWARE_DEVICE_PATH_TYPE		1

#define holy_EFI_PCI_DEVICE_PATH_SUBTYPE		1

struct holy_efi_pci_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint8_t function;
  holy_efi_uint8_t device;
} holy_PACKED;
typedef struct holy_efi_pci_device_path holy_efi_pci_device_path_t;

#define holy_EFI_PCCARD_DEVICE_PATH_SUBTYPE		2

struct holy_efi_pccard_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint8_t function;
} holy_PACKED;
typedef struct holy_efi_pccard_device_path holy_efi_pccard_device_path_t;

#define holy_EFI_MEMORY_MAPPED_DEVICE_PATH_SUBTYPE	3

struct holy_efi_memory_mapped_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint32_t memory_type;
  holy_efi_physical_address_t start_address;
  holy_efi_physical_address_t end_address;
} holy_PACKED;
typedef struct holy_efi_memory_mapped_device_path holy_efi_memory_mapped_device_path_t;

#define holy_EFI_VENDOR_DEVICE_PATH_SUBTYPE		4

struct holy_efi_vendor_device_path
{
  holy_efi_device_path_t header;
  holy_efi_packed_guid_t vendor_guid;
  holy_efi_uint8_t vendor_defined_data[0];
} holy_PACKED;
typedef struct holy_efi_vendor_device_path holy_efi_vendor_device_path_t;

#define holy_EFI_CONTROLLER_DEVICE_PATH_SUBTYPE		5

struct holy_efi_controller_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint32_t controller_number;
} holy_PACKED;
typedef struct holy_efi_controller_device_path holy_efi_controller_device_path_t;

/* ACPI Device Path.  */
#define holy_EFI_ACPI_DEVICE_PATH_TYPE			2

#define holy_EFI_ACPI_DEVICE_PATH_SUBTYPE		1

struct holy_efi_acpi_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint32_t hid;
  holy_efi_uint32_t uid;
} holy_PACKED;
typedef struct holy_efi_acpi_device_path holy_efi_acpi_device_path_t;

#define holy_EFI_EXPANDED_ACPI_DEVICE_PATH_SUBTYPE	2

struct holy_efi_expanded_acpi_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint32_t hid;
  holy_efi_uint32_t uid;
  holy_efi_uint32_t cid;
  char hidstr[0];
} holy_PACKED;
typedef struct holy_efi_expanded_acpi_device_path holy_efi_expanded_acpi_device_path_t;

#define holy_EFI_EXPANDED_ACPI_HIDSTR(dp)	\
  (((holy_efi_expanded_acpi_device_path_t *) dp)->hidstr)
#define holy_EFI_EXPANDED_ACPI_UIDSTR(dp)	\
  (holy_EFI_EXPANDED_ACPI_HIDSTR(dp) \
   + holy_strlen (holy_EFI_EXPANDED_ACPI_HIDSTR(dp)) + 1)
#define holy_EFI_EXPANDED_ACPI_CIDSTR(dp)	\
  (holy_EFI_EXPANDED_ACPI_UIDSTR(dp) \
   + holy_strlen (holy_EFI_EXPANDED_ACPI_UIDSTR(dp)) + 1)

/* Messaging Device Path.  */
#define holy_EFI_MESSAGING_DEVICE_PATH_TYPE		3

#define holy_EFI_ATAPI_DEVICE_PATH_SUBTYPE		1

struct holy_efi_atapi_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint8_t primary_secondary;
  holy_efi_uint8_t slave_master;
  holy_efi_uint16_t lun;
} holy_PACKED;
typedef struct holy_efi_atapi_device_path holy_efi_atapi_device_path_t;

#define holy_EFI_SCSI_DEVICE_PATH_SUBTYPE		2

struct holy_efi_scsi_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint16_t pun;
  holy_efi_uint16_t lun;
} holy_PACKED;
typedef struct holy_efi_scsi_device_path holy_efi_scsi_device_path_t;

#define holy_EFI_FIBRE_CHANNEL_DEVICE_PATH_SUBTYPE	3

struct holy_efi_fibre_channel_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint32_t reserved;
  holy_efi_uint64_t wwn;
  holy_efi_uint64_t lun;
} holy_PACKED;
typedef struct holy_efi_fibre_channel_device_path holy_efi_fibre_channel_device_path_t;

#define holy_EFI_1394_DEVICE_PATH_SUBTYPE		4

struct holy_efi_1394_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint32_t reserved;
  holy_efi_uint64_t guid;
} holy_PACKED;
typedef struct holy_efi_1394_device_path holy_efi_1394_device_path_t;

#define holy_EFI_USB_DEVICE_PATH_SUBTYPE		5

struct holy_efi_usb_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint8_t parent_port_number;
  holy_efi_uint8_t usb_interface;
} holy_PACKED;
typedef struct holy_efi_usb_device_path holy_efi_usb_device_path_t;

#define holy_EFI_USB_CLASS_DEVICE_PATH_SUBTYPE		15

struct holy_efi_usb_class_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint16_t vendor_id;
  holy_efi_uint16_t product_id;
  holy_efi_uint8_t device_class;
  holy_efi_uint8_t device_subclass;
  holy_efi_uint8_t device_protocol;
} holy_PACKED;
typedef struct holy_efi_usb_class_device_path holy_efi_usb_class_device_path_t;

#define holy_EFI_I2O_DEVICE_PATH_SUBTYPE		6

struct holy_efi_i2o_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint32_t tid;
} holy_PACKED;
typedef struct holy_efi_i2o_device_path holy_efi_i2o_device_path_t;

#define holy_EFI_MAC_ADDRESS_DEVICE_PATH_SUBTYPE	11

struct holy_efi_mac_address_device_path
{
  holy_efi_device_path_t header;
  holy_efi_mac_address_t mac_address;
  holy_efi_uint8_t if_type;
} holy_PACKED;
typedef struct holy_efi_mac_address_device_path holy_efi_mac_address_device_path_t;

#define holy_EFI_IPV4_DEVICE_PATH_SUBTYPE		12

struct holy_efi_ipv4_device_path
{
  holy_efi_device_path_t header;
  holy_efi_ipv4_address_t local_ip_address;
  holy_efi_ipv4_address_t remote_ip_address;
  holy_efi_uint16_t local_port;
  holy_efi_uint16_t remote_port;
  holy_efi_uint16_t protocol;
  holy_efi_uint8_t static_ip_address;
} holy_PACKED;
typedef struct holy_efi_ipv4_device_path holy_efi_ipv4_device_path_t;

#define holy_EFI_IPV6_DEVICE_PATH_SUBTYPE		13

struct holy_efi_ipv6_device_path
{
  holy_efi_device_path_t header;
  holy_efi_ipv6_address_t local_ip_address;
  holy_efi_ipv6_address_t remote_ip_address;
  holy_efi_uint16_t local_port;
  holy_efi_uint16_t remote_port;
  holy_efi_uint16_t protocol;
  holy_efi_uint8_t static_ip_address;
} holy_PACKED;
typedef struct holy_efi_ipv6_device_path holy_efi_ipv6_device_path_t;

#define holy_EFI_INFINIBAND_DEVICE_PATH_SUBTYPE		9

struct holy_efi_infiniband_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint32_t resource_flags;
  holy_efi_uint8_t port_gid[16];
  holy_efi_uint64_t remote_id;
  holy_efi_uint64_t target_port_id;
  holy_efi_uint64_t device_id;
} holy_PACKED;
typedef struct holy_efi_infiniband_device_path holy_efi_infiniband_device_path_t;

#define holy_EFI_UART_DEVICE_PATH_SUBTYPE		14

struct holy_efi_uart_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint32_t reserved;
  holy_efi_uint64_t baud_rate;
  holy_efi_uint8_t data_bits;
  holy_efi_uint8_t parity;
  holy_efi_uint8_t stop_bits;
} holy_PACKED;
typedef struct holy_efi_uart_device_path holy_efi_uart_device_path_t;

#define holy_EFI_SATA_DEVICE_PATH_SUBTYPE		18

struct holy_efi_sata_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint16_t hba_port;
  holy_efi_uint16_t multiplier_port;
  holy_efi_uint16_t lun;
} holy_PACKED;
typedef struct holy_efi_sata_device_path holy_efi_sata_device_path_t;

#define holy_EFI_VENDOR_MESSAGING_DEVICE_PATH_SUBTYPE	10

/* Media Device Path.  */
#define holy_EFI_MEDIA_DEVICE_PATH_TYPE			4

#define holy_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE		1

struct holy_efi_hard_drive_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint32_t partition_number;
  holy_efi_lba_t partition_start;
  holy_efi_lba_t partition_size;
  holy_efi_uint8_t partition_signature[16];
  holy_efi_uint8_t partmap_type;
  holy_efi_uint8_t signature_type;
} holy_PACKED;
typedef struct holy_efi_hard_drive_device_path holy_efi_hard_drive_device_path_t;

#define holy_EFI_CDROM_DEVICE_PATH_SUBTYPE		2

struct holy_efi_cdrom_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint32_t boot_entry;
  holy_efi_lba_t partition_start;
  holy_efi_lba_t partition_size;
} holy_PACKED;
typedef struct holy_efi_cdrom_device_path holy_efi_cdrom_device_path_t;

#define holy_EFI_VENDOR_MEDIA_DEVICE_PATH_SUBTYPE	3

struct holy_efi_vendor_media_device_path
{
  holy_efi_device_path_t header;
  holy_efi_packed_guid_t vendor_guid;
  holy_efi_uint8_t vendor_defined_data[0];
} holy_PACKED;
typedef struct holy_efi_vendor_media_device_path holy_efi_vendor_media_device_path_t;

#define holy_EFI_FILE_PATH_DEVICE_PATH_SUBTYPE		4

struct holy_efi_file_path_device_path
{
  holy_efi_device_path_t header;
  holy_efi_char16_t path_name[0];
} holy_PACKED;
typedef struct holy_efi_file_path_device_path holy_efi_file_path_device_path_t;

#define holy_EFI_PROTOCOL_DEVICE_PATH_SUBTYPE		5

struct holy_efi_protocol_device_path
{
  holy_efi_device_path_t header;
  holy_efi_packed_guid_t guid;
} holy_PACKED;
typedef struct holy_efi_protocol_device_path holy_efi_protocol_device_path_t;

#define holy_EFI_PIWG_DEVICE_PATH_SUBTYPE		6

struct holy_efi_piwg_device_path
{
  holy_efi_device_path_t header;
  holy_efi_packed_guid_t guid;
} holy_PACKED;
typedef struct holy_efi_piwg_device_path holy_efi_piwg_device_path_t;


/* BIOS Boot Specification Device Path.  */
#define holy_EFI_BIOS_DEVICE_PATH_TYPE			5

#define holy_EFI_BIOS_DEVICE_PATH_SUBTYPE		1

struct holy_efi_bios_device_path
{
  holy_efi_device_path_t header;
  holy_efi_uint16_t device_type;
  holy_efi_uint16_t status_flags;
  char description[0];
} holy_PACKED;
typedef struct holy_efi_bios_device_path holy_efi_bios_device_path_t;

struct holy_efi_open_protocol_information_entry
{
  holy_efi_handle_t agent_handle;
  holy_efi_handle_t controller_handle;
  holy_efi_uint32_t attributes;
  holy_efi_uint32_t open_count;
};
typedef struct holy_efi_open_protocol_information_entry holy_efi_open_protocol_information_entry_t;

struct holy_efi_time
{
  holy_efi_uint16_t year;
  holy_efi_uint8_t month;
  holy_efi_uint8_t day;
  holy_efi_uint8_t hour;
  holy_efi_uint8_t minute;
  holy_efi_uint8_t second;
  holy_efi_uint8_t pad1;
  holy_efi_uint32_t nanosecond;
  holy_efi_int16_t time_zone;
  holy_efi_uint8_t daylight;
  holy_efi_uint8_t pad2;
} holy_PACKED;
typedef struct holy_efi_time holy_efi_time_t;

struct holy_efi_time_capabilities
{
  holy_efi_uint32_t resolution;
  holy_efi_uint32_t accuracy;
  holy_efi_boolean_t sets_to_zero;
};
typedef struct holy_efi_time_capabilities holy_efi_time_capabilities_t;

struct holy_efi_input_key
{
  holy_efi_uint16_t scan_code;
  holy_efi_char16_t unicode_char;
};
typedef struct holy_efi_input_key holy_efi_input_key_t;

typedef holy_efi_uint8_t holy_efi_key_toggle_state_t;
struct holy_efi_key_state
{
	holy_efi_uint32_t key_shift_state;
	holy_efi_key_toggle_state_t key_toggle_state;
};
typedef struct holy_efi_key_state holy_efi_key_state_t;

#define holy_EFI_SHIFT_STATE_VALID     0x80000000
#define holy_EFI_RIGHT_SHIFT_PRESSED   0x00000001
#define holy_EFI_LEFT_SHIFT_PRESSED    0x00000002
#define holy_EFI_RIGHT_CONTROL_PRESSED 0x00000004
#define holy_EFI_LEFT_CONTROL_PRESSED  0x00000008
#define holy_EFI_RIGHT_ALT_PRESSED     0x00000010
#define holy_EFI_LEFT_ALT_PRESSED      0x00000020
#define holy_EFI_RIGHT_LOGO_PRESSED    0x00000040
#define holy_EFI_LEFT_LOGO_PRESSED     0x00000080
#define holy_EFI_MENU_KEY_PRESSED      0x00000100
#define holy_EFI_SYS_REQ_PRESSED       0x00000200

#define holy_EFI_TOGGLE_STATE_VALID 0x80
#define holy_EFI_KEY_STATE_EXPOSED  0x40
#define holy_EFI_SCROLL_LOCK_ACTIVE 0x01
#define holy_EFI_NUM_LOCK_ACTIVE    0x02
#define holy_EFI_CAPS_LOCK_ACTIVE   0x04

struct holy_efi_simple_text_output_mode
{
  holy_efi_int32_t max_mode;
  holy_efi_int32_t mode;
  holy_efi_int32_t attribute;
  holy_efi_int32_t cursor_column;
  holy_efi_int32_t cursor_row;
  holy_efi_boolean_t cursor_visible;
};
typedef struct holy_efi_simple_text_output_mode holy_efi_simple_text_output_mode_t;

/* Tables.  */
struct holy_efi_table_header
{
  holy_efi_uint64_t signature;
  holy_efi_uint32_t revision;
  holy_efi_uint32_t header_size;
  holy_efi_uint32_t crc32;
  holy_efi_uint32_t reserved;
};
typedef struct holy_efi_table_header holy_efi_table_header_t;

struct holy_efi_boot_services
{
  holy_efi_table_header_t hdr;

  holy_efi_tpl_t
  (*raise_tpl) (holy_efi_tpl_t new_tpl);

  void
  (*restore_tpl) (holy_efi_tpl_t old_tpl);

  holy_efi_status_t
  (*allocate_pages) (holy_efi_allocate_type_t type,
		     holy_efi_memory_type_t memory_type,
		     holy_efi_uintn_t pages,
		     holy_efi_physical_address_t *memory);

  holy_efi_status_t
  (*free_pages) (holy_efi_physical_address_t memory,
		 holy_efi_uintn_t pages);

  holy_efi_status_t
  (*get_memory_map) (holy_efi_uintn_t *memory_map_size,
		     holy_efi_memory_descriptor_t *memory_map,
		     holy_efi_uintn_t *map_key,
		     holy_efi_uintn_t *descriptor_size,
		     holy_efi_uint32_t *descriptor_version);

  holy_efi_status_t
  (*allocate_pool) (holy_efi_memory_type_t pool_type,
		    holy_efi_uintn_t size,
		    void **buffer);

  holy_efi_status_t
  (*free_pool) (void *buffer);

  holy_efi_status_t
  (*create_event) (holy_efi_uint32_t type,
		   holy_efi_tpl_t notify_tpl,
		   void (*notify_function) (holy_efi_event_t event,
					    void *context),
		   void *notify_context,
		   holy_efi_event_t *event);

  holy_efi_status_t
  (*set_timer) (holy_efi_event_t event,
		holy_efi_timer_delay_t type,
		holy_efi_uint64_t trigger_time);

   holy_efi_status_t
   (*wait_for_event) (holy_efi_uintn_t num_events,
		      holy_efi_event_t *event,
		      holy_efi_uintn_t *index);

  holy_efi_status_t
  (*signal_event) (holy_efi_event_t event);

  holy_efi_status_t
  (*close_event) (holy_efi_event_t event);

  holy_efi_status_t
  (*check_event) (holy_efi_event_t event);

   holy_efi_status_t
   (*install_protocol_interface) (holy_efi_handle_t *handle,
				  holy_efi_guid_t *protocol,
				  holy_efi_interface_type_t protocol_interface_type,
				  void *protocol_interface);

  holy_efi_status_t
  (*reinstall_protocol_interface) (holy_efi_handle_t handle,
				   holy_efi_guid_t *protocol,
				   void *old_interface,
				   void *new_interface);

  holy_efi_status_t
  (*uninstall_protocol_interface) (holy_efi_handle_t handle,
				   holy_efi_guid_t *protocol,
				   void *protocol_interface);

  holy_efi_status_t
  (*handle_protocol) (holy_efi_handle_t handle,
		      holy_efi_guid_t *protocol,
		      void **protocol_interface);

  void *reserved;

  holy_efi_status_t
  (*register_protocol_notify) (holy_efi_guid_t *protocol,
			       holy_efi_event_t event,
			       void **registration);

  holy_efi_status_t
  (*locate_handle) (holy_efi_locate_search_type_t search_type,
		    holy_efi_guid_t *protocol,
		    void *search_key,
		    holy_efi_uintn_t *buffer_size,
		    holy_efi_handle_t *buffer);

  holy_efi_status_t
  (*locate_device_path) (holy_efi_guid_t *protocol,
			 holy_efi_device_path_t **device_path,
			 holy_efi_handle_t *device);

  holy_efi_status_t
  (*install_configuration_table) (holy_efi_guid_t *guid, void *table);

  holy_efi_status_t
  (*load_image) (holy_efi_boolean_t boot_policy,
		 holy_efi_handle_t parent_image_handle,
		 holy_efi_device_path_t *file_path,
		 void *source_buffer,
		 holy_efi_uintn_t source_size,
		 holy_efi_handle_t *image_handle);

  holy_efi_status_t
  (*start_image) (holy_efi_handle_t image_handle,
		  holy_efi_uintn_t *exit_data_size,
		  holy_efi_char16_t **exit_data);

  holy_efi_status_t
  (*exit) (holy_efi_handle_t image_handle,
	   holy_efi_status_t exit_status,
	   holy_efi_uintn_t exit_data_size,
	   holy_efi_char16_t *exit_data) __attribute__((noreturn));

  holy_efi_status_t
  (*unload_image) (holy_efi_handle_t image_handle);

  holy_efi_status_t
  (*exit_boot_services) (holy_efi_handle_t image_handle,
			 holy_efi_uintn_t map_key);

  holy_efi_status_t
  (*get_next_monotonic_count) (holy_efi_uint64_t *count);

  holy_efi_status_t
  (*stall) (holy_efi_uintn_t microseconds);

  holy_efi_status_t
  (*set_watchdog_timer) (holy_efi_uintn_t timeout,
			 holy_efi_uint64_t watchdog_code,
			 holy_efi_uintn_t data_size,
			 holy_efi_char16_t *watchdog_data);

  holy_efi_status_t
  (*connect_controller) (holy_efi_handle_t controller_handle,
			 holy_efi_handle_t *driver_image_handle,
			 holy_efi_device_path_protocol_t *remaining_device_path,
			 holy_efi_boolean_t recursive);

  holy_efi_status_t
  (*disconnect_controller) (holy_efi_handle_t controller_handle,
			    holy_efi_handle_t driver_image_handle,
			    holy_efi_handle_t child_handle);

  holy_efi_status_t
  (*open_protocol) (holy_efi_handle_t handle,
		    holy_efi_guid_t *protocol,
		    void **protocol_interface,
		    holy_efi_handle_t agent_handle,
		    holy_efi_handle_t controller_handle,
		    holy_efi_uint32_t attributes);

  holy_efi_status_t
  (*close_protocol) (holy_efi_handle_t handle,
		     holy_efi_guid_t *protocol,
		     holy_efi_handle_t agent_handle,
		     holy_efi_handle_t controller_handle);

  holy_efi_status_t
  (*open_protocol_information) (holy_efi_handle_t handle,
				holy_efi_guid_t *protocol,
				holy_efi_open_protocol_information_entry_t **entry_buffer,
				holy_efi_uintn_t *entry_count);

  holy_efi_status_t
  (*protocols_per_handle) (holy_efi_handle_t handle,
			   holy_efi_packed_guid_t ***protocol_buffer,
			   holy_efi_uintn_t *protocol_buffer_count);

  holy_efi_status_t
  (*locate_handle_buffer) (holy_efi_locate_search_type_t search_type,
			   holy_efi_guid_t *protocol,
			   void *search_key,
			   holy_efi_uintn_t *no_handles,
			   holy_efi_handle_t **buffer);

  holy_efi_status_t
  (*locate_protocol) (holy_efi_guid_t *protocol,
		      void *registration,
		      void **protocol_interface);

  holy_efi_status_t
  (*install_multiple_protocol_interfaces) (holy_efi_handle_t *handle, ...);

  holy_efi_status_t
  (*uninstall_multiple_protocol_interfaces) (holy_efi_handle_t handle, ...);

  holy_efi_status_t
  (*calculate_crc32) (void *data,
		      holy_efi_uintn_t data_size,
		      holy_efi_uint32_t *crc32);

  void
  (*copy_mem) (void *destination, void *source, holy_efi_uintn_t length);

  void
  (*set_mem) (void *buffer, holy_efi_uintn_t size, holy_efi_uint8_t value);
};
typedef struct holy_efi_boot_services holy_efi_boot_services_t;

struct holy_efi_runtime_services
{
  holy_efi_table_header_t hdr;

  holy_efi_status_t
  (*get_time) (holy_efi_time_t *time,
	       holy_efi_time_capabilities_t *capabilities);

  holy_efi_status_t
  (*set_time) (holy_efi_time_t *time);

  holy_efi_status_t
  (*get_wakeup_time) (holy_efi_boolean_t *enabled,
		      holy_efi_boolean_t *pending,
		      holy_efi_time_t *time);

  holy_efi_status_t
  (*set_wakeup_time) (holy_efi_boolean_t enabled,
		      holy_efi_time_t *time);

  holy_efi_status_t
  (*set_virtual_address_map) (holy_efi_uintn_t memory_map_size,
			      holy_efi_uintn_t descriptor_size,
			      holy_efi_uint32_t descriptor_version,
			      holy_efi_memory_descriptor_t *virtual_map);

  holy_efi_status_t
  (*convert_pointer) (holy_efi_uintn_t debug_disposition, void **address);

#define holy_EFI_GLOBAL_VARIABLE_GUID \
  { 0x8BE4DF61, 0x93CA, 0x11d2, { 0xAA, 0x0D, 0x00, 0xE0, 0x98, 0x03, 0x2B,0x8C }}


  holy_efi_status_t
  (*get_variable) (holy_efi_char16_t *variable_name,
		   const holy_efi_guid_t *vendor_guid,
		   holy_efi_uint32_t *attributes,
		   holy_efi_uintn_t *data_size,
		   void *data);

  holy_efi_status_t
  (*get_next_variable_name) (holy_efi_uintn_t *variable_name_size,
			     holy_efi_char16_t *variable_name,
			     holy_efi_guid_t *vendor_guid);

  holy_efi_status_t
  (*set_variable) (holy_efi_char16_t *variable_name,
		   const holy_efi_guid_t *vendor_guid,
		   holy_efi_uint32_t attributes,
		   holy_efi_uintn_t data_size,
		   void *data);

  holy_efi_status_t
  (*get_next_high_monotonic_count) (holy_efi_uint32_t *high_count);

  void
  (*reset_system) (holy_efi_reset_type_t reset_type,
		   holy_efi_status_t reset_status,
		   holy_efi_uintn_t data_size,
		   holy_efi_char16_t *reset_data);
};
typedef struct holy_efi_runtime_services holy_efi_runtime_services_t;

struct holy_efi_configuration_table
{
  holy_efi_packed_guid_t vendor_guid;
  void *vendor_table;
} holy_PACKED;
typedef struct holy_efi_configuration_table holy_efi_configuration_table_t;

#define holy_EFIEMU_SYSTEM_TABLE_SIGNATURE 0x5453595320494249LL
#define holy_EFIEMU_RUNTIME_SERVICES_SIGNATURE 0x56524553544e5552LL

struct holy_efi_serial_io_interface
{
  holy_efi_uint32_t revision;
  void (*reset) (void);
  holy_efi_status_t (*set_attributes) (struct holy_efi_serial_io_interface *this,
				       holy_efi_uint64_t speed,
				       holy_efi_uint32_t fifo_depth,
				       holy_efi_uint32_t timeout,
				       holy_efi_parity_type_t parity,
				       holy_uint8_t word_len,
				       holy_efi_stop_bits_t stop_bits);
  holy_efi_status_t (*set_control_bits) (struct holy_efi_serial_io_interface *this,
					 holy_efi_uint32_t flags);
  void (*get_control_bits) (void);
  holy_efi_status_t (*write) (struct holy_efi_serial_io_interface *this,
			      holy_efi_uintn_t *buf_size,
			      void *buffer);
  holy_efi_status_t (*read) (struct holy_efi_serial_io_interface *this,
			     holy_efi_uintn_t *buf_size,
			     void *buffer);
};

struct holy_efi_simple_input_interface
{
  holy_efi_status_t
  (*reset) (struct holy_efi_simple_input_interface *this,
	    holy_efi_boolean_t extended_verification);

  holy_efi_status_t
  (*read_key_stroke) (struct holy_efi_simple_input_interface *this,
		      holy_efi_input_key_t *key);

  holy_efi_event_t wait_for_key;
};
typedef struct holy_efi_simple_input_interface holy_efi_simple_input_interface_t;

struct holy_efi_key_data {
	holy_efi_input_key_t key;
	holy_efi_key_state_t key_state;
};
typedef struct holy_efi_key_data holy_efi_key_data_t;

typedef holy_efi_status_t (*holy_efi_key_notify_function_t) (
	holy_efi_key_data_t *key_data
	);

struct holy_efi_simple_text_input_ex_interface
{
	holy_efi_status_t
	(*reset) (struct holy_efi_simple_text_input_ex_interface *this,
		  holy_efi_boolean_t extended_verification);

	holy_efi_status_t
	(*read_key_stroke) (struct holy_efi_simple_text_input_ex_interface *this,
			    holy_efi_key_data_t *key_data);

	holy_efi_event_t wait_for_key;

	holy_efi_status_t
	(*set_state) (struct holy_efi_simple_text_input_ex_interface *this,
		      holy_efi_key_toggle_state_t *key_toggle_state);

	holy_efi_status_t
	(*register_key_notify) (struct holy_efi_simple_text_input_ex_interface *this,
				holy_efi_key_data_t *key_data,
				holy_efi_key_notify_function_t key_notification_function);

	holy_efi_status_t
	(*unregister_key_notify) (struct holy_efi_simple_text_input_ex_interface *this,
				  void *notification_handle);
};
typedef struct holy_efi_simple_text_input_ex_interface holy_efi_simple_text_input_ex_interface_t;

struct holy_efi_simple_text_output_interface
{
  holy_efi_status_t
  (*reset) (struct holy_efi_simple_text_output_interface *this,
	    holy_efi_boolean_t extended_verification);

  holy_efi_status_t
  (*output_string) (struct holy_efi_simple_text_output_interface *this,
		    holy_efi_char16_t *string);

  holy_efi_status_t
  (*test_string) (struct holy_efi_simple_text_output_interface *this,
		  holy_efi_char16_t *string);

  holy_efi_status_t
  (*query_mode) (struct holy_efi_simple_text_output_interface *this,
		 holy_efi_uintn_t mode_number,
		 holy_efi_uintn_t *columns,
		 holy_efi_uintn_t *rows);

  holy_efi_status_t
  (*set_mode) (struct holy_efi_simple_text_output_interface *this,
	       holy_efi_uintn_t mode_number);

  holy_efi_status_t
  (*set_attributes) (struct holy_efi_simple_text_output_interface *this,
		     holy_efi_uintn_t attribute);

  holy_efi_status_t
  (*clear_screen) (struct holy_efi_simple_text_output_interface *this);

  holy_efi_status_t
  (*set_cursor_position) (struct holy_efi_simple_text_output_interface *this,
			  holy_efi_uintn_t column,
			  holy_efi_uintn_t row);

  holy_efi_status_t
  (*enable_cursor) (struct holy_efi_simple_text_output_interface *this,
		    holy_efi_boolean_t visible);

  holy_efi_simple_text_output_mode_t *mode;
};
typedef struct holy_efi_simple_text_output_interface holy_efi_simple_text_output_interface_t;

typedef holy_uint8_t holy_efi_pxe_packet_t[1472];

typedef struct holy_efi_pxe_mode
{
  holy_uint8_t unused[52];
  holy_efi_pxe_packet_t dhcp_discover;
  holy_efi_pxe_packet_t dhcp_ack;
  holy_efi_pxe_packet_t proxy_offer;
  holy_efi_pxe_packet_t pxe_discover;
  holy_efi_pxe_packet_t pxe_reply;
} holy_efi_pxe_mode_t;

typedef struct holy_efi_pxe
{
  holy_uint64_t rev;
  void (*start) (void);
  void (*stop) (void);
  void (*dhcp) (void);
  void (*discover) (void);
  void (*mftp) (void);
  void (*udpwrite) (void);
  void (*udpread) (void);
  void (*setipfilter) (void);
  void (*arp) (void);
  void (*setparams) (void);
  void (*setstationip) (void);
  void (*setpackets) (void);
  struct holy_efi_pxe_mode *mode;
} holy_efi_pxe_t;

#define holy_EFI_BLACK		0x00
#define holy_EFI_BLUE		0x01
#define holy_EFI_GREEN		0x02
#define holy_EFI_CYAN		0x03
#define holy_EFI_RED		0x04
#define holy_EFI_MAGENTA	0x05
#define holy_EFI_BROWN		0x06
#define holy_EFI_LIGHTGRAY	0x07
#define holy_EFI_BRIGHT		0x08
#define holy_EFI_DARKGRAY	0x08
#define holy_EFI_LIGHTBLUE	0x09
#define holy_EFI_LIGHTGREEN	0x0A
#define holy_EFI_LIGHTCYAN	0x0B
#define holy_EFI_LIGHTRED	0x0C
#define holy_EFI_LIGHTMAGENTA	0x0D
#define holy_EFI_YELLOW		0x0E
#define holy_EFI_WHITE		0x0F

#define holy_EFI_BACKGROUND_BLACK	0x00
#define holy_EFI_BACKGROUND_BLUE	0x10
#define holy_EFI_BACKGROUND_GREEN	0x20
#define holy_EFI_BACKGROUND_CYAN	0x30
#define holy_EFI_BACKGROUND_RED		0x40
#define holy_EFI_BACKGROUND_MAGENTA	0x50
#define holy_EFI_BACKGROUND_BROWN	0x60
#define holy_EFI_BACKGROUND_LIGHTGRAY	0x70

#define holy_EFI_TEXT_ATTR(fg, bg)	((fg) | ((bg)))

struct holy_efi_system_table
{
  holy_efi_table_header_t hdr;
  holy_efi_char16_t *firmware_vendor;
  holy_efi_uint32_t firmware_revision;
  holy_efi_handle_t console_in_handler;
  holy_efi_simple_input_interface_t *con_in;
  holy_efi_handle_t console_out_handler;
  holy_efi_simple_text_output_interface_t *con_out;
  holy_efi_handle_t standard_error_handle;
  holy_efi_simple_text_output_interface_t *std_err;
  holy_efi_runtime_services_t *runtime_services;
  holy_efi_boot_services_t *boot_services;
  holy_efi_uintn_t num_table_entries;
  holy_efi_configuration_table_t *configuration_table;
};
typedef struct holy_efi_system_table  holy_efi_system_table_t;

struct holy_efi_loaded_image
{
  holy_efi_uint32_t revision;
  holy_efi_handle_t parent_handle;
  holy_efi_system_table_t *system_table;

  holy_efi_handle_t device_handle;
  holy_efi_device_path_t *file_path;
  void *reserved;

  holy_efi_uint32_t load_options_size;
  void *load_options;

  void *image_base;
  holy_efi_uint64_t image_size;
  holy_efi_memory_type_t image_code_type;
  holy_efi_memory_type_t image_data_type;

  holy_efi_status_t (*unload) (holy_efi_handle_t image_handle);
};
typedef struct holy_efi_loaded_image holy_efi_loaded_image_t;

struct holy_efi_disk_io
{
  holy_efi_uint64_t revision;
  holy_efi_status_t (*read) (struct holy_efi_disk_io *this,
			     holy_efi_uint32_t media_id,
			     holy_efi_uint64_t offset,
			     holy_efi_uintn_t buffer_size,
			     void *buffer);
  holy_efi_status_t (*write) (struct holy_efi_disk_io *this,
			     holy_efi_uint32_t media_id,
			     holy_efi_uint64_t offset,
			     holy_efi_uintn_t buffer_size,
			     void *buffer);
};
typedef struct holy_efi_disk_io holy_efi_disk_io_t;

struct holy_efi_block_io_media
{
  holy_efi_uint32_t media_id;
  holy_efi_boolean_t removable_media;
  holy_efi_boolean_t media_present;
  holy_efi_boolean_t logical_partition;
  holy_efi_boolean_t read_only;
  holy_efi_boolean_t write_caching;
  holy_efi_uint8_t pad[3];
  holy_efi_uint32_t block_size;
  holy_efi_uint32_t io_align;
  holy_efi_uint8_t pad2[4];
  holy_efi_lba_t last_block;
};
typedef struct holy_efi_block_io_media holy_efi_block_io_media_t;

typedef holy_uint8_t holy_efi_mac_t[32];

struct holy_efi_simple_network_mode
{
  holy_uint32_t state;
  holy_uint32_t hwaddr_size;
  holy_uint32_t media_header_size;
  holy_uint32_t max_packet_size;
  holy_uint32_t nvram_size;
  holy_uint32_t nvram_access_size;
  holy_uint32_t receive_filter_mask;
  holy_uint32_t receive_filter_setting;
  holy_uint32_t max_mcast_filter_count;
  holy_uint32_t mcast_filter_count;
  holy_efi_mac_t mcast_filter[16];
  holy_efi_mac_t current_address;
  holy_efi_mac_t broadcast_address;
  holy_efi_mac_t permanent_address;
  holy_uint8_t if_type;
  holy_uint8_t mac_changeable;
  holy_uint8_t multitx_supported;
  holy_uint8_t media_present_supported;
  holy_uint8_t media_present;
};

enum
  {
    holy_EFI_NETWORK_STOPPED,
    holy_EFI_NETWORK_STARTED,
    holy_EFI_NETWORK_INITIALIZED,
  };

enum
  {
    holy_EFI_SIMPLE_NETWORK_RECEIVE_UNICAST		  = 0x01,
    holy_EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST		  = 0x02,
    holy_EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST		  = 0x04,
    holy_EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS		  = 0x08,
    holy_EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST = 0x10,
  };

struct holy_efi_simple_network
{
  holy_uint64_t revision;
  holy_efi_status_t (*start) (struct holy_efi_simple_network *this);
  holy_efi_status_t (*stop) (struct holy_efi_simple_network *this);
  holy_efi_status_t (*initialize) (struct holy_efi_simple_network *this,
				   holy_efi_uintn_t extra_rx,
				   holy_efi_uintn_t extra_tx);
  void (*reset) (void);
  holy_efi_status_t (*shutdown) (struct holy_efi_simple_network *this);
  holy_efi_status_t (*receive_filters) (struct holy_efi_simple_network *this,
					holy_uint32_t enable,
					holy_uint32_t disable,
					holy_efi_boolean_t reset_mcast_filter,
					holy_efi_uintn_t mcast_filter_count,
					holy_efi_mac_address_t *mcast_filter);
  void (*station_address) (void);
  void (*statistics) (void);
  void (*mcastiptomac) (void);
  void (*nvdata) (void);
  holy_efi_status_t (*get_status) (struct holy_efi_simple_network *this,
				   holy_uint32_t *int_status,
				   void **txbuf);
  holy_efi_status_t (*transmit) (struct holy_efi_simple_network *this,
				 holy_efi_uintn_t header_size,
				 holy_efi_uintn_t buffer_size,
				 void *buffer,
				 holy_efi_mac_t *src_addr,
				 holy_efi_mac_t *dest_addr,
				 holy_efi_uint16_t *protocol);
  holy_efi_status_t (*receive) (struct holy_efi_simple_network *this,
				holy_efi_uintn_t *header_size,
				holy_efi_uintn_t *buffer_size,
				void *buffer,
				holy_efi_mac_t *src_addr,
				holy_efi_mac_t *dest_addr,
				holy_uint16_t *protocol);
  void (*waitforpacket) (void);
  struct holy_efi_simple_network_mode *mode;
};
typedef struct holy_efi_simple_network holy_efi_simple_network_t;


struct holy_efi_block_io
{
  holy_efi_uint64_t revision;
  holy_efi_block_io_media_t *media;
  holy_efi_status_t (*reset) (struct holy_efi_block_io *this,
			      holy_efi_boolean_t extended_verification);
  holy_efi_status_t (*read_blocks) (struct holy_efi_block_io *this,
				    holy_efi_uint32_t media_id,
				    holy_efi_lba_t lba,
				    holy_efi_uintn_t buffer_size,
				    void *buffer);
  holy_efi_status_t (*write_blocks) (struct holy_efi_block_io *this,
				     holy_efi_uint32_t media_id,
				     holy_efi_lba_t lba,
				     holy_efi_uintn_t buffer_size,
				     void *buffer);
  holy_efi_status_t (*flush_blocks) (struct holy_efi_block_io *this);
};
typedef struct holy_efi_block_io holy_efi_block_io_t;

#if (holy_TARGET_SIZEOF_VOID_P == 4) || defined (__ia64__) \
  || defined (__aarch64__) || defined (__MINGW64__) || defined (__CYGWIN__)

#define efi_call_0(func)		func()
#define efi_call_1(func, a)		func(a)
#define efi_call_2(func, a, b)		func(a, b)
#define efi_call_3(func, a, b, c)	func(a, b, c)
#define efi_call_4(func, a, b, c, d)	func(a, b, c, d)
#define efi_call_5(func, a, b, c, d, e)	func(a, b, c, d, e)
#define efi_call_6(func, a, b, c, d, e, f) func(a, b, c, d, e, f)
#define efi_call_7(func, a, b, c, d, e, f, g) func(a, b, c, d, e, f, g)
#define efi_call_10(func, a, b, c, d, e, f, g, h, i, j)	func(a, b, c, d, e, f, g, h, i, j)

#else

#define efi_call_0(func) \
  efi_wrap_0(func)
#define efi_call_1(func, a) \
  efi_wrap_1(func, (holy_uint64_t) (a))
#define efi_call_2(func, a, b) \
  efi_wrap_2(func, (holy_uint64_t) (a), (holy_uint64_t) (b))
#define efi_call_3(func, a, b, c) \
  efi_wrap_3(func, (holy_uint64_t) (a), (holy_uint64_t) (b), \
	     (holy_uint64_t) (c))
#define efi_call_4(func, a, b, c, d) \
  efi_wrap_4(func, (holy_uint64_t) (a), (holy_uint64_t) (b), \
	     (holy_uint64_t) (c), (holy_uint64_t) (d))
#define efi_call_5(func, a, b, c, d, e)	\
  efi_wrap_5(func, (holy_uint64_t) (a), (holy_uint64_t) (b), \
	     (holy_uint64_t) (c), (holy_uint64_t) (d), (holy_uint64_t) (e))
#define efi_call_6(func, a, b, c, d, e, f) \
  efi_wrap_6(func, (holy_uint64_t) (a), (holy_uint64_t) (b), \
	     (holy_uint64_t) (c), (holy_uint64_t) (d), (holy_uint64_t) (e), \
	     (holy_uint64_t) (f))
#define efi_call_7(func, a, b, c, d, e, f, g) \
  efi_wrap_7(func, (holy_uint64_t) (a), (holy_uint64_t) (b), \
	     (holy_uint64_t) (c), (holy_uint64_t) (d), (holy_uint64_t) (e), \
	     (holy_uint64_t) (f), (holy_uint64_t) (g))
#define efi_call_10(func, a, b, c, d, e, f, g, h, i, j) \
  efi_wrap_10(func, (holy_uint64_t) (a), (holy_uint64_t) (b), \
	      (holy_uint64_t) (c), (holy_uint64_t) (d), (holy_uint64_t) (e), \
	      (holy_uint64_t) (f), (holy_uint64_t) (g),	(holy_uint64_t) (h), \
	      (holy_uint64_t) (i), (holy_uint64_t) (j))

holy_uint64_t EXPORT_FUNC(efi_wrap_0) (void *func);
holy_uint64_t EXPORT_FUNC(efi_wrap_1) (void *func, holy_uint64_t arg1);
holy_uint64_t EXPORT_FUNC(efi_wrap_2) (void *func, holy_uint64_t arg1,
                                       holy_uint64_t arg2);
holy_uint64_t EXPORT_FUNC(efi_wrap_3) (void *func, holy_uint64_t arg1,
                                       holy_uint64_t arg2, holy_uint64_t arg3);
holy_uint64_t EXPORT_FUNC(efi_wrap_4) (void *func, holy_uint64_t arg1,
                                       holy_uint64_t arg2, holy_uint64_t arg3,
                                       holy_uint64_t arg4);
holy_uint64_t EXPORT_FUNC(efi_wrap_5) (void *func, holy_uint64_t arg1,
                                       holy_uint64_t arg2, holy_uint64_t arg3,
                                       holy_uint64_t arg4, holy_uint64_t arg5);
holy_uint64_t EXPORT_FUNC(efi_wrap_6) (void *func, holy_uint64_t arg1,
                                       holy_uint64_t arg2, holy_uint64_t arg3,
                                       holy_uint64_t arg4, holy_uint64_t arg5,
                                       holy_uint64_t arg6);
holy_uint64_t EXPORT_FUNC(efi_wrap_7) (void *func, holy_uint64_t arg1,
                                       holy_uint64_t arg2, holy_uint64_t arg3,
                                       holy_uint64_t arg4, holy_uint64_t arg5,
                                       holy_uint64_t arg6, holy_uint64_t arg7);
holy_uint64_t EXPORT_FUNC(efi_wrap_10) (void *func, holy_uint64_t arg1,
                                        holy_uint64_t arg2, holy_uint64_t arg3,
                                        holy_uint64_t arg4, holy_uint64_t arg5,
                                        holy_uint64_t arg6, holy_uint64_t arg7,
                                        holy_uint64_t arg8, holy_uint64_t arg9,
                                        holy_uint64_t arg10);
#endif

#endif /* ! holy_EFI_API_HEADER */
