/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/loader.h>
#include <holy/command.h>
#include <holy/multiboot.h>
#include <holy/cpu/multiboot.h>
#include <holy/elf.h>
#include <holy/aout.h>
#include <holy/file.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/env.h>
#include <holy/cpu/relocator.h>
#include <holy/video.h>
#include <holy/memory.h>
#include <holy/i18n.h>
#include <holy/tpm.h>

holy_MOD_LICENSE ("GPLv2+");

#ifdef holy_MACHINE_EFI
#include <holy/efi/efi.h>
#endif

struct holy_relocator *holy_multiboot_relocator = NULL;
holy_uint32_t holy_multiboot_payload_eip;
#if defined (holy_MACHINE_PCBIOS) || defined (holy_MACHINE_MULTIBOOT) || defined (holy_MACHINE_COREBOOT) || defined (holy_MACHINE_QEMU)
#define DEFAULT_VIDEO_MODE "text"
#else
#define DEFAULT_VIDEO_MODE "auto"
#endif

static int accepts_video;
static int accepts_ega_text;
static int console_required;
static holy_dl_t my_mod;


/* Helper for holy_get_multiboot_mmap_count.  */
static int
count_hook (holy_uint64_t addr __attribute__ ((unused)),
	    holy_uint64_t size __attribute__ ((unused)),
	    holy_memory_type_t type __attribute__ ((unused)), void *data)
{
  holy_size_t *count = data;

  (*count)++;
  return 0;
}

/* Return the length of the Multiboot mmap that will be needed to allocate
   our platform's map.  */
holy_uint32_t
holy_get_multiboot_mmap_count (void)
{
  holy_size_t count = 0;

  holy_mmap_iterate (count_hook, &count);

  return count;
}

holy_err_t
holy_multiboot_set_video_mode (void)
{
  holy_err_t err;
  const char *modevar;

#if holy_MACHINE_HAS_VGA_TEXT
  if (accepts_video)
#endif
    {
      modevar = holy_env_get ("gfxpayload");
      if (! modevar || *modevar == 0)
	err = holy_video_set_mode (DEFAULT_VIDEO_MODE, 0, 0);
      else
	{
	  char *tmp;
	  tmp = holy_xasprintf ("%s;" DEFAULT_VIDEO_MODE, modevar);
	  if (! tmp)
	    return holy_errno;
	  err = holy_video_set_mode (tmp, 0, 0);
	  holy_free (tmp);
	}
    }
#if holy_MACHINE_HAS_VGA_TEXT
  else
    err = holy_video_set_mode ("text", 0, 0);
#endif

  return err;
}

#ifdef holy_MACHINE_EFI
#ifdef __x86_64__
#define holy_relocator_efi_boot		holy_relocator64_efi_boot
#define holy_relocator_efi_state	holy_relocator64_efi_state
#endif
#endif

#ifdef holy_relocator_efi_boot
static void
efi_boot (struct holy_relocator *rel,
	  holy_uint32_t target)
{
  struct holy_relocator_efi_state state_efi = MULTIBOOT_EFI_INITIAL_STATE;

  state_efi.MULTIBOOT_EFI_ENTRY_REGISTER = holy_multiboot_payload_eip;
  state_efi.MULTIBOOT_EFI_MBI_REGISTER = target;

  holy_relocator_efi_boot (rel, state_efi);
}
#else
#define holy_efi_is_finished	1
static void
efi_boot (struct holy_relocator *rel __attribute__ ((unused)),
	  holy_uint32_t target __attribute__ ((unused)))
{
}
#endif

#if defined (__i386__) || defined (__x86_64__)
static void
normal_boot (struct holy_relocator *rel, struct holy_relocator32_state state)
{
  holy_relocator32_boot (rel, state, 0);
}
#else
static void
normal_boot (struct holy_relocator *rel, struct holy_relocator32_state state)
{
  holy_relocator32_boot (rel, state);
}
#endif

static holy_err_t
holy_multiboot_boot (void)
{
  holy_err_t err;
  struct holy_relocator32_state state = MULTIBOOT_INITIAL_STATE;

  state.MULTIBOOT_ENTRY_REGISTER = holy_multiboot_payload_eip;

  err = holy_multiboot_make_mbi (&state.MULTIBOOT_MBI_REGISTER);

  if (err)
    return err;

  if (holy_efi_is_finished)
    normal_boot (holy_multiboot_relocator, state);
  else
    efi_boot (holy_multiboot_relocator, state.MULTIBOOT_MBI_REGISTER);

  /* Not reached.  */
  return holy_ERR_NONE;
}

static holy_err_t
holy_multiboot_unload (void)
{
  holy_multiboot_free_mbi ();

  holy_relocator_unload (holy_multiboot_relocator);
  holy_multiboot_relocator = NULL;

  holy_dl_unref (my_mod);

  return holy_ERR_NONE;
}

static holy_uint64_t highest_load;

#define MULTIBOOT_LOAD_ELF64
#include "multiboot_elfxx.c"
#undef MULTIBOOT_LOAD_ELF64

#define MULTIBOOT_LOAD_ELF32
#include "multiboot_elfxx.c"
#undef MULTIBOOT_LOAD_ELF32

/* Load ELF32 or ELF64.  */
holy_err_t
holy_multiboot_load_elf (mbi_load_data_t *mld)
{
  if (holy_multiboot_is_elf32 (mld->buffer))
    return holy_multiboot_load_elf32 (mld);
  else if (holy_multiboot_is_elf64 (mld->buffer))
    return holy_multiboot_load_elf64 (mld);

  return holy_error (holy_ERR_UNKNOWN_OS, N_("invalid arch-dependent ELF magic"));
}

holy_err_t
holy_multiboot_set_console (int console_type, int accepted_consoles,
			    int width, int height, int depth,
			    int console_req)
{
  console_required = console_req;
  if (!(accepted_consoles 
	& (holy_MULTIBOOT_CONSOLE_FRAMEBUFFER
	   | (holy_MACHINE_HAS_VGA_TEXT ? holy_MULTIBOOT_CONSOLE_EGA_TEXT : 0))))
    {
      if (console_required)
	return holy_error (holy_ERR_BAD_OS,
			   "OS requires a console but none is available");
      holy_puts_ (N_("WARNING: no console will be available to OS"));
      accepts_video = 0;
      accepts_ega_text = 0;
      return holy_ERR_NONE;
    }

  if (console_type == holy_MULTIBOOT_CONSOLE_FRAMEBUFFER)
    {
      char *buf;
      if (depth && width && height)
	buf = holy_xasprintf ("%dx%dx%d,%dx%d,auto", width,
			      height, depth, width, height);
      else if (width && height)
	buf = holy_xasprintf ("%dx%d,auto", width, height);
      else
	buf = holy_strdup ("auto");

      if (!buf)
	return holy_errno;
      holy_env_set ("gfxpayload", buf);
      holy_free (buf);
    }
  else
    {
#if holy_MACHINE_HAS_VGA_TEXT
      holy_env_set ("gfxpayload", "text");
#else
      /* Always use video if no VGA text is available.  */
      holy_env_set ("gfxpayload", "auto");
#endif
    }

  accepts_video = !!(accepted_consoles & holy_MULTIBOOT_CONSOLE_FRAMEBUFFER);
  accepts_ega_text = !!(accepted_consoles & holy_MULTIBOOT_CONSOLE_EGA_TEXT);
  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_multiboot (holy_command_t cmd __attribute__ ((unused)),
		    int argc, char *argv[])
{
  holy_file_t file = 0;
  holy_err_t err;

  holy_loader_unset ();

  highest_load = 0;

#ifndef holy_USE_MULTIBOOT2
  holy_multiboot_quirks = holy_MULTIBOOT_QUIRKS_NONE;
  int option_found = 0;

  do
    {
      option_found = 0;
      if (argc != 0 && holy_strcmp (argv[0], "--quirk-bad-kludge") == 0)
	{
	  argc--;
	  argv++;
	  option_found = 1;
	  holy_multiboot_quirks |= holy_MULTIBOOT_QUIRK_BAD_KLUDGE;
	}

      if (argc != 0 && holy_strcmp (argv[0], "--quirk-modules-after-kernel") == 0)
	{
	  argc--;
	  argv++;
	  option_found = 1;
	  holy_multiboot_quirks |= holy_MULTIBOOT_QUIRK_MODULES_AFTER_KERNEL;
	}
    } while (option_found);
#endif

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  file = holy_file_open (argv[0]);
  if (! file)
    return holy_errno;

  holy_dl_ref (my_mod);

  /* Skip filename.  */
  holy_multiboot_init_mbi (argc - 1, argv + 1);

  holy_relocator_unload (holy_multiboot_relocator);
  holy_multiboot_relocator = holy_relocator_new ();

  if (!holy_multiboot_relocator)
    goto fail;

  err = holy_multiboot_load (file, argv[0]);
  if (err)
    goto fail;

  holy_multiboot_set_bootdev ();

  holy_loader_set (holy_multiboot_boot, holy_multiboot_unload, 0);

 fail:
  if (file)
    holy_file_close (file);

  if (holy_errno != holy_ERR_NONE)
    {
      holy_relocator_unload (holy_multiboot_relocator);
      holy_multiboot_relocator = NULL;
      holy_dl_unref (my_mod);
    }

  return holy_errno;
}

static holy_err_t
holy_cmd_module (holy_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  holy_file_t file = 0;
  holy_ssize_t size;
  void *module = NULL;
  holy_addr_t target;
  holy_err_t err;
  int nounzip = 0;
  holy_uint64_t lowest_addr = 0;

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  if (holy_strcmp (argv[0], "--nounzip") == 0)
    {
      argv++;
      argc--;
      nounzip = 1;
    }

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  if (!holy_multiboot_relocator)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("you need to load the kernel first"));

  if (nounzip)
    holy_file_filter_disable_compression ();

  file = holy_file_open (argv[0]);
  if (! file)
    return holy_errno;

#ifndef holy_USE_MULTIBOOT2
  lowest_addr = 0x100000;
  if (holy_multiboot_quirks & holy_MULTIBOOT_QUIRK_MODULES_AFTER_KERNEL)
    lowest_addr = ALIGN_UP (highest_load + 1048576, 4096);
#endif

  size = holy_file_size (file);
  if (size)
  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_align (holy_multiboot_relocator, &ch,
					    lowest_addr, (0xffffffff - size) + 1,
					    size, MULTIBOOT_MOD_ALIGN,
					    holy_RELOCATOR_PREFERENCE_NONE, 1);
    if (err)
      {
	holy_file_close (file);
	return err;
      }
    module = get_virtual_current_address (ch);
    target = get_physical_target_address (ch);
  }
  else
    {
      module = 0;
      target = 0;
    }

  err = holy_multiboot_add_module (target, size, argc - 1, argv + 1);
  if (err)
    {
      holy_file_close (file);
      return err;
    }

  if (size && holy_file_read (file, module, size) != size)
    {
      holy_file_close (file);
      if (!holy_errno)
	holy_error (holy_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
		    argv[0]);
      return holy_errno;
    }

  holy_file_close (file);
  holy_tpm_measure (module, size, holy_BINARY_PCR, "holy_multiboot", argv[0]);
  holy_print_error();
  return holy_ERR_NONE;
}

static holy_command_t cmd_multiboot, cmd_module;

holy_MOD_INIT(multiboot)
{
  cmd_multiboot =
#ifdef holy_USE_MULTIBOOT2
    holy_register_command ("multiboot2", holy_cmd_multiboot,
			   0, N_("Load a multiboot 2 kernel."));
  cmd_module =
    holy_register_command ("module2", holy_cmd_module,
			   0, N_("Load a multiboot 2 module."));
#else
    holy_register_command ("multiboot", holy_cmd_multiboot,
			   0, N_("Load a multiboot kernel."));
  cmd_module =
    holy_register_command ("module", holy_cmd_module,
			   0, N_("Load a multiboot module."));
#endif

  my_mod = mod;
}

holy_MOD_FINI(multiboot)
{
  holy_unregister_command (cmd_multiboot);
  holy_unregister_command (cmd_module);
}
