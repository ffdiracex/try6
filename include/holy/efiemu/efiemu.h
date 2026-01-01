/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EFI_EMU_HEADER
#define holy_EFI_EMU_HEADER	1

#include <holy/efi/api.h>
#include <holy/file.h>
#include <holy/memory.h>

#define holy_EFIEMU_PAGESIZE 4096

/* EFI api defined in 32-bit and 64-bit version*/
struct holy_efi_system_table32
{
  holy_efi_table_header_t hdr;
  holy_efi_uint32_t firmware_vendor;
  holy_efi_uint32_t firmware_revision;
  holy_efi_uint32_t console_in_handler;
  holy_efi_uint32_t con_in;
  holy_efi_uint32_t console_out_handler;
  holy_efi_uint32_t con_out;
  holy_efi_uint32_t standard_error_handle;
  holy_efi_uint32_t std_err;
  holy_efi_uint32_t runtime_services;
  holy_efi_uint32_t boot_services;
  holy_efi_uint32_t num_table_entries;
  holy_efi_uint32_t configuration_table;
} holy_PACKED;
typedef struct holy_efi_system_table32  holy_efi_system_table32_t;

struct holy_efi_system_table64
{
  holy_efi_table_header_t hdr;
  holy_efi_uint64_t firmware_vendor;
  holy_efi_uint32_t firmware_revision;
  holy_efi_uint32_t pad;
  holy_efi_uint64_t console_in_handler;
  holy_efi_uint64_t con_in;
  holy_efi_uint64_t console_out_handler;
  holy_efi_uint64_t con_out;
  holy_efi_uint64_t standard_error_handle;
  holy_efi_uint64_t std_err;
  holy_efi_uint64_t runtime_services;
  holy_efi_uint64_t boot_services;
  holy_efi_uint64_t num_table_entries;
  holy_efi_uint64_t configuration_table;
} holy_PACKED;
typedef struct holy_efi_system_table64  holy_efi_system_table64_t;

struct holy_efiemu_runtime_services32
{
  holy_efi_table_header_t hdr;
  holy_efi_uint32_t get_time;
  holy_efi_uint32_t set_time;
  holy_efi_uint32_t get_wakeup_time;
  holy_efi_uint32_t set_wakeup_time;
  holy_efi_uint32_t set_virtual_address_map;
  holy_efi_uint32_t convert_pointer;
  holy_efi_uint32_t get_variable;
  holy_efi_uint32_t get_next_variable_name;
  holy_efi_uint32_t set_variable;
  holy_efi_uint32_t get_next_high_monotonic_count;
  holy_efi_uint32_t reset_system;
} holy_PACKED;
typedef struct holy_efiemu_runtime_services32 holy_efiemu_runtime_services32_t;

struct holy_efiemu_runtime_services64
{
  holy_efi_table_header_t hdr;
  holy_efi_uint64_t get_time;
  holy_efi_uint64_t set_time;
  holy_efi_uint64_t get_wakeup_time;
  holy_efi_uint64_t set_wakeup_time;
  holy_efi_uint64_t set_virtual_address_map;
  holy_efi_uint64_t convert_pointer;
  holy_efi_uint64_t get_variable;
  holy_efi_uint64_t get_next_variable_name;
  holy_efi_uint64_t set_variable;
  holy_efi_uint64_t get_next_high_monotonic_count;
  holy_efi_uint64_t reset_system;
} holy_PACKED;
typedef struct holy_efiemu_runtime_services64 holy_efiemu_runtime_services64_t;

extern holy_efi_system_table32_t *holy_efiemu_system_table32;
extern holy_efi_system_table64_t *holy_efiemu_system_table64;

/* Convenience macros to access currently loaded efiemu */
#define holy_efiemu_system_table ((holy_efiemu_sizeof_uintn_t () == 8) \
				  ? (void *) holy_efiemu_system_table64 \
				  : (void *) holy_efiemu_system_table32)
#define holy_EFIEMU_SIZEOF_OF_UINTN (holy_efiemu_sizeof_uintn_t ())
#define holy_EFIEMU_SYSTEM_TABLE(x) ((holy_efiemu_sizeof_uintn_t () == 8) \
				     ? holy_efiemu_system_table64->x \
				     : holy_efiemu_system_table32->x)
#define holy_EFIEMU_SYSTEM_TABLE_SET(x,y) ((holy_efiemu_sizeof_uintn_t () == 8)\
					   ? (holy_efiemu_system_table64->x \
					      = (y)) \
					   : (holy_efiemu_system_table32->x \
					      = (y)))
#define holy_EFIEMU_SYSTEM_TABLE_PTR(x) ((holy_efiemu_sizeof_uintn_t () == 8)\
					 ? (void *) (holy_addr_t)	\
					 (holy_efiemu_system_table64->x) \
					 : (void *) (holy_addr_t) \
					 (holy_efiemu_system_table32->x))
#define holy_EFIEMU_SYSTEM_TABLE_VAR(x) ((holy_efiemu_sizeof_uintn_t () == 8) \
					 ? (void *) \
					 &(holy_efiemu_system_table64->x) \
					 : (void *) \
					 &(holy_efiemu_system_table32->x))
#define holy_EFIEMU_SYSTEM_TABLE_SIZEOF(x) \
  ((holy_efiemu_sizeof_uintn_t () == 8) \
   ? sizeof(holy_efiemu_system_table64->x)\
   : sizeof(holy_efiemu_system_table32->x))
#define holy_EFIEMU_SYSTEM_TABLE_SIZEOF_TOTAL ((holy_efiemu_sizeof_uintn_t () == 8) ? sizeof(*holy_efiemu_system_table64):sizeof(*holy_efiemu_system_table32))

/* ELF management definitions and functions */

struct holy_efiemu_segment
{
  struct holy_efiemu_segment *next;
  holy_size_t size;
  unsigned section;
  int handle;
  int ptv_rel_needed;
  holy_off_t off;
  void *srcptr;
};
typedef struct holy_efiemu_segment *holy_efiemu_segment_t;

struct holy_efiemu_elf_sym
{
  int handle;
  holy_off_t off;
  unsigned section;
};

int holy_efiemu_check_header32 (void *ehdr, holy_size_t size);
int holy_efiemu_check_header64 (void *ehdr, holy_size_t size);
holy_err_t holy_efiemu_loadcore_init32 (void *core,
					const char *filename,
					holy_size_t core_size,
					holy_efiemu_segment_t *segments);
holy_err_t holy_efiemu_loadcore_init64 (void *core, const char *filename,
					holy_size_t core_size,
					holy_efiemu_segment_t *segments);
holy_err_t holy_efiemu_loadcore_load32 (void *core,
					holy_size_t core_size,
					holy_efiemu_segment_t segments);
holy_err_t holy_efiemu_loadcore_load64 (void *core,
					holy_size_t core_size,
					holy_efiemu_segment_t segments);
holy_err_t holy_efiemu_loadcore_unload32 (void);
holy_err_t holy_efiemu_loadcore_unload64 (void);
holy_err_t holy_efiemu_loadcore_unload(void);
holy_err_t holy_efiemu_loadcore_init (holy_file_t file,
				      const char *filename);
holy_err_t holy_efiemu_loadcore_load (void);

/* Configuration tables manipulation. Definitions and functions */
struct holy_efiemu_configuration_table
{
  struct holy_efiemu_configuration_table *next;
  holy_efi_guid_t guid;
  void * (*get_table) (void *data);
  void (*unload) (void *data);
  void *data;
};
struct holy_efiemu_configuration_table32
{
  holy_efi_packed_guid_t vendor_guid;
  holy_efi_uint32_t vendor_table;
} holy_PACKED;
typedef struct holy_efiemu_configuration_table32 holy_efiemu_configuration_table32_t;
struct holy_efiemu_configuration_table64
{
  holy_efi_packed_guid_t vendor_guid;
  holy_efi_uint64_t vendor_table;
} holy_PACKED;
typedef struct holy_efiemu_configuration_table64 holy_efiemu_configuration_table64_t;
holy_err_t holy_efiemu_unregister_configuration_table (holy_efi_guid_t guid);
holy_err_t
holy_efiemu_register_configuration_table (holy_efi_guid_t guid,
					  void * (*get_table) (void *data),
					  void (*unload) (void *data),
					  void *data);

/* Memory management functions */
int holy_efiemu_request_memalign (holy_size_t align, holy_size_t size,
				  holy_efi_memory_type_t type);
void *holy_efiemu_mm_obtain_request (int handle);
holy_err_t holy_efiemu_mm_unload (void);
holy_err_t holy_efiemu_mm_do_alloc (void);
holy_err_t holy_efiemu_mm_init (void);
void holy_efiemu_mm_return_request (int handle);
holy_efi_memory_type_t holy_efiemu_mm_get_type (int handle);

/* Drop-in replacements for holy_efi_* and holy_machine_* */
int holy_efiemu_get_memory_map (holy_efi_uintn_t *memory_map_size,
				holy_efi_memory_descriptor_t *memory_map,
				holy_efi_uintn_t *map_key,
				holy_efi_uintn_t *descriptor_size,
				holy_efi_uint32_t *descriptor_version);


holy_err_t
holy_efiemu_finish_boot_services (holy_efi_uintn_t *memory_map_size,
				  holy_efi_memory_descriptor_t *memory_map,
				  holy_efi_uintn_t *map_key,
				  holy_efi_uintn_t *descriptor_size,
				  holy_efi_uint32_t *descriptor_version);

holy_err_t
holy_efiemu_mmap_iterate (holy_memory_hook_t hook, void *hook_data);
int holy_efiemu_sizeof_uintn_t (void);
holy_err_t
holy_efiemu_get_lower_upper_memory (holy_uint64_t *lower, holy_uint64_t *upper);

/* efiemu main control definitions and functions*/
typedef enum {holy_EFIEMU_NOTLOADED,
	      holy_EFIEMU32, holy_EFIEMU64} holy_efiemu_mode_t;
struct holy_efiemu_prepare_hook
{
  struct holy_efiemu_prepare_hook *next;
  holy_err_t (*hook) (void *data);
  void (*unload) (void *data);
  void *data;
};
holy_err_t holy_efiemu_prepare32 (struct holy_efiemu_prepare_hook
				  *prepare_hooks,
				  struct holy_efiemu_configuration_table
				  *config_tables);
holy_err_t holy_efiemu_prepare64 (struct holy_efiemu_prepare_hook
				  *prepare_hooks,
				  struct holy_efiemu_configuration_table
				  *config_tables);
holy_err_t holy_efiemu_unload (void);
holy_err_t holy_efiemu_prepare (void);
holy_err_t
holy_efiemu_register_prepare_hook (holy_err_t (*hook) (void *data),
				   void (*unload) (void *data),
				   void *data);

/* symbols and pointers */
holy_err_t holy_efiemu_alloc_syms (void);
holy_err_t holy_efiemu_request_symbols (int num);
holy_err_t holy_efiemu_resolve_symbol (const char *name,
				       int *handle, holy_off_t *off);
holy_err_t holy_efiemu_register_symbol (const char *name,
					int handle, holy_off_t off);
void holy_efiemu_free_syms (void);
holy_err_t holy_efiemu_write_value (void * addr, holy_uint32_t value,
				    int plus_handle,
				    int minus_handle, int ptv_needed, int size);
holy_err_t holy_efiemu_write_sym_markers (void);
holy_err_t holy_efiemu_pnvram (void);
const char *holy_efiemu_get_default_core_name (void);
void holy_efiemu_pnvram_cmd_unregister (void);
holy_err_t holy_efiemu_autocore (void);
holy_err_t holy_efiemu_crc32 (void);
holy_err_t holy_efiemu_crc64 (void);
holy_err_t
holy_efiemu_set_virtual_address_map (holy_efi_uintn_t memory_map_size,
				     holy_efi_uintn_t descriptor_size,
				     holy_efi_uint32_t descriptor_version
				     __attribute__ ((unused)),
				     holy_efi_memory_descriptor_t *virtual_map);

holy_err_t holy_machine_efiemu_init_tables (void);

#endif /* ! holy_EFI_EMU_HEADER */
