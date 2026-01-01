/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_ACPI_HEADER
#define holy_ACPI_HEADER	1

#ifndef holy_DSDT_TEST
#include <holy/types.h>
#include <holy/err.h>
#else
#define holy_PACKED __attribute__ ((packed))
#endif

#define holy_RSDP_SIGNATURE "RSD PTR "
#define holy_RSDP_SIGNATURE_SIZE 8

struct holy_acpi_rsdp_v10
{
  holy_uint8_t signature[holy_RSDP_SIGNATURE_SIZE];
  holy_uint8_t checksum;
  holy_uint8_t oemid[6];
  holy_uint8_t revision;
  holy_uint32_t rsdt_addr;
} holy_PACKED;

struct holy_acpi_rsdp_v20
{
  struct holy_acpi_rsdp_v10 rsdpv1;
  holy_uint32_t length;
  holy_uint64_t xsdt_addr;
  holy_uint8_t checksum;
  holy_uint8_t reserved[3];
} holy_PACKED;

struct holy_acpi_table_header
{
  holy_uint8_t signature[4];
  holy_uint32_t length;
  holy_uint8_t revision;
  holy_uint8_t checksum;
  holy_uint8_t oemid[6];
  holy_uint8_t oemtable[8];
  holy_uint32_t oemrev;
  holy_uint8_t creator_id[4];
  holy_uint32_t creator_rev;
} holy_PACKED;

#define holy_ACPI_FADT_SIGNATURE "FACP"

struct holy_acpi_fadt
{
  struct holy_acpi_table_header hdr;
  holy_uint32_t facs_addr;
  holy_uint32_t dsdt_addr;
  holy_uint8_t somefields1[20];
  holy_uint32_t pm1a;
  holy_uint8_t somefields2[8];
  holy_uint32_t pmtimer;
  holy_uint8_t somefields3[32];
  holy_uint32_t flags;
  holy_uint8_t somefields4[16];
  holy_uint64_t facs_xaddr;
  holy_uint64_t dsdt_xaddr;
  holy_uint8_t somefields5[96];
} holy_PACKED;

#define holy_ACPI_MADT_SIGNATURE "APIC"

struct holy_acpi_madt_entry_header
{
  holy_uint8_t type;
  holy_uint8_t len;
};

struct holy_acpi_madt
{
  struct holy_acpi_table_header hdr;
  holy_uint32_t lapic_addr;
  holy_uint32_t flags;
  struct holy_acpi_madt_entry_header entries[0];
};

enum
  {
    holy_ACPI_MADT_ENTRY_TYPE_LAPIC = 0,
    holy_ACPI_MADT_ENTRY_TYPE_IOAPIC = 1,
    holy_ACPI_MADT_ENTRY_TYPE_INTERRUPT_OVERRIDE = 2,
    holy_ACPI_MADT_ENTRY_TYPE_LAPIC_NMI = 4,
    holy_ACPI_MADT_ENTRY_TYPE_SAPIC = 6,
    holy_ACPI_MADT_ENTRY_TYPE_LSAPIC = 7,
    holy_ACPI_MADT_ENTRY_TYPE_PLATFORM_INT_SOURCE = 8
  };

struct holy_acpi_madt_entry_lapic
{
  struct holy_acpi_madt_entry_header hdr;
  holy_uint8_t acpiid;
  holy_uint8_t apicid;
  holy_uint32_t flags;
};

struct holy_acpi_madt_entry_ioapic
{
  struct holy_acpi_madt_entry_header hdr;
  holy_uint8_t id;
  holy_uint8_t pad;
  holy_uint32_t address;
  holy_uint32_t global_sys_interrupt;
};

struct holy_acpi_madt_entry_interrupt_override
{
  struct holy_acpi_madt_entry_header hdr;
  holy_uint8_t bus;
  holy_uint8_t source;
  holy_uint32_t global_sys_interrupt;
  holy_uint16_t flags;
} holy_PACKED;


struct holy_acpi_madt_entry_lapic_nmi
{
  struct holy_acpi_madt_entry_header hdr;
  holy_uint8_t acpiid;
  holy_uint16_t flags;
  holy_uint8_t lint;
} holy_PACKED;

struct holy_acpi_madt_entry_sapic
{
  struct holy_acpi_madt_entry_header hdr;
  holy_uint8_t id;
  holy_uint8_t pad;
  holy_uint32_t global_sys_interrupt_base;
  holy_uint64_t addr;
};

struct holy_acpi_madt_entry_lsapic
{
  struct holy_acpi_madt_entry_header hdr;
  holy_uint8_t cpu_id;
  holy_uint8_t id;
  holy_uint8_t eid;
  holy_uint8_t pad[3];
  holy_uint32_t flags;
  holy_uint32_t cpu_uid;
  holy_uint8_t cpu_uid_str[0];
};

struct holy_acpi_madt_entry_platform_int_source
{
  struct holy_acpi_madt_entry_header hdr;
  holy_uint16_t flags;
  holy_uint8_t inttype;
  holy_uint8_t cpu_id;
  holy_uint8_t cpu_eid;
  holy_uint8_t sapic_vector;
  holy_uint32_t global_sys_int;
  holy_uint32_t src_flags;
};

enum
  {
    holy_ACPI_MADT_ENTRY_SAPIC_FLAGS_ENABLED = 1
  };

#ifndef holy_DSDT_TEST
struct holy_acpi_rsdp_v10 *holy_acpi_get_rsdpv1 (void);
struct holy_acpi_rsdp_v20 *holy_acpi_get_rsdpv2 (void);
struct holy_acpi_rsdp_v10 *EXPORT_FUNC(holy_machine_acpi_get_rsdpv1) (void);
struct holy_acpi_rsdp_v20 *EXPORT_FUNC(holy_machine_acpi_get_rsdpv2) (void);
holy_uint8_t EXPORT_FUNC(holy_byte_checksum) (void *base, holy_size_t size);

holy_err_t holy_acpi_create_ebda (void);

void holy_acpi_halt (void);
#endif

#define holy_ACPI_SLP_EN (1 << 13)
#define holy_ACPI_SLP_TYP_OFFSET 10

enum
  {
    holy_ACPI_OPCODE_ZERO = 0, holy_ACPI_OPCODE_ONE = 1,
    holy_ACPI_OPCODE_NAME = 8, holy_ACPI_OPCODE_ALIAS = 0x06,
    holy_ACPI_OPCODE_BYTE_CONST = 0x0a,
    holy_ACPI_OPCODE_WORD_CONST = 0x0b,
    holy_ACPI_OPCODE_DWORD_CONST = 0x0c,
    holy_ACPI_OPCODE_STRING_CONST = 0x0d,
    holy_ACPI_OPCODE_SCOPE = 0x10,
    holy_ACPI_OPCODE_BUFFER = 0x11,
    holy_ACPI_OPCODE_PACKAGE = 0x12,
    holy_ACPI_OPCODE_METHOD = 0x14, holy_ACPI_OPCODE_EXTOP = 0x5b,
    holy_ACPI_OPCODE_ADD = 0x72,
    holy_ACPI_OPCODE_CONCAT = 0x73,
    holy_ACPI_OPCODE_SUBTRACT = 0x74,
    holy_ACPI_OPCODE_MULTIPLY = 0x77,
    holy_ACPI_OPCODE_DIVIDE = 0x78,
    holy_ACPI_OPCODE_LSHIFT = 0x79,
    holy_ACPI_OPCODE_RSHIFT = 0x7a,
    holy_ACPI_OPCODE_AND = 0x7b,
    holy_ACPI_OPCODE_NAND = 0x7c,
    holy_ACPI_OPCODE_OR = 0x7d,
    holy_ACPI_OPCODE_NOR = 0x7e,
    holy_ACPI_OPCODE_XOR = 0x7f,
    holy_ACPI_OPCODE_CONCATRES = 0x84,
    holy_ACPI_OPCODE_MOD = 0x85,
    holy_ACPI_OPCODE_INDEX = 0x88,
    holy_ACPI_OPCODE_CREATE_DWORD_FIELD = 0x8a,
    holy_ACPI_OPCODE_CREATE_WORD_FIELD = 0x8b,
    holy_ACPI_OPCODE_CREATE_BYTE_FIELD = 0x8c,
    holy_ACPI_OPCODE_TOSTRING = 0x9c,
    holy_ACPI_OPCODE_IF = 0xa0, holy_ACPI_OPCODE_ONES = 0xff
  };
enum
  {
    holy_ACPI_EXTOPCODE_MUTEX = 0x01,
    holy_ACPI_EXTOPCODE_EVENT_OP = 0x02,
    holy_ACPI_EXTOPCODE_OPERATION_REGION = 0x80,
    holy_ACPI_EXTOPCODE_FIELD_OP = 0x81,
    holy_ACPI_EXTOPCODE_DEVICE_OP = 0x82,
    holy_ACPI_EXTOPCODE_PROCESSOR_OP = 0x83,
    holy_ACPI_EXTOPCODE_POWER_RES_OP = 0x84,
    holy_ACPI_EXTOPCODE_THERMAL_ZONE_OP = 0x85,
    holy_ACPI_EXTOPCODE_INDEX_FIELD_OP = 0x86,
    holy_ACPI_EXTOPCODE_BANK_FIELD_OP = 0x87,
  };

struct holy_acpi_fadt *
EXPORT_FUNC(holy_acpi_find_fadt) (void);

#endif /* ! holy_ACPI_HEADER */
