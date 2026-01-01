/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EFI_EFI_HEADER
#define holy_EFI_EFI_HEADER	1

#include <holy/types.h>
#include <holy/dl.h>
#include <holy/efi/api.h>

/* Functions.  */
void *EXPORT_FUNC(holy_efi_locate_protocol) (holy_efi_guid_t *protocol,
					     void *registration);
holy_efi_handle_t *
EXPORT_FUNC(holy_efi_locate_handle) (holy_efi_locate_search_type_t search_type,
				     holy_efi_guid_t *protocol,
				     void *search_key,
				     holy_efi_uintn_t *num_handles);
void *EXPORT_FUNC(holy_efi_open_protocol) (holy_efi_handle_t handle,
					   holy_efi_guid_t *protocol,
					   holy_efi_uint32_t attributes);
int EXPORT_FUNC(holy_efi_set_text_mode) (int on);
void EXPORT_FUNC(holy_efi_stall) (holy_efi_uintn_t microseconds);
void *
EXPORT_FUNC(holy_efi_allocate_pages) (holy_efi_physical_address_t address,
				      holy_efi_uintn_t pages);
void *
EXPORT_FUNC(holy_efi_allocate_pages_max) (holy_efi_physical_address_t max,
					  holy_efi_uintn_t pages);
void EXPORT_FUNC(holy_efi_free_pages) (holy_efi_physical_address_t address,
				       holy_efi_uintn_t pages);
int
EXPORT_FUNC(holy_efi_get_memory_map) (holy_efi_uintn_t *memory_map_size,
				      holy_efi_memory_descriptor_t *memory_map,
				      holy_efi_uintn_t *map_key,
				      holy_efi_uintn_t *descriptor_size,
				      holy_efi_uint32_t *descriptor_version);
holy_efi_loaded_image_t *EXPORT_FUNC(holy_efi_get_loaded_image) (holy_efi_handle_t image_handle);
void EXPORT_FUNC(holy_efi_print_device_path) (holy_efi_device_path_t *dp);
char *EXPORT_FUNC(holy_efi_get_filename) (holy_efi_device_path_t *dp);
holy_efi_device_path_t *
EXPORT_FUNC(holy_efi_get_device_path) (holy_efi_handle_t handle);
holy_efi_device_path_t *
EXPORT_FUNC(holy_efi_find_last_device_path) (const holy_efi_device_path_t *dp);
holy_efi_device_path_t *
EXPORT_FUNC(holy_efi_duplicate_device_path) (const holy_efi_device_path_t *dp);
holy_err_t EXPORT_FUNC (holy_efi_finish_boot_services) (holy_efi_uintn_t *outbuf_size, void *outbuf,
							holy_efi_uintn_t *map_key,
							holy_efi_uintn_t *efi_desc_size,
							holy_efi_uint32_t *efi_desc_version);
holy_err_t EXPORT_FUNC (holy_efi_set_virtual_address_map) (holy_efi_uintn_t memory_map_size,
							   holy_efi_uintn_t descriptor_size,
							   holy_efi_uint32_t descriptor_version,
							   holy_efi_memory_descriptor_t *virtual_map);
void *EXPORT_FUNC (holy_efi_get_variable) (const char *variable,
					   const holy_efi_guid_t *guid,
					   holy_size_t *datasize_out);
holy_err_t
EXPORT_FUNC (holy_efi_set_variable) (const char *var,
				     const holy_efi_guid_t *guid,
				     void *data,
				     holy_size_t datasize);
holy_efi_boolean_t EXPORT_FUNC (holy_efi_secure_boot) (void);
int
EXPORT_FUNC (holy_efi_compare_device_paths) (const holy_efi_device_path_t *dp1,
					     const holy_efi_device_path_t *dp2);

extern void (*EXPORT_VAR(holy_efi_net_config)) (holy_efi_handle_t hnd,
						char **device,
						char **path);

#if defined(__arm__) || defined(__aarch64__)
void *EXPORT_FUNC(holy_efi_get_firmware_fdt)(void);
#endif

holy_addr_t holy_efi_modules_addr (void);

void holy_efi_mm_init (void);
void holy_efi_mm_fini (void);
void holy_efi_init (void);
void holy_efi_fini (void);
void holy_efi_set_prefix (void);

/* Variables.  */
extern holy_efi_system_table_t *EXPORT_VAR(holy_efi_system_table);
extern holy_efi_handle_t EXPORT_VAR(holy_efi_image_handle);

extern int EXPORT_VAR(holy_efi_is_finished);

struct holy_net_card;

holy_efi_handle_t
holy_efinet_get_device_handle (struct holy_net_card *card);

#endif /* ! holy_EFI_EFI_HEADER */
