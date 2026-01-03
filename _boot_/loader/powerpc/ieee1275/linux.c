/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/elf.h>
#include <holy/elfload.h>
#include <holy/loader.h>
#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/ieee1275/ieee1275.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/memory.h>
#include <holy/lib/cmdline.h>
#include <holy/cache.h>
#include <holy/linux.h>

holy_MOD_LICENSE ("GPLv2+");

#define ELF32_LOADMASK (0xc0000000UL)
#define ELF64_LOADMASK (0xc000000000000000ULL)

static holy_dl_t my_mod;

static int loaded;

static holy_addr_t initrd_addr;
static holy_size_t initrd_size;

static holy_addr_t linux_addr;
static holy_addr_t linux_entry;
static holy_size_t linux_size;

static char *linux_args;

typedef void (*kernel_entry_t) (void *, unsigned long, int (void *),
				unsigned long, unsigned long);

/* Context for holy_linux_claimmap_iterate.  */
struct holy_linux_claimmap_iterate_ctx
{
  holy_addr_t target;
  holy_size_t size;
  holy_size_t align;
  holy_addr_t found_addr;
};

/* Helper for holy_linux_claimmap_iterate.  */
static int
alloc_mem (holy_uint64_t addr, holy_uint64_t len, holy_memory_type_t type,
	   void *data)
{
  struct holy_linux_claimmap_iterate_ctx *ctx = data;

  holy_uint64_t end = addr + len;
  addr = ALIGN_UP (addr, ctx->align);
  ctx->target = ALIGN_UP (ctx->target, ctx->align);

  /* Target above the memory chunk.  */
  if (type != holy_MEMORY_AVAILABLE || ctx->target > end)
    return 0;

  /* Target inside the memory chunk.  */
  if (ctx->target >= addr && ctx->target < end &&
      ctx->size <= end - ctx->target)
    {
      if (holy_claimmap (ctx->target, ctx->size) == holy_ERR_NONE)
	{
	  ctx->found_addr = ctx->target;
	  return 1;
	}
      holy_print_error ();
    }
  /* Target below the memory chunk.  */
  if (ctx->target < addr && addr + ctx->size <= end)
    {
      if (holy_claimmap (addr, ctx->size) == holy_ERR_NONE)
	{
	  ctx->found_addr = addr;
	  return 1;
	}
      holy_print_error ();
    }
  return 0;
}

static holy_addr_t
holy_linux_claimmap_iterate (holy_addr_t target, holy_size_t size,
			     holy_size_t align)
{
  struct holy_linux_claimmap_iterate_ctx ctx = {
    .target = target,
    .size = size,
    .align = align,
    .found_addr = (holy_addr_t) -1
  };

  if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_FORCE_CLAIM))
    {
      holy_uint64_t addr = target;
      if (addr < holy_IEEE1275_STATIC_HEAP_START
	  + holy_IEEE1275_STATIC_HEAP_LEN)
	addr = holy_IEEE1275_STATIC_HEAP_START
	  + holy_IEEE1275_STATIC_HEAP_LEN;
      addr = ALIGN_UP (addr, align);
      if (holy_claimmap (addr, size) == holy_ERR_NONE)
	return addr;
      return (holy_addr_t) -1;
    }
	

  holy_machine_mmap_iterate (alloc_mem, &ctx);

  return ctx.found_addr;
}

static holy_err_t
holy_linux_boot (void)
{
  kernel_entry_t linuxmain;
  holy_ssize_t actual;

  holy_arch_sync_caches ((void *) linux_addr, linux_size);
  /* Set the command line arguments.  */
  holy_ieee1275_set_property (holy_ieee1275_chosen, "bootargs", linux_args,
			      holy_strlen (linux_args) + 1, &actual);

  holy_dprintf ("loader", "Entry point: 0x%x\n", linux_entry);
  holy_dprintf ("loader", "Initrd at: 0x%x, size 0x%x\n", initrd_addr,
		initrd_size);
  holy_dprintf ("loader", "Boot arguments: %s\n", linux_args);
  holy_dprintf ("loader", "Jumping to Linux...\n");

  /* Boot the kernel.  */
  linuxmain = (kernel_entry_t) linux_entry;
  linuxmain ((void *) initrd_addr, initrd_size, holy_ieee1275_entry_fn, 0, 0);

  return holy_ERR_NONE;
}

static holy_err_t
holy_linux_release_mem (void)
{
  holy_free (linux_args);
  linux_args = 0;

  if (linux_addr && holy_ieee1275_release (linux_addr, linux_size))
    return holy_error (holy_ERR_OUT_OF_MEMORY, "cannot release memory");

  if (initrd_addr && holy_ieee1275_release (initrd_addr, initrd_size))
    return holy_error (holy_ERR_OUT_OF_MEMORY, "cannot release memory");

  linux_addr = 0;
  initrd_addr = 0;

  return holy_ERR_NONE;
}

static holy_err_t
holy_linux_unload (void)
{
  holy_err_t err;

  err = holy_linux_release_mem ();
  holy_dl_unref (my_mod);

  loaded = 0;

  return err;
}

static holy_err_t
holy_linux_load32 (holy_elf_t elf, const char *filename)
{
  Elf32_Addr base_addr;
  holy_addr_t seg_addr;
  holy_uint32_t align;
  holy_uint32_t offset;
  Elf32_Addr entry;

  linux_size = holy_elf32_size (elf, &base_addr, &align);
  if (linux_size == 0)
    return holy_errno;
  /* Pad it; the kernel scribbles over memory beyond its load address.  */
  linux_size += 0x100000;

  /* Linux's entry point incorrectly contains a virtual address.  */
  entry = elf->ehdr.ehdr32.e_entry & ~ELF32_LOADMASK;

  /* Linux's incorrectly contains a virtual address.  */
  base_addr &= ~ELF32_LOADMASK;
  offset = entry - base_addr;

  /* On some systems, firmware occupies the memory we're trying to use.
   * Happily, Linux can be loaded anywhere (it relocates itself).  Iterate
   * until we find an open area.  */
  seg_addr = holy_linux_claimmap_iterate (base_addr & ~ELF32_LOADMASK, linux_size, align);
  if (seg_addr == (holy_addr_t) -1)
    return holy_error (holy_ERR_OUT_OF_MEMORY, "couldn't claim memory");

  linux_entry = seg_addr + offset;
  linux_addr = seg_addr;

  /* Now load the segments into the area we claimed.  */
  return holy_elf32_load (elf, filename, (void *) (seg_addr - base_addr), holy_ELF_LOAD_FLAGS_30BITS, 0, 0);
}

static holy_err_t
holy_linux_load64 (holy_elf_t elf, const char *filename)
{
  Elf64_Addr base_addr;
  holy_addr_t seg_addr;
  holy_uint64_t align;
  holy_uint64_t offset;
  Elf64_Addr entry;

  linux_size = holy_elf64_size (elf, &base_addr, &align);
  if (linux_size == 0)
    return holy_errno;
  /* Pad it; the kernel scribbles over memory beyond its load address.  */
  linux_size += 0x100000;

  base_addr &= ~ELF64_LOADMASK;
  entry = elf->ehdr.ehdr64.e_entry & ~ELF64_LOADMASK;
  offset = entry - base_addr;
  /* Linux's incorrectly contains a virtual address.  */

  /* On some systems, firmware occupies the memory we're trying to use.
   * Happily, Linux can be loaded anywhere (it relocates itself).  Iterate
   * until we find an open area.  */
  seg_addr = holy_linux_claimmap_iterate (base_addr & ~ELF64_LOADMASK, linux_size, align);
  if (seg_addr == (holy_addr_t) -1)
    return holy_error (holy_ERR_OUT_OF_MEMORY, "couldn't claim memory");

  linux_entry = seg_addr + offset;
  linux_addr = seg_addr;

  /* Now load the segments into the area we claimed.  */
  return holy_elf64_load (elf, filename, (void *) (holy_addr_t) (seg_addr - base_addr), holy_ELF_LOAD_FLAGS_62BITS, 0, 0);
}

static holy_err_t
holy_cmd_linux (holy_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  holy_elf_t elf = 0;
  int size;

  holy_dl_ref (my_mod);

  if (argc == 0)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto out;
    }

  elf = holy_elf_open (argv[0]);
  if (! elf)
    goto out;

  if (elf->ehdr.ehdr32.e_type != ET_EXEC && elf->ehdr.ehdr32.e_type != ET_DYN)
    {
      holy_error (holy_ERR_UNKNOWN_OS,
		  N_("this ELF file is not of the right type"));
      goto out;
    }

  /* Release the previously used memory.  */
  holy_loader_unset ();

  if (holy_elf_is_elf32 (elf))
    holy_linux_load32 (elf, argv[0]);
  else
  if (holy_elf_is_elf64 (elf))
    holy_linux_load64 (elf, argv[0]);
  else
    {
      holy_error (holy_ERR_BAD_FILE_TYPE, N_("invalid arch-dependent ELF magic"));
      goto out;
    }

  size = holy_loader_cmdline_size(argc, argv);
  linux_args = holy_malloc (size + sizeof (LINUX_IMAGE));
  if (! linux_args)
    goto out;

  /* Create kernel command line.  */
  holy_memcpy (linux_args, LINUX_IMAGE, sizeof (LINUX_IMAGE));
  holy_create_loader_cmdline (argc, argv, linux_args + sizeof (LINUX_IMAGE) - 1,
			      size);

out:

  if (elf)
    holy_elf_close (elf);

  if (holy_errno != holy_ERR_NONE)
    {
      holy_linux_release_mem ();
      holy_dl_unref (my_mod);
      loaded = 0;
    }
  else
    {
      holy_loader_set (holy_linux_boot, holy_linux_unload, 1);
      initrd_addr = 0;
      loaded = 1;
    }

  return holy_errno;
}

static holy_err_t
holy_cmd_initrd (holy_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  holy_size_t size = 0;
  holy_addr_t first_addr;
  holy_addr_t addr;
  struct holy_linux_initrd_context initrd_ctx = { 0, 0, 0 };

  if (argc == 0)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto fail;
    }

  if (!loaded)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("you need to load the kernel first"));
      goto fail;
    }

  if (holy_initrd_init (argc, argv, &initrd_ctx))
    goto fail;

  size = holy_get_initrd_size (&initrd_ctx);

  first_addr = linux_addr + linux_size;

  /* Attempt to claim at a series of addresses until successful in
     the same way that holy_rescue_cmd_linux does.  */
  addr = holy_linux_claimmap_iterate (first_addr, size, 0x100000);
  if (addr == (holy_addr_t) -1)
     goto fail;

  holy_dprintf ("loader", "Loading initrd at 0x%x, size 0x%x\n", addr, size);

  if (holy_initrd_load (&initrd_ctx, argv, (void *) addr))
    goto fail;

  initrd_addr = addr;
  initrd_size = size;

 fail:
  holy_initrd_close (&initrd_ctx);

  return holy_errno;
}

static holy_command_t cmd_linux, cmd_initrd;

holy_MOD_INIT(linux)
{
  cmd_linux = holy_register_command ("linux", holy_cmd_linux,
				     0, N_("Load Linux."));
  cmd_initrd = holy_register_command ("initrd", holy_cmd_initrd,
				      0, N_("Load initrd."));
  my_mod = mod;
}

holy_MOD_FINI(linux)
{
  holy_unregister_command (cmd_linux);
  holy_unregister_command (cmd_initrd);
}
