/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/charset.h>
#include <holy/efi/api.h>
#include <holy/efi/efi.h>
#include <holy/efi/console_control.h>
#include <holy/efi/pe32.h>
#include <holy/time.h>
#include <holy/term.h>
#include <holy/kernel.h>
#include <holy/mm.h>
#include <holy/loader.h>

/* The handle of holy itself. Filled in by the startup code.  */
holy_efi_handle_t holy_efi_image_handle;

/* The pointer to a system table. Filled in by the startup code.  */
holy_efi_system_table_t *holy_efi_system_table;

static holy_efi_guid_t console_control_guid = holy_EFI_CONSOLE_CONTROL_GUID;
static holy_efi_guid_t loaded_image_guid = holy_EFI_LOADED_IMAGE_GUID;
static holy_efi_guid_t device_path_guid = holy_EFI_DEVICE_PATH_GUID;

void *
holy_efi_locate_protocol (holy_efi_guid_t *protocol, void *registration)
{
  void *interface;
  holy_efi_status_t status;

  status = efi_call_3 (holy_efi_system_table->boot_services->locate_protocol,
                       protocol, registration, &interface);
  if (status != holy_EFI_SUCCESS)
    return 0;

  return interface;
}

/* Return the array of handles which meet the requirement. If successful,
   the number of handles is stored in NUM_HANDLES. The array is allocated
   from the heap.  */
holy_efi_handle_t *
holy_efi_locate_handle (holy_efi_locate_search_type_t search_type,
			holy_efi_guid_t *protocol,
			void *search_key,
			holy_efi_uintn_t *num_handles)
{
  holy_efi_boot_services_t *b;
  holy_efi_status_t status;
  holy_efi_handle_t *buffer;
  holy_efi_uintn_t buffer_size = 8 * sizeof (holy_efi_handle_t);

  buffer = holy_malloc (buffer_size);
  if (! buffer)
    return 0;

  b = holy_efi_system_table->boot_services;
  status = efi_call_5 (b->locate_handle, search_type, protocol, search_key,
			     &buffer_size, buffer);
  if (status == holy_EFI_BUFFER_TOO_SMALL)
    {
      holy_free (buffer);
      buffer = holy_malloc (buffer_size);
      if (! buffer)
	return 0;

      status = efi_call_5 (b->locate_handle, search_type, protocol, search_key,
				 &buffer_size, buffer);
    }

  if (status != holy_EFI_SUCCESS)
    {
      holy_free (buffer);
      return 0;
    }

  *num_handles = buffer_size / sizeof (holy_efi_handle_t);
  return buffer;
}

void *
holy_efi_open_protocol (holy_efi_handle_t handle,
			holy_efi_guid_t *protocol,
			holy_efi_uint32_t attributes)
{
  holy_efi_boot_services_t *b;
  holy_efi_status_t status;
  void *interface;

  b = holy_efi_system_table->boot_services;
  status = efi_call_6 (b->open_protocol, handle,
		       protocol,
		       &interface,
		       holy_efi_image_handle,
		       0,
		       attributes);
  if (status != holy_EFI_SUCCESS)
    return 0;

  return interface;
}

int
holy_efi_set_text_mode (int on)
{
  holy_efi_console_control_protocol_t *c;
  holy_efi_screen_mode_t mode, new_mode;

  c = holy_efi_locate_protocol (&console_control_guid, 0);
  if (! c)
    /* No console control protocol instance available, assume it is
       already in text mode. */
    return 1;

  if (efi_call_4 (c->get_mode, c, &mode, 0, 0) != holy_EFI_SUCCESS)
    return 0;

  new_mode = on ? holy_EFI_SCREEN_TEXT : holy_EFI_SCREEN_GRAPHICS;
  if (mode != new_mode)
    if (efi_call_2 (c->set_mode, c, new_mode) != holy_EFI_SUCCESS)
      return 0;

  return 1;
}

void
holy_efi_stall (holy_efi_uintn_t microseconds)
{
  efi_call_1 (holy_efi_system_table->boot_services->stall, microseconds);
}

holy_efi_loaded_image_t *
holy_efi_get_loaded_image (holy_efi_handle_t image_handle)
{
  return holy_efi_open_protocol (image_handle,
				 &loaded_image_guid,
				 holy_EFI_OPEN_PROTOCOL_GET_PROTOCOL);
}

void
holy_exit (void)
{
  holy_machine_fini (holy_LOADER_FLAG_NORETURN);
  efi_call_4 (holy_efi_system_table->boot_services->exit,
              holy_efi_image_handle, holy_EFI_SUCCESS, 0, 0);
  for (;;) ;
}

holy_err_t
holy_efi_set_virtual_address_map (holy_efi_uintn_t memory_map_size,
				  holy_efi_uintn_t descriptor_size,
				  holy_efi_uint32_t descriptor_version,
				  holy_efi_memory_descriptor_t *virtual_map)
{
  holy_efi_runtime_services_t *r;
  holy_efi_status_t status;

  r = holy_efi_system_table->runtime_services;
  status = efi_call_4 (r->set_virtual_address_map, memory_map_size,
		       descriptor_size, descriptor_version, virtual_map);

  if (status == holy_EFI_SUCCESS)
    return holy_ERR_NONE;

  return holy_error (holy_ERR_IO, "set_virtual_address_map failed");
}

holy_err_t
holy_efi_set_variable(const char *var, const holy_efi_guid_t *guid,
		      void *data, holy_size_t datasize)
{
  holy_efi_status_t status;
  holy_efi_runtime_services_t *r;
  holy_efi_char16_t *var16;
  holy_size_t len, len16;

  len = holy_strlen (var);
  len16 = len * holy_MAX_UTF16_PER_UTF8;
  var16 = holy_malloc ((len16 + 1) * sizeof (var16[0]));
  if (!var16)
    return holy_errno;
  len16 = holy_utf8_to_utf16 (var16, len16, (holy_uint8_t *) var, len, NULL);
  var16[len16] = 0;

  r = holy_efi_system_table->runtime_services;

  status = efi_call_5 (r->set_variable, var16, guid, 
		       (holy_EFI_VARIABLE_NON_VOLATILE
			| holy_EFI_VARIABLE_BOOTSERVICE_ACCESS
			| holy_EFI_VARIABLE_RUNTIME_ACCESS),
		       datasize, data);
  holy_free (var16);
  if (status == holy_EFI_SUCCESS)
    return holy_ERR_NONE;

  return holy_error (holy_ERR_IO, "could not set EFI variable `%s'", var);
}

void *
holy_efi_get_variable (const char *var, const holy_efi_guid_t *guid,
		       holy_size_t *datasize_out)
{
  holy_efi_status_t status;
  holy_efi_uintn_t datasize = 0;
  holy_efi_runtime_services_t *r;
  holy_efi_char16_t *var16;
  void *data;
  holy_size_t len, len16;

  *datasize_out = 0;

  len = holy_strlen (var);
  len16 = len * holy_MAX_UTF16_PER_UTF8;
  var16 = holy_malloc ((len16 + 1) * sizeof (var16[0]));
  if (!var16)
    return NULL;
  len16 = holy_utf8_to_utf16 (var16, len16, (holy_uint8_t *) var, len, NULL);
  var16[len16] = 0;

  r = holy_efi_system_table->runtime_services;

  status = efi_call_5 (r->get_variable, var16, guid, NULL, &datasize, NULL);

  if (status != holy_EFI_BUFFER_TOO_SMALL || !datasize)
    {
      holy_free (var16);
      return NULL;
    }

  data = holy_malloc (datasize);
  if (!data)
    {
      holy_free (var16);
      return NULL;
    }

  status = efi_call_5 (r->get_variable, var16, guid, NULL, &datasize, data);
  holy_free (var16);

  if (status == holy_EFI_SUCCESS)
    {
      *datasize_out = datasize;
      return data;
    }

  holy_free (data);
  return NULL;
}

holy_efi_boolean_t
holy_efi_secure_boot (void)
{
  holy_efi_guid_t efi_var_guid = holy_EFI_GLOBAL_VARIABLE_GUID;
  holy_size_t datasize;
  char *secure_boot = NULL;
  char *setup_mode = NULL;
  holy_efi_boolean_t ret = 0;

  secure_boot = holy_efi_get_variable("SecureBoot", &efi_var_guid, &datasize);

  if (datasize != 1 || !secure_boot)
    goto out;

  setup_mode = holy_efi_get_variable("SetupMode", &efi_var_guid, &datasize);

  if (datasize != 1 || !setup_mode)
    goto out;

  if (*secure_boot && !*setup_mode)
    ret = 1;

 out:
  holy_free (secure_boot);
  holy_free (setup_mode);
  return ret;
}

#pragma GCC diagnostic ignored "-Wcast-align"

/* Search the mods section from the PE32/PE32+ image. This code uses
   a PE32 header, but should work with PE32+ as well.  */
holy_addr_t
holy_efi_modules_addr (void)
{
  holy_efi_loaded_image_t *image;
  struct holy_pe32_header *header;
  struct holy_pe32_coff_header *coff_header;
  struct holy_pe32_section_table *sections;
  struct holy_pe32_section_table *section;
  struct holy_module_info *info;
  holy_uint16_t i;

  image = holy_efi_get_loaded_image (holy_efi_image_handle);
  if (! image)
    return 0;

  header = image->image_base;
  coff_header = &(header->coff_header);
  sections
    = (struct holy_pe32_section_table *) ((char *) coff_header
					  + sizeof (*coff_header)
					  + coff_header->optional_header_size);

  for (i = 0, section = sections;
       i < coff_header->num_sections;
       i++, section++)
    {
      if (holy_strcmp (section->name, "mods") == 0)
	break;
    }

  if (i == coff_header->num_sections)
    return 0;

  info = (struct holy_module_info *) ((char *) image->image_base
				      + section->virtual_address);
  if (info->magic != holy_MODULE_MAGIC)
    return 0;

  return (holy_addr_t) info;
}

#pragma GCC diagnostic error "-Wcast-align"

char *
holy_efi_get_filename (holy_efi_device_path_t *dp0)
{
  char *name = 0, *p, *pi;
  holy_size_t filesize = 0;
  holy_efi_device_path_t *dp;

  dp = dp0;

  while (1)
    {
      holy_efi_uint8_t type = holy_EFI_DEVICE_PATH_TYPE (dp);
      holy_efi_uint8_t subtype = holy_EFI_DEVICE_PATH_SUBTYPE (dp);

      if (type == holy_EFI_END_DEVICE_PATH_TYPE)
	break;
      if (type == holy_EFI_MEDIA_DEVICE_PATH_TYPE
	       && subtype == holy_EFI_FILE_PATH_DEVICE_PATH_SUBTYPE)
	{
	  holy_efi_uint16_t len;
	  len = ((holy_EFI_DEVICE_PATH_LENGTH (dp) - 4)
		 / sizeof (holy_efi_char16_t));
	  filesize += holy_MAX_UTF8_PER_UTF16 * len + 2;
	}

      dp = holy_EFI_NEXT_DEVICE_PATH (dp);
    }

  if (!filesize)
    return NULL;

  dp = dp0;

  p = name = holy_malloc (filesize);
  if (!name)
    return NULL;

  while (1)
    {
      holy_efi_uint8_t type = holy_EFI_DEVICE_PATH_TYPE (dp);
      holy_efi_uint8_t subtype = holy_EFI_DEVICE_PATH_SUBTYPE (dp);

      if (type == holy_EFI_END_DEVICE_PATH_TYPE)
	break;
      else if (type == holy_EFI_MEDIA_DEVICE_PATH_TYPE
	       && subtype == holy_EFI_FILE_PATH_DEVICE_PATH_SUBTYPE)
	{
	  holy_efi_file_path_device_path_t *fp;
	  holy_efi_uint16_t len;

	  *p++ = '/';

	  len = ((holy_EFI_DEVICE_PATH_LENGTH (dp) - 4)
		 / sizeof (holy_efi_char16_t));
	  fp = (holy_efi_file_path_device_path_t *) dp;
	  /* According to EFI spec Path Name is NULL terminated */
	  while (len > 0 && fp->path_name[len - 1] == 0)
	    len--;

	  p = (char *) holy_utf16_to_utf8 ((unsigned char *) p, fp->path_name, len);
	}

      dp = holy_EFI_NEXT_DEVICE_PATH (dp);
    }

  *p = '\0';

  for (pi = name, p = name; *pi;)
    {
      /* EFI breaks paths with backslashes.  */
      if (*pi == '\\' || *pi == '/')
	{
	  *p++ = '/';
	  while (*pi == '\\' || *pi == '/')
	    pi++;
	  continue;
	}
      *p++ = *pi++;
    }
  *p = '\0';

  return name;
}

holy_efi_device_path_t *
holy_efi_get_device_path (holy_efi_handle_t handle)
{
  return holy_efi_open_protocol (handle, &device_path_guid,
				 holy_EFI_OPEN_PROTOCOL_GET_PROTOCOL);
}

/* Return the device path node right before the end node.  */
holy_efi_device_path_t *
holy_efi_find_last_device_path (const holy_efi_device_path_t *dp)
{
  holy_efi_device_path_t *next, *p;

  if (holy_EFI_END_ENTIRE_DEVICE_PATH (dp))
    return 0;

  for (p = (holy_efi_device_path_t *) dp, next = holy_EFI_NEXT_DEVICE_PATH (p);
       ! holy_EFI_END_ENTIRE_DEVICE_PATH (next);
       p = next, next = holy_EFI_NEXT_DEVICE_PATH (next))
    ;

  return p;
}

/* Duplicate a device path.  */
holy_efi_device_path_t *
holy_efi_duplicate_device_path (const holy_efi_device_path_t *dp)
{
  holy_efi_device_path_t *p;
  holy_size_t total_size = 0;

  for (p = (holy_efi_device_path_t *) dp;
       ;
       p = holy_EFI_NEXT_DEVICE_PATH (p))
    {
      total_size += holy_EFI_DEVICE_PATH_LENGTH (p);
      if (holy_EFI_END_ENTIRE_DEVICE_PATH (p))
	break;
    }

  p = holy_malloc (total_size);
  if (! p)
    return 0;

  holy_memcpy (p, dp, total_size);
  return p;
}

static void
dump_vendor_path (const char *type, holy_efi_vendor_device_path_t *vendor)
{
  holy_uint32_t vendor_data_len = vendor->header.length - sizeof (*vendor);
  holy_printf ("/%sVendor(%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x)[%x: ",
	       type,
	       (unsigned) vendor->vendor_guid.data1,
	       (unsigned) vendor->vendor_guid.data2,
	       (unsigned) vendor->vendor_guid.data3,
	       (unsigned) vendor->vendor_guid.data4[0],
	       (unsigned) vendor->vendor_guid.data4[1],
	       (unsigned) vendor->vendor_guid.data4[2],
	       (unsigned) vendor->vendor_guid.data4[3],
	       (unsigned) vendor->vendor_guid.data4[4],
	       (unsigned) vendor->vendor_guid.data4[5],
	       (unsigned) vendor->vendor_guid.data4[6],
	       (unsigned) vendor->vendor_guid.data4[7],
	       vendor_data_len);
  if (vendor->header.length > sizeof (*vendor))
    {
      holy_uint32_t i;
      for (i = 0; i < vendor_data_len; i++)
	holy_printf ("%02x ", vendor->vendor_defined_data[i]);
    }
  holy_printf ("]");
}


/* Print the chain of Device Path nodes. This is mainly for debugging. */
void
holy_efi_print_device_path (holy_efi_device_path_t *dp)
{
  while (1)
    {
      holy_efi_uint8_t type = holy_EFI_DEVICE_PATH_TYPE (dp);
      holy_efi_uint8_t subtype = holy_EFI_DEVICE_PATH_SUBTYPE (dp);
      holy_efi_uint16_t len = holy_EFI_DEVICE_PATH_LENGTH (dp);

      switch (type)
	{
	case holy_EFI_END_DEVICE_PATH_TYPE:
	  switch (subtype)
	    {
	    case holy_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE:
	      holy_printf ("/EndEntire\n");
	      //holy_putchar ('\n');
	      break;
	    case holy_EFI_END_THIS_DEVICE_PATH_SUBTYPE:
	      holy_printf ("/EndThis\n");
	      //holy_putchar ('\n');
	      break;
	    default:
	      holy_printf ("/EndUnknown(%x)\n", (unsigned) subtype);
	      break;
	    }
	  break;

	case holy_EFI_HARDWARE_DEVICE_PATH_TYPE:
	  switch (subtype)
	    {
	    case holy_EFI_PCI_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_pci_device_path_t *pci
		  = (holy_efi_pci_device_path_t *) dp;
		holy_printf ("/PCI(%x,%x)",
			     (unsigned) pci->function, (unsigned) pci->device);
	      }
	      break;
	    case holy_EFI_PCCARD_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_pccard_device_path_t *pccard
		  = (holy_efi_pccard_device_path_t *) dp;
		holy_printf ("/PCCARD(%x)",
			     (unsigned) pccard->function);
	      }
	      break;
	    case holy_EFI_MEMORY_MAPPED_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_memory_mapped_device_path_t *mmapped
		  = (holy_efi_memory_mapped_device_path_t *) dp;
		holy_printf ("/MMap(%x,%llx,%llx)",
			     (unsigned) mmapped->memory_type,
			     (unsigned long long) mmapped->start_address,
			     (unsigned long long) mmapped->end_address);
	      }
	      break;
	    case holy_EFI_VENDOR_DEVICE_PATH_SUBTYPE:
	      dump_vendor_path ("Hardware",
				(holy_efi_vendor_device_path_t *) dp);
	      break;
	    case holy_EFI_CONTROLLER_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_controller_device_path_t *controller
		  = (holy_efi_controller_device_path_t *) dp;
		holy_printf ("/Ctrl(%x)",
			     (unsigned) controller->controller_number);
	      }
	      break;
	    default:
	      holy_printf ("/UnknownHW(%x)", (unsigned) subtype);
	      break;
	    }
	  break;

	case holy_EFI_ACPI_DEVICE_PATH_TYPE:
	  switch (subtype)
	    {
	    case holy_EFI_ACPI_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_acpi_device_path_t *acpi
		  = (holy_efi_acpi_device_path_t *) dp;
		holy_printf ("/ACPI(%x,%x)",
			     (unsigned) acpi->hid,
			     (unsigned) acpi->uid);
	      }
	      break;
	    case holy_EFI_EXPANDED_ACPI_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_expanded_acpi_device_path_t *eacpi
		  = (holy_efi_expanded_acpi_device_path_t *) dp;
		holy_printf ("/ACPI(");

		if (holy_EFI_EXPANDED_ACPI_HIDSTR (dp)[0] == '\0')
		  holy_printf ("%x,", (unsigned) eacpi->hid);
		else
		  holy_printf ("%s,", holy_EFI_EXPANDED_ACPI_HIDSTR (dp));

		if (holy_EFI_EXPANDED_ACPI_UIDSTR (dp)[0] == '\0')
		  holy_printf ("%x,", (unsigned) eacpi->uid);
		else
		  holy_printf ("%s,", holy_EFI_EXPANDED_ACPI_UIDSTR (dp));

		if (holy_EFI_EXPANDED_ACPI_CIDSTR (dp)[0] == '\0')
		  holy_printf ("%x)", (unsigned) eacpi->cid);
		else
		  holy_printf ("%s)", holy_EFI_EXPANDED_ACPI_CIDSTR (dp));
	      }
	      break;
	    default:
	      holy_printf ("/UnknownACPI(%x)", (unsigned) subtype);
	      break;
	    }
	  break;

	case holy_EFI_MESSAGING_DEVICE_PATH_TYPE:
	  switch (subtype)
	    {
	    case holy_EFI_ATAPI_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_atapi_device_path_t *atapi
		  = (holy_efi_atapi_device_path_t *) dp;
		holy_printf ("/ATAPI(%x,%x,%x)",
			     (unsigned) atapi->primary_secondary,
			     (unsigned) atapi->slave_master,
			     (unsigned) atapi->lun);
	      }
	      break;
	    case holy_EFI_SCSI_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_scsi_device_path_t *scsi
		  = (holy_efi_scsi_device_path_t *) dp;
		holy_printf ("/SCSI(%x,%x)",
			     (unsigned) scsi->pun,
			     (unsigned) scsi->lun);
	      }
	      break;
	    case holy_EFI_FIBRE_CHANNEL_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_fibre_channel_device_path_t *fc
		  = (holy_efi_fibre_channel_device_path_t *) dp;
		holy_printf ("/FibreChannel(%llx,%llx)",
			     (unsigned long long) fc->wwn,
			     (unsigned long long) fc->lun);
	      }
	      break;
	    case holy_EFI_1394_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_1394_device_path_t *firewire
		  = (holy_efi_1394_device_path_t *) dp;
		holy_printf ("/1394(%llx)",
			     (unsigned long long) firewire->guid);
	      }
	      break;
	    case holy_EFI_USB_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_usb_device_path_t *usb
		  = (holy_efi_usb_device_path_t *) dp;
		holy_printf ("/USB(%x,%x)",
			     (unsigned) usb->parent_port_number,
			     (unsigned) usb->usb_interface);
	      }
	      break;
	    case holy_EFI_USB_CLASS_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_usb_class_device_path_t *usb_class
		  = (holy_efi_usb_class_device_path_t *) dp;
		holy_printf ("/USBClass(%x,%x,%x,%x,%x)",
			     (unsigned) usb_class->vendor_id,
			     (unsigned) usb_class->product_id,
			     (unsigned) usb_class->device_class,
			     (unsigned) usb_class->device_subclass,
			     (unsigned) usb_class->device_protocol);
	      }
	      break;
	    case holy_EFI_I2O_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_i2o_device_path_t *i2o
		  = (holy_efi_i2o_device_path_t *) dp;
		holy_printf ("/I2O(%x)", (unsigned) i2o->tid);
	      }
	      break;
	    case holy_EFI_MAC_ADDRESS_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_mac_address_device_path_t *mac
		  = (holy_efi_mac_address_device_path_t *) dp;
		holy_printf ("/MacAddr(%02x:%02x:%02x:%02x:%02x:%02x,%x)",
			     (unsigned) mac->mac_address[0],
			     (unsigned) mac->mac_address[1],
			     (unsigned) mac->mac_address[2],
			     (unsigned) mac->mac_address[3],
			     (unsigned) mac->mac_address[4],
			     (unsigned) mac->mac_address[5],
			     (unsigned) mac->if_type);
	      }
	      break;
	    case holy_EFI_IPV4_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_ipv4_device_path_t *ipv4
		  = (holy_efi_ipv4_device_path_t *) dp;
		holy_printf ("/IPv4(%u.%u.%u.%u,%u.%u.%u.%u,%u,%u,%x,%x)",
			     (unsigned) ipv4->local_ip_address[0],
			     (unsigned) ipv4->local_ip_address[1],
			     (unsigned) ipv4->local_ip_address[2],
			     (unsigned) ipv4->local_ip_address[3],
			     (unsigned) ipv4->remote_ip_address[0],
			     (unsigned) ipv4->remote_ip_address[1],
			     (unsigned) ipv4->remote_ip_address[2],
			     (unsigned) ipv4->remote_ip_address[3],
			     (unsigned) ipv4->local_port,
			     (unsigned) ipv4->remote_port,
			     (unsigned) ipv4->protocol,
			     (unsigned) ipv4->static_ip_address);
	      }
	      break;
	    case holy_EFI_IPV6_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_ipv6_device_path_t *ipv6
		  = (holy_efi_ipv6_device_path_t *) dp;
		holy_printf ("/IPv6(%x:%x:%x:%x:%x:%x:%x:%x,%x:%x:%x:%x:%x:%x:%x:%x,%u,%u,%x,%x)",
			     (unsigned) ipv6->local_ip_address[0],
			     (unsigned) ipv6->local_ip_address[1],
			     (unsigned) ipv6->local_ip_address[2],
			     (unsigned) ipv6->local_ip_address[3],
			     (unsigned) ipv6->local_ip_address[4],
			     (unsigned) ipv6->local_ip_address[5],
			     (unsigned) ipv6->local_ip_address[6],
			     (unsigned) ipv6->local_ip_address[7],
			     (unsigned) ipv6->remote_ip_address[0],
			     (unsigned) ipv6->remote_ip_address[1],
			     (unsigned) ipv6->remote_ip_address[2],
			     (unsigned) ipv6->remote_ip_address[3],
			     (unsigned) ipv6->remote_ip_address[4],
			     (unsigned) ipv6->remote_ip_address[5],
			     (unsigned) ipv6->remote_ip_address[6],
			     (unsigned) ipv6->remote_ip_address[7],
			     (unsigned) ipv6->local_port,
			     (unsigned) ipv6->remote_port,
			     (unsigned) ipv6->protocol,
			     (unsigned) ipv6->static_ip_address);
	      }
	      break;
	    case holy_EFI_INFINIBAND_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_infiniband_device_path_t *ib
		  = (holy_efi_infiniband_device_path_t *) dp;
		holy_printf ("/InfiniBand(%x,%llx,%llx,%llx)",
			     (unsigned) ib->port_gid[0], /* XXX */
			     (unsigned long long) ib->remote_id,
			     (unsigned long long) ib->target_port_id,
			     (unsigned long long) ib->device_id);
	      }
	      break;
	    case holy_EFI_UART_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_uart_device_path_t *uart
		  = (holy_efi_uart_device_path_t *) dp;
		holy_printf ("/UART(%llu,%u,%x,%x)",
			     (unsigned long long) uart->baud_rate,
			     uart->data_bits,
			     uart->parity,
			     uart->stop_bits);
	      }
	      break;
	    case holy_EFI_SATA_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_sata_device_path_t *sata;
		sata = (holy_efi_sata_device_path_t *) dp;
		holy_printf ("/Sata(%x,%x,%x)",
			     sata->hba_port,
			     sata->multiplier_port,
			     sata->lun);
	      }
	      break;

	    case holy_EFI_VENDOR_MESSAGING_DEVICE_PATH_SUBTYPE:
	      dump_vendor_path ("Messaging",
				(holy_efi_vendor_device_path_t *) dp);
	      break;
	    default:
	      holy_printf ("/UnknownMessaging(%x)", (unsigned) subtype);
	      break;
	    }
	  break;

	case holy_EFI_MEDIA_DEVICE_PATH_TYPE:
	  switch (subtype)
	    {
	    case holy_EFI_HARD_DRIVE_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_hard_drive_device_path_t *hd = (holy_efi_hard_drive_device_path_t *) dp;
		holy_printf ("/HD(%u,%llx,%llx,%02x%02x%02x%02x%02x%02x%02x%02x,%x,%x)",
			     hd->partition_number,
			     (unsigned long long) hd->partition_start,
			     (unsigned long long) hd->partition_size,
			     (unsigned) hd->partition_signature[0],
			     (unsigned) hd->partition_signature[1],
			     (unsigned) hd->partition_signature[2],
			     (unsigned) hd->partition_signature[3],
			     (unsigned) hd->partition_signature[4],
			     (unsigned) hd->partition_signature[5],
			     (unsigned) hd->partition_signature[6],
			     (unsigned) hd->partition_signature[7],
			     (unsigned) hd->partmap_type,
			     (unsigned) hd->signature_type);
	      }
	      break;
	    case holy_EFI_CDROM_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_cdrom_device_path_t *cd
		  = (holy_efi_cdrom_device_path_t *) dp;
		holy_printf ("/CD(%u,%llx,%llx)",
			     cd->boot_entry,
			     (unsigned long long) cd->partition_start,
			     (unsigned long long) cd->partition_size);
	      }
	      break;
	    case holy_EFI_VENDOR_MEDIA_DEVICE_PATH_SUBTYPE:
	      dump_vendor_path ("Media",
				(holy_efi_vendor_device_path_t *) dp);
	      break;
	    case holy_EFI_FILE_PATH_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_file_path_device_path_t *fp;
		holy_uint8_t *buf;
		fp = (holy_efi_file_path_device_path_t *) dp;
		buf = holy_malloc ((len - 4) * 2 + 1);
		if (buf)
		  *holy_utf16_to_utf8 (buf, fp->path_name,
				       (len - 4) / sizeof (holy_efi_char16_t))
		    = '\0';
		else
		  holy_errno = holy_ERR_NONE;
		holy_printf ("/File(%s)", buf);
		holy_free (buf);
	      }
	      break;
	    case holy_EFI_PROTOCOL_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_protocol_device_path_t *proto
		  = (holy_efi_protocol_device_path_t *) dp;
		holy_printf ("/Protocol(%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x)",
			     (unsigned) proto->guid.data1,
			     (unsigned) proto->guid.data2,
			     (unsigned) proto->guid.data3,
			     (unsigned) proto->guid.data4[0],
			     (unsigned) proto->guid.data4[1],
			     (unsigned) proto->guid.data4[2],
			     (unsigned) proto->guid.data4[3],
			     (unsigned) proto->guid.data4[4],
			     (unsigned) proto->guid.data4[5],
			     (unsigned) proto->guid.data4[6],
			     (unsigned) proto->guid.data4[7]);
	      }
	      break;
	    default:
	      holy_printf ("/UnknownMedia(%x)", (unsigned) subtype);
	      break;
	    }
	  break;

	case holy_EFI_BIOS_DEVICE_PATH_TYPE:
	  switch (subtype)
	    {
	    case holy_EFI_BIOS_DEVICE_PATH_SUBTYPE:
	      {
		holy_efi_bios_device_path_t *bios
		  = (holy_efi_bios_device_path_t *) dp;
		holy_printf ("/BIOS(%x,%x,%s)",
			     (unsigned) bios->device_type,
			     (unsigned) bios->status_flags,
			     (char *) (dp + 1));
	      }
	      break;
	    default:
	      holy_printf ("/UnknownBIOS(%x)", (unsigned) subtype);
	      break;
	    }
	  break;

	default:
	  holy_printf ("/UnknownType(%x,%x)\n",
		       (unsigned) type,
		       (unsigned) subtype);
	  return;
	  break;
	}

      if (holy_EFI_END_ENTIRE_DEVICE_PATH (dp))
	break;

      dp = (holy_efi_device_path_t *) ((char *) dp + len);
    }
}

/* Compare device paths.  */
int
holy_efi_compare_device_paths (const holy_efi_device_path_t *dp1,
			       const holy_efi_device_path_t *dp2)
{
  if (! dp1 || ! dp2)
    /* Return non-zero.  */
    return 1;

  while (1)
    {
      holy_efi_uint8_t type1, type2;
      holy_efi_uint8_t subtype1, subtype2;
      holy_efi_uint16_t len1, len2;
      int ret;

      type1 = holy_EFI_DEVICE_PATH_TYPE (dp1);
      type2 = holy_EFI_DEVICE_PATH_TYPE (dp2);

      if (type1 != type2)
	return (int) type2 - (int) type1;

      subtype1 = holy_EFI_DEVICE_PATH_SUBTYPE (dp1);
      subtype2 = holy_EFI_DEVICE_PATH_SUBTYPE (dp2);

      if (subtype1 != subtype2)
	return (int) subtype1 - (int) subtype2;

      len1 = holy_EFI_DEVICE_PATH_LENGTH (dp1);
      len2 = holy_EFI_DEVICE_PATH_LENGTH (dp2);

      if (len1 != len2)
	return (int) len1 - (int) len2;

      ret = holy_memcmp (dp1, dp2, len1);
      if (ret != 0)
	return ret;

      if (holy_EFI_END_ENTIRE_DEVICE_PATH (dp1))
	break;

      dp1 = (holy_efi_device_path_t *) ((char *) dp1 + len1);
      dp2 = (holy_efi_device_path_t *) ((char *) dp2 + len2);
    }

  return 0;
}
