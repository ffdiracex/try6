/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EFI_PCI_HEADER
#define holy_EFI_PCI_HEADER	1

#include <holy/symbol.h>

#define holy_EFI_PCI_IO_GUID \
  { 0x4cf5b200, 0x68b8, 0x4ca5, { 0x9e, 0xec, 0xb2, 0x3e, 0x3f, 0x50, 0x02, 0x9a }}

#define holy_EFI_PCI_ROOT_IO_GUID \
  { 0x2F707EBB, 0x4A1A, 0x11d4, { 0x9A, 0x38, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D }}

typedef enum
  {
    holy_EFI_PCI_IO_WIDTH_UINT8,
    holy_EFI_PCI_IO_WIDTH_UINT16,
    holy_EFI_PCI_IO_WIDTH_UINT32,
    holy_EFI_PCI_IO_WIDTH_UINT64,
    holy_EFI_PCI_IO_WIDTH_FIFO_UINT8,
    holy_EFI_PCI_IO_WIDTH_FIFO_UINT16,
    holy_EFI_PCI_IO_WIDTH_FIFO_UINT32,
    holy_EFI_PCI_IO_WIDTH_FIFO_UINT64,
    holy_EFI_PCI_IO_WIDTH_FILL_UINT8,
    holy_EFI_PCI_IO_WIDTH_FILL_UINT16,
    holy_EFI_PCI_IO_WIDTH_FILL_UINT32,
    holy_EFI_PCI_IO_WIDTH_FILL_UINT64,
    holy_EFI_PCI_IO_WIDTH_MAXIMUM,
  }
  holy_efi_pci_io_width_t;

struct holy_efi_pci_io;

typedef holy_efi_status_t
(*holy_efi_pci_io_mem_t) (struct holy_efi_pci_io *this,
			  holy_efi_pci_io_width_t width,
			  holy_efi_uint8_t bar_index,
			  holy_efi_uint64_t offset,
			  holy_efi_uintn_t count,
			  void *buffer);

typedef holy_efi_status_t
(*holy_efi_pci_io_config_t) (struct holy_efi_pci_io *this,
			     holy_efi_pci_io_width_t width,
			     holy_efi_uint32_t offset,
			     holy_efi_uintn_t count,
			     void *buffer);
typedef struct
{
  holy_efi_pci_io_mem_t read;
  holy_efi_pci_io_mem_t write;
} holy_efi_pci_io_access_t;

typedef struct
{
  holy_efi_pci_io_config_t read;
  holy_efi_pci_io_config_t write;
} holy_efi_pci_io_config_access_t;

typedef enum
  {
    holy_EFI_PCI_IO_OPERATION_BUS_MASTER_READ,
    holy_EFI_PCI_IO_OPERATION_BUS_MASTER_WRITE,
    holy_EFI_PCI_IO_OPERATION_BUS_MASTER_COMMON_BUFFER,
    holy_EFI_PCI_IO_OPERATION_BUS_MASTER_MAXIMUM
  }
  holy_efi_pci_io_operation_t;

#define holy_EFI_PCI_IO_ATTRIBUTE_ISA_IO               0x0002
#define holy_EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO       0x0004
#define holy_EFI_PCI_IO_ATTRIBUTE_VGA_MEMORY           0x0008
#define holy_EFI_PCI_IO_ATTRIBUTE_VGA_IO               0x0010
#define holy_EFI_PCI_IO_ATTRIBUTE_IDE_PRIMARY_IO       0x0020
#define holy_EFI_PCI_IO_ATTRIBUTE_IDE_SECONDARY_IO     0x0040
#define holy_EFI_PCI_IO_ATTRIBUTE_MEMORY_WRITE_COMBINE 0x0080
#define holy_EFI_PCI_IO_ATTRIBUTE_IO                   0x0100
#define holy_EFI_PCI_IO_ATTRIBUTE_MEMORY               0x0200
#define holy_EFI_PCI_IO_ATTRIBUTE_BUS_MASTER           0x0400
#define holy_EFI_PCI_IO_ATTRIBUTE_MEMORY_CACHED        0x0800
#define holy_EFI_PCI_IO_ATTRIBUTE_MEMORY_DISABLE       0x1000
#define holy_EFI_PCI_IO_ATTRIBUTE_EMBEDDED_DEVICE      0x2000
#define holy_EFI_PCI_IO_ATTRIBUTE_EMBEDDED_ROM         0x4000
#define holy_EFI_PCI_IO_ATTRIBUTE_DUAL_ADDRESS_CYCLE   0x8000
#define holy_EFI_PCI_IO_ATTRIBUTE_ISA_IO_16            0x10000
#define holy_EFI_PCI_IO_ATTRIBUTE_VGA_PALETTE_IO_16    0x20000
#define holy_EFI_PCI_IO_ATTRIBUTE_VGA_IO_16            0x40000

typedef enum
  {
    holy_EFI_PCI_IO_ATTRIBUTE_OPERATION_GET,
    holy_EFI_PCI_IO_ATTRIBUTE_OPERATION_SET,
    holy_EFI_PCI_IO_ATTRIBUTE_OPERATION_ENABLE,
    holy_EFI_PCI_IO_ATTRIBUTE_OPERATION_DISABLE,
    holy_EFI_PCI_IO_ATTRIBUTE_OPERATION_SUPPORTED,
    holy_EFI_PCI_IO_ATTRIBUTE_OPERATION_MAXIMUM
  }
  holy_efi_pci_io_attribute_operation_t;

typedef holy_efi_status_t
(*holy_efi_pci_io_poll_io_mem_t) (struct holy_efi_pci_io *this,
				  holy_efi_pci_io_width_t  width,
				  holy_efi_uint8_t bar_ndex,
				  holy_efi_uint64_t offset,
				  holy_efi_uint64_t mask,
				  holy_efi_uint64_t value,
				  holy_efi_uint64_t delay,
				  holy_efi_uint64_t *result);

typedef holy_efi_status_t
(*holy_efi_pci_io_copy_mem_t) (struct holy_efi_pci_io *this,
			       holy_efi_pci_io_width_t width,
			       holy_efi_uint8_t dest_bar_index,
			       holy_efi_uint64_t dest_offset,
			       holy_efi_uint8_t src_bar_index,
			       holy_efi_uint64_t src_offset,
			       holy_efi_uintn_t count);

typedef holy_efi_status_t
(*holy_efi_pci_io_map_t) (struct holy_efi_pci_io *this,
			  holy_efi_pci_io_operation_t operation,
			  void *host_address,
			  holy_efi_uintn_t *number_of_bytes,
			  holy_efi_uint64_t *device_address,
			  void **mapping);

typedef holy_efi_status_t
(*holy_efi_pci_io_unmap_t) (struct holy_efi_pci_io *this,
			    void *mapping);

typedef holy_efi_status_t
(*holy_efi_pci_io_allocate_buffer_t) (struct holy_efi_pci_io *this,
				      holy_efi_allocate_type_t type,
				      holy_efi_memory_type_t memory_type,
				      holy_efi_uintn_t pages,
				      void **host_address,
				      holy_efi_uint64_t attributes);

typedef holy_efi_status_t
(*holy_efi_pci_io_free_buffer_t) (struct holy_efi_pci_io *this,
				  holy_efi_allocate_type_t type,
				  holy_efi_memory_type_t memory_type,
				  holy_efi_uintn_t pages,
				  void **host_address,
				  holy_efi_uint64_t attributes);

typedef holy_efi_status_t
(*holy_efi_pci_io_flush_t) (struct holy_efi_pci_io *this);

typedef holy_efi_status_t
(*holy_efi_pci_io_get_location_t) (struct holy_efi_pci_io *this,
				   holy_efi_uintn_t *segment_number,
				   holy_efi_uintn_t *bus_number,
				   holy_efi_uintn_t *device_number,
				   holy_efi_uintn_t *function_number);

typedef holy_efi_status_t
(*holy_efi_pci_io_attributes_t) (struct holy_efi_pci_io *this,
				 holy_efi_pci_io_attribute_operation_t operation,
				 holy_efi_uint64_t attributes,
				 holy_efi_uint64_t *result);

typedef holy_efi_status_t
(*holy_efi_pci_io_get_bar_attributes_t) (struct holy_efi_pci_io *this,
					 holy_efi_uint8_t bar_index,
					 holy_efi_uint64_t *supports,
					 void **resources);

typedef holy_efi_status_t
(*holy_efi_pci_io_set_bar_attributes_t) (struct holy_efi_pci_io *this,
					 holy_efi_uint64_t attributes,
					 holy_efi_uint8_t bar_index,
					 holy_efi_uint64_t *offset,
					 holy_efi_uint64_t *length);
struct holy_efi_pci_io {
  holy_efi_pci_io_poll_io_mem_t poll_mem;
  holy_efi_pci_io_poll_io_mem_t poll_io;
  holy_efi_pci_io_access_t mem;
  holy_efi_pci_io_access_t io;
  holy_efi_pci_io_config_access_t pci;
  holy_efi_pci_io_copy_mem_t copy_mem;
  holy_efi_pci_io_map_t map;
  holy_efi_pci_io_unmap_t unmap;
  holy_efi_pci_io_allocate_buffer_t allocate_buffer;
  holy_efi_pci_io_free_buffer_t free_buffer;
  holy_efi_pci_io_flush_t flush;
  holy_efi_pci_io_get_location_t get_location;
  holy_efi_pci_io_attributes_t attributes;
  holy_efi_pci_io_get_bar_attributes_t get_bar_attributes;
  holy_efi_pci_io_set_bar_attributes_t set_bar_attributes;
  holy_efi_uint64_t rom_size;
  void *rom_image;
};
typedef struct holy_efi_pci_io holy_efi_pci_io_t;

struct holy_efi_pci_root_io;

typedef struct
{
  holy_efi_status_t(*read) (struct holy_efi_pci_root_io *this,
			    holy_efi_pci_io_width_t width,
			    holy_efi_uint64_t address,
			    holy_efi_uintn_t count,
			    void *buffer);
  holy_efi_status_t(*write) (struct holy_efi_pci_root_io *this,
			     holy_efi_pci_io_width_t width,
			     holy_efi_uint64_t address,
			     holy_efi_uintn_t count,
			     void *buffer);
} holy_efi_pci_root_io_access_t;

typedef enum
  {
    holy_EFI_PCI_ROOT_IO_OPERATION_BUS_MASTER_READ,
    holy_EFI_PCI_ROOT_IO_OPERATION_BUS_MASTER_WRITE,
    holy_EFI_PCI_ROOT_IO_OPERATION_BUS_MASTER_COMMON_BUFFER,
    holy_EFI_PCI_ROOT_IO_OPERATION_BUS_MASTER_READ_64,
    holy_EFI_PCI_ROOT_IO_OPERATION_BUS_MASTER_WRITE_64,
    holy_EFI_PCI_ROOT_IO_OPERATION_BUS_MASTER_COMMON_BUFFER_64,
    holy_EFI_PCI_ROOT_IO_OPERATION_BUS_MASTER_MAXIMUM
  }
  holy_efi_pci_root_io_operation_t;

typedef holy_efi_status_t
(*holy_efi_pci_root_io_poll_io_mem_t) (struct holy_efi_pci_root_io *this,
				       holy_efi_pci_io_width_t  width,
				       holy_efi_uint64_t address,
				       holy_efi_uint64_t mask,
				       holy_efi_uint64_t value,
				       holy_efi_uint64_t delay,
				       holy_efi_uint64_t *result);

typedef holy_efi_status_t
(*holy_efi_pci_root_io_copy_mem_t) (struct holy_efi_pci_root_io *this,
				    holy_efi_pci_io_width_t width,
				    holy_efi_uint64_t dest_offset,
				    holy_efi_uint64_t src_offset,
				    holy_efi_uintn_t count);


typedef holy_efi_status_t
(*holy_efi_pci_root_io_map_t) (struct holy_efi_pci_root_io *this,
				holy_efi_pci_root_io_operation_t operation,
			       void *host_address,
			       holy_efi_uintn_t *number_of_bytes,
			       holy_efi_uint64_t *device_address,
			       void **mapping);

typedef holy_efi_status_t
(*holy_efi_pci_root_io_unmap_t) (struct holy_efi_pci_root_io *this,
				 void *mapping);

typedef holy_efi_status_t
(*holy_efi_pci_root_io_allocate_buffer_t) (struct holy_efi_pci_root_io *this,
					   holy_efi_allocate_type_t type,
					   holy_efi_memory_type_t memory_type,
					   holy_efi_uintn_t pages,
					   void **host_address,
					   holy_efi_uint64_t attributes);

typedef holy_efi_status_t
(*holy_efi_pci_root_io_free_buffer_t) (struct holy_efi_pci_root_io *this,
				       holy_efi_uintn_t pages,
				       void **host_address);

typedef holy_efi_status_t
(*holy_efi_pci_root_io_flush_t) (struct holy_efi_pci_root_io *this);

typedef holy_efi_status_t
(*holy_efi_pci_root_io_get_attributes_t) (struct holy_efi_pci_root_io *this,
					  holy_efi_uint64_t *supports,
					  void **resources);

typedef holy_efi_status_t
(*holy_efi_pci_root_io_set_attributes_t) (struct holy_efi_pci_root_io *this,
					  holy_efi_uint64_t attributes,
					  holy_efi_uint64_t *offset,
					  holy_efi_uint64_t *length);

typedef holy_efi_status_t
(*holy_efi_pci_root_io_configuration_t) (struct holy_efi_pci_root_io *this,
					 void **resources);

struct holy_efi_pci_root_io {
  holy_efi_handle_t parent;
  holy_efi_pci_root_io_poll_io_mem_t poll_mem;
  holy_efi_pci_root_io_poll_io_mem_t poll_io;
  holy_efi_pci_root_io_access_t mem;
  holy_efi_pci_root_io_access_t io;
  holy_efi_pci_root_io_access_t pci;
  holy_efi_pci_root_io_copy_mem_t copy_mem;
  holy_efi_pci_root_io_map_t map;
  holy_efi_pci_root_io_unmap_t unmap;
  holy_efi_pci_root_io_allocate_buffer_t allocate_buffer;
  holy_efi_pci_root_io_free_buffer_t free_buffer;
  holy_efi_pci_root_io_flush_t flush;
  holy_efi_pci_root_io_get_attributes_t get_attributes;
  holy_efi_pci_root_io_set_attributes_t set_attributes;
  holy_efi_pci_root_io_configuration_t configuration;
};

typedef struct holy_efi_pci_root_io holy_efi_pci_root_io_t;

#endif  /* !holy_EFI_PCI_HEADER */
