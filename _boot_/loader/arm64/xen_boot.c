/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/cache.h>
#include <holy/charset.h>
#include <holy/command.h>
#include <holy/err.h>
#include <holy/file.h>
#include <holy/fdt.h>
#include <holy/list.h>
#include <holy/loader.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/types.h>
#include <holy/cpu/fdtload.h>
#include <holy/cpu/linux.h>
#include <holy/efi/efi.h>
#include <holy/efi/pe32.h>	/* required by struct xen_hypervisor_header */
#include <holy/i18n.h>
#include <holy/lib/cmdline.h>

holy_MOD_LICENSE ("GPLv2+");

#define XEN_HYPERVISOR_NAME  "xen_hypervisor"
#define MODULE_CUSTOM_COMPATIBLE  "multiboot,module"

/* This maximum size is defined in Power.org ePAPR V1.1
 * https://www.power.org/documentation/epapr-version-1-1/
 * 2.2.1.1 Node Name Requirements
 * node-name@unit-address
 * 31 + 1(@) + 16(64bit address in hex format) + 1(\0) = 49
 */
#define FDT_NODE_NAME_MAX_SIZE  (49)

struct compat_string_struct
{
  holy_size_t size;
  const char *compat_string;
};
typedef struct compat_string_struct compat_string_struct_t;
#define FDT_COMPATIBLE(x) {.size = sizeof(x), .compat_string = (x)}

enum module_type
{
  MODULE_IMAGE,
  MODULE_INITRD,
  MODULE_XSM,
  MODULE_CUSTOM
};
typedef enum module_type module_type_t;

struct xen_hypervisor_header
{
  struct holy_arm64_linux_kernel_header efi_head;

  /* This is always PE\0\0.  */
  holy_uint8_t signature[holy_PE32_SIGNATURE_SIZE];
  /* The COFF file header.  */
  struct holy_pe32_coff_header coff_header;
  /* The Optional header.  */
  struct holy_pe64_optional_header optional_header;
};

struct xen_boot_binary
{
  struct xen_boot_binary *next;
  struct xen_boot_binary **prev;
  int is_hypervisor;

  holy_addr_t start;
  holy_size_t size;
  holy_size_t align;

  char *cmdline;
  int cmdline_size;
};

static holy_dl_t my_mod;

static int loaded;

static struct xen_boot_binary *xen_hypervisor;
static struct xen_boot_binary *module_head;

static __inline holy_addr_t
xen_boot_address_align (holy_addr_t start, holy_size_t align)
{
  return (align ? (ALIGN_UP (start, align)) : start);
}

static holy_err_t
prepare_xen_hypervisor_params (void *xen_boot_fdt)
{
  int chosen_node = 0;
  int retval;

  chosen_node = holy_fdt_find_subnode (xen_boot_fdt, 0, "chosen");
  if (chosen_node < 0)
    chosen_node = holy_fdt_add_subnode (xen_boot_fdt, 0, "chosen");
  if (chosen_node < 1)
    return holy_error (holy_ERR_IO, "failed to get chosen node in FDT");

  holy_dprintf ("xen_loader",
		"Xen Hypervisor cmdline : %s @ %p size:%d\n",
		xen_hypervisor->cmdline, xen_hypervisor->cmdline,
		xen_hypervisor->cmdline_size);

  retval = holy_fdt_set_prop (xen_boot_fdt, chosen_node, "bootargs",
			      xen_hypervisor->cmdline,
			      xen_hypervisor->cmdline_size);
  if (retval)
    return holy_error (holy_ERR_IO, "failed to install/update FDT");

  return holy_ERR_NONE;
}

static holy_err_t
prepare_xen_module_params (struct xen_boot_binary *module, void *xen_boot_fdt)
{
  int retval, chosen_node = 0, module_node = 0;
  char module_name[FDT_NODE_NAME_MAX_SIZE];

  retval = holy_snprintf (module_name, FDT_NODE_NAME_MAX_SIZE, "module@%lx",
			  xen_boot_address_align (module->start,
						  module->align));
  holy_dprintf ("xen_loader", "Module node name %s \n", module_name);

  if (retval < (int) sizeof ("module@"))
    return holy_error (holy_ERR_IO, N_("failed to get FDT"));

  chosen_node = holy_fdt_find_subnode (xen_boot_fdt, 0, "chosen");
  if (chosen_node < 0)
    chosen_node = holy_fdt_add_subnode (xen_boot_fdt, 0, "chosen");
  if (chosen_node < 1)
    return holy_error (holy_ERR_IO, "failed to get chosen node in FDT");

  module_node =
    holy_fdt_find_subnode (xen_boot_fdt, chosen_node, module_name);
  if (module_node < 0)
    module_node =
      holy_fdt_add_subnode (xen_boot_fdt, chosen_node, module_name);

  retval = holy_fdt_set_prop (xen_boot_fdt, module_node, "compatible",
			      MODULE_CUSTOM_COMPATIBLE, sizeof(MODULE_CUSTOM_COMPATIBLE) - 1);
  if (retval)
    return holy_error (holy_ERR_IO, "failed to update FDT");

  holy_dprintf ("xen_loader", "Module\n");

  retval = holy_fdt_set_reg64 (xen_boot_fdt, module_node,
			       xen_boot_address_align (module->start,
						       module->align),
			       module->size);
  if (retval)
    return holy_error (holy_ERR_IO, "failed to update FDT");

  if (module->cmdline && module->cmdline_size > 0)
    {
      holy_dprintf ("xen_loader",
		    "Module cmdline : %s @ %p size:%d\n",
		    module->cmdline, module->cmdline, module->cmdline_size);

      retval = holy_fdt_set_prop (xen_boot_fdt, module_node, "bootargs",
				  module->cmdline, module->cmdline_size + 1);
      if (retval)
	return holy_error (holy_ERR_IO, "failed to update FDT");
    }
  else
    {
      holy_dprintf ("xen_loader", "Module has no bootargs!\n");
    }

  return holy_ERR_NONE;
}

static holy_err_t
finalize_params_xen_boot (void)
{
  struct xen_boot_binary *module;
  void *xen_boot_fdt;
  holy_size_t additional_size = 0x1000;

  /* Hypervisor.  */
  additional_size += FDT_NODE_NAME_MAX_SIZE + xen_hypervisor->cmdline_size;
  FOR_LIST_ELEMENTS (module, module_head)
  {
    additional_size += 6 * FDT_NODE_NAME_MAX_SIZE + sizeof(MODULE_CUSTOM_COMPATIBLE) - 1
      + module->cmdline_size;
  }

  xen_boot_fdt = holy_fdt_load (additional_size);
  if (!xen_boot_fdt)
    return holy_error (holy_ERR_IO, "failed to get FDT");

  if (xen_hypervisor)
    {
      if (prepare_xen_hypervisor_params (xen_boot_fdt) != holy_ERR_NONE)
	goto fail;
    }
  else
    {
      holy_dprintf ("xen_loader", "Failed to get Xen Hypervisor info!\n");
      goto fail;
    }

  /* Set module params info */
  FOR_LIST_ELEMENTS (module, module_head)
  {
    if (module->start && module->size > 0)
      {
	holy_dprintf ("xen_loader", "Module @ 0x%lx size:0x%lx\n",
		      xen_boot_address_align (module->start, module->align),
		      module->size);
	if (prepare_xen_module_params (module, xen_boot_fdt) != holy_ERR_NONE)
	  goto fail;
      }
    else
      {
	holy_dprintf ("xen_loader", "Module info error!\n");
	goto fail;
      }
  }

  if (holy_fdt_install() == holy_ERR_NONE)
    return holy_ERR_NONE;

fail:
  holy_fdt_unload ();

  return holy_error (holy_ERR_IO, "failed to install/update FDT");
}


static holy_err_t
xen_boot (void)
{
  holy_err_t err = finalize_params_xen_boot ();
  if (err)
    return err;

  return holy_arm64_uefi_boot_image (xen_hypervisor->start,
				     xen_hypervisor->size,
				     xen_hypervisor->cmdline);
}

static void
single_binary_unload (struct xen_boot_binary *binary)
{
  if (!binary)
    return;

  if (binary->start && binary->size > 0)
    {
      holy_efi_free_pages ((holy_efi_physical_address_t) binary->start,
			   holy_EFI_BYTES_TO_PAGES (binary->size + binary->align));
    }

  if (binary->cmdline && binary->cmdline_size > 0)
    {
      holy_free (binary->cmdline);
      holy_dprintf ("xen_loader",
		    "Module cmdline memory free @ %p size: %d\n",
		    binary->cmdline, binary->cmdline_size);
    }

  if (!binary->is_hypervisor)
    holy_list_remove (holy_AS_LIST (binary));

  holy_dprintf ("xen_loader",
		"Module struct memory free @ %p size: 0x%lx\n",
		binary, sizeof (binary));
  holy_free (binary);

  return;
}

static void
all_binaries_unload (void)
{
  struct xen_boot_binary *binary;

  FOR_LIST_ELEMENTS (binary, module_head)
  {
    single_binary_unload (binary);
  }

  if (xen_hypervisor)
    single_binary_unload (xen_hypervisor);

  return;
}

static holy_err_t
xen_unload (void)
{
  loaded = 0;
  all_binaries_unload ();
  holy_fdt_unload ();
  holy_dl_unref (my_mod);

  return holy_ERR_NONE;
}

static void
xen_boot_binary_load (struct xen_boot_binary *binary, holy_file_t file,
		      int argc, char *argv[])
{
  binary->size = holy_file_size (file);
  holy_dprintf ("xen_loader", "Xen_boot file size: 0x%lx\n", binary->size);

  binary->start
    = (holy_addr_t) holy_efi_allocate_pages (0,
					     holy_EFI_BYTES_TO_PAGES
					     (binary->size +
					      binary->align));
  if (!binary->start)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, N_("out of memory"));
      return;
    }

  holy_dprintf ("xen_loader", "Xen_boot numpages: 0x%lx\n",
	        holy_EFI_BYTES_TO_PAGES (binary->size + binary->align));

  if (holy_file_read (file, (void *) xen_boot_address_align (binary->start,
							     binary->align),
		      binary->size) != (holy_ssize_t) binary->size)
    {
      single_binary_unload (binary);
      holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"), argv[0]);
      return;
    }

  if (argc > 1)
    {
      binary->cmdline_size = holy_loader_cmdline_size (argc - 1, argv + 1);
      binary->cmdline = holy_zalloc (binary->cmdline_size);
      if (!binary->cmdline)
	{
	  single_binary_unload (binary);
	  holy_error (holy_ERR_OUT_OF_MEMORY, N_("out of memory"));
	  return;
	}
      holy_create_loader_cmdline (argc - 1, argv + 1, binary->cmdline,
				  binary->cmdline_size);
      holy_dprintf ("xen_loader",
		    "Xen_boot cmdline @ %p %s, size: %d\n",
		    binary->cmdline, binary->cmdline, binary->cmdline_size);
    }
  else
    {
      binary->cmdline_size = 0;
      binary->cmdline = NULL;
    }

  holy_errno = holy_ERR_NONE;
  return;
}

static holy_err_t
holy_cmd_xen_module (holy_command_t cmd __attribute__((unused)),
		     int argc, char *argv[])
{

  struct xen_boot_binary *module = NULL;
  holy_file_t file = 0;

  if (!argc)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto fail;
    }

  if (!loaded)
    {
      holy_error (holy_ERR_BAD_ARGUMENT,
		  N_("you need to load the Xen Hypervisor first"));
      goto fail;
    }

  module =
    (struct xen_boot_binary *) holy_zalloc (sizeof (struct xen_boot_binary));
  if (!module)
    return holy_errno;

  module->is_hypervisor = 0;
  module->align = 4096;

  holy_dprintf ("xen_loader", "Init module and node info\n");

  file = holy_file_open (argv[0]);
  if (!file)
    goto fail;

  xen_boot_binary_load (module, file, argc, argv);
  if (holy_errno == holy_ERR_NONE)
    holy_list_push (holy_AS_LIST_P (&module_head), holy_AS_LIST (module));

 fail:
  if (file)
    holy_file_close (file);
  if (holy_errno != holy_ERR_NONE)
    single_binary_unload (module);

  return holy_errno;
}

static holy_err_t
holy_cmd_xen_hypervisor (holy_command_t cmd __attribute__ ((unused)),
			 int argc, char *argv[])
{
  struct xen_hypervisor_header sh;
  holy_file_t file = NULL;

  holy_dl_ref (my_mod);

  if (!argc)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto fail;
    }

  file = holy_file_open (argv[0]);
  if (!file)
    goto fail;

  if (holy_file_read (file, &sh, sizeof (sh)) != (long) sizeof (sh))
    goto fail;
  if (holy_arm64_uefi_check_image
      ((struct holy_arm64_linux_kernel_header *) &sh) != holy_ERR_NONE)
    goto fail;
  holy_file_seek (file, 0);

  /* if another module has called holy_loader_set,
     we need to make sure that another module is unloaded properly */
  holy_loader_unset ();

  xen_hypervisor =
    (struct xen_boot_binary *) holy_zalloc (sizeof (struct xen_boot_binary));
  if (!xen_hypervisor)
    return holy_errno;

  xen_hypervisor->is_hypervisor = 1;
  xen_hypervisor->align = (holy_size_t) sh.optional_header.section_alignment;

  xen_boot_binary_load (xen_hypervisor, file, argc, argv);
  if (holy_errno == holy_ERR_NONE)
    {
      holy_loader_set (xen_boot, xen_unload, 0);
      loaded = 1;
    }

fail:
  if (file)
    holy_file_close (file);
  if (holy_errno != holy_ERR_NONE)
    {
      loaded = 0;
      all_binaries_unload ();
      holy_dl_unref (my_mod);
    }

  return holy_errno;
}

static holy_command_t cmd_xen_hypervisor;
static holy_command_t cmd_xen_module;

holy_MOD_INIT (xen_boot)
{
  cmd_xen_hypervisor =
    holy_register_command ("xen_hypervisor", holy_cmd_xen_hypervisor, 0,
			   N_("Load a xen hypervisor."));
  cmd_xen_module =
    holy_register_command ("xen_module", holy_cmd_xen_module, 0,
			   N_("Load a xen module."));
  my_mod = mod;
}

holy_MOD_FINI (xen_boot)
{
  holy_unregister_command (cmd_xen_hypervisor);
  holy_unregister_command (cmd_xen_module);
}
