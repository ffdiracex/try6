/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/efi/api.h>
#include <holy/efi/edid.h>
#include <holy/efi/pci.h>
#include <holy/efi/efi.h>
#include <holy/efi/uga_draw.h>
#include <holy/efi/graphics_output.h>
#include <holy/efi/console_control.h>
#include <holy/command.h>

holy_MOD_LICENSE ("GPLv2+");

struct known_protocol
{
  holy_efi_guid_t guid;
  const char *name;
} known_protocols[] = 
  {
    { holy_EFI_DISK_IO_GUID, "disk" },
    { holy_EFI_BLOCK_IO_GUID, "block" },
    { holy_EFI_SERIAL_IO_GUID, "serial" },
    { holy_EFI_SIMPLE_NETWORK_GUID, "network" },
    { holy_EFI_PXE_GUID, "pxe" },
    { holy_EFI_DEVICE_PATH_GUID, "device path" },
    { holy_EFI_PCI_IO_GUID, "PCI" },
    { holy_EFI_PCI_ROOT_IO_GUID, "PCI root" },
    { holy_EFI_EDID_ACTIVE_GUID, "active EDID" },
    { holy_EFI_EDID_DISCOVERED_GUID, "discovered EDID" },
    { holy_EFI_EDID_OVERRIDE_GUID, "override EDID" },
    { holy_EFI_GOP_GUID, "GOP" },
    { holy_EFI_UGA_DRAW_GUID, "UGA draw" },
    { holy_EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID, "simple text output" },
    { holy_EFI_SIMPLE_TEXT_INPUT_PROTOCOL_GUID, "simple text input" },
    { holy_EFI_SIMPLE_POINTER_PROTOCOL_GUID, "simple pointer" },
    { holy_EFI_CONSOLE_CONTROL_GUID, "console control" },
    { holy_EFI_ABSOLUTE_POINTER_PROTOCOL_GUID, "absolute pointer" },
    { holy_EFI_DRIVER_BINDING_PROTOCOL_GUID, "EFI driver binding" },
    { holy_EFI_LOAD_FILE_PROTOCOL_GUID, "load file" },
    { holy_EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID, "simple FS" },
    { holy_EFI_TAPE_IO_PROTOCOL_GUID, "tape I/O" },
    { holy_EFI_UNICODE_COLLATION_PROTOCOL_GUID, "unicode collation" },
    { holy_EFI_SCSI_IO_PROTOCOL_GUID, "SCSI I/O" },
    { holy_EFI_USB2_HC_PROTOCOL_GUID, "USB host" },
    { holy_EFI_DEBUG_SUPPORT_PROTOCOL_GUID, "debug support" },
    { holy_EFI_DEBUGPORT_PROTOCOL_GUID, "debug port" },
    { holy_EFI_DECOMPRESS_PROTOCOL_GUID, "decompress" },
    { holy_EFI_LOADED_IMAGE_PROTOCOL_GUID, "loaded image" },
    { holy_EFI_DEVICE_PATH_TO_TEXT_PROTOCOL_GUID, "device path to text" },
    { holy_EFI_DEVICE_PATH_UTILITIES_PROTOCOL_GUID, "device path utilities" },
    { holy_EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL_GUID, "device path from text" },
    { holy_EFI_HII_CONFIG_ROUTING_PROTOCOL_GUID, "HII config routing" },
    { holy_EFI_HII_DATABASE_PROTOCOL_GUID, "HII database" },
    { holy_EFI_HII_STRING_PROTOCOL_GUID, "HII string" },
    { holy_EFI_HII_IMAGE_PROTOCOL_GUID, "HII image" },
    { holy_EFI_HII_FONT_PROTOCOL_GUID, "HII font" },
    { holy_EFI_COMPONENT_NAME2_PROTOCOL_GUID, "component name 2" },
    { holy_EFI_HII_CONFIGURATION_ACCESS_PROTOCOL_GUID,
      "HII configuration access" },
    { holy_EFI_USB_IO_PROTOCOL_GUID, "USB I/O" },
  };

static holy_err_t
holy_cmd_lsefi (holy_command_t cmd __attribute__ ((unused)),
		int argc __attribute__ ((unused)),
		char **args __attribute__ ((unused)))
{
  holy_efi_handle_t *handles;
  holy_efi_uintn_t num_handles;
  unsigned i, j, k;

  handles = holy_efi_locate_handle (holy_EFI_ALL_HANDLES,
				    NULL, NULL, &num_handles);

  for (i = 0; i < num_handles; i++)
    {
      holy_efi_handle_t handle = handles[i];
      holy_efi_status_t status;
      holy_efi_uintn_t num_protocols;
      holy_efi_packed_guid_t **protocols;
      holy_efi_device_path_t *dp;

      holy_printf ("Handle %p\n", handle);

      dp = holy_efi_get_device_path (handle);
      if (dp)
	{
	  holy_printf ("  ");
	  holy_efi_print_device_path (dp);
	}

      status = efi_call_3 (holy_efi_system_table->boot_services->protocols_per_handle,
			   handle, &protocols, &num_protocols);
      if (status != holy_EFI_SUCCESS)
	holy_printf ("Unable to retrieve protocols\n");
      for (j = 0; j < num_protocols; j++)
	{
	  for (k = 0; k < ARRAY_SIZE (known_protocols); k++)
	    if (holy_memcmp (protocols[j], &known_protocols[k].guid,
			     sizeof (known_protocols[k].guid)) == 0)
		break;
	  if (k < ARRAY_SIZE (known_protocols))
	    holy_printf ("  %s\n", known_protocols[k].name);
	  else
	    holy_printf ("  %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\n",
			 protocols[j]->data1,
			 protocols[j]->data2,
			 protocols[j]->data3,
			 (unsigned) protocols[j]->data4[0],
			 (unsigned) protocols[j]->data4[1],
			 (unsigned) protocols[j]->data4[2],
			 (unsigned) protocols[j]->data4[3],
			 (unsigned) protocols[j]->data4[4],
			 (unsigned) protocols[j]->data4[5],
			 (unsigned) protocols[j]->data4[6],
			 (unsigned) protocols[j]->data4[7]);
	}

    }

  return 0;
}

static holy_command_t cmd;

holy_MOD_INIT(lsefi)
{
  cmd = holy_register_command ("lsefi", holy_cmd_lsefi,
			       NULL, "Display EFI handles.");
}

holy_MOD_FINI(lsefi)
{
  holy_unregister_command (cmd);
}
