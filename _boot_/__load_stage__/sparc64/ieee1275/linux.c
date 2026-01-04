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
#include <holy/linux.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_dl_t my_mod;

static int loaded;

/* /virtual-memory/translations property layout  */
struct holy_ieee1275_translation {
  holy_uint64_t vaddr;
  holy_uint64_t size;
  holy_uint64_t data;
};

static struct holy_ieee1275_translation *of_trans;
static int of_num_trans;

static holy_addr_t phys_base;
static holy_addr_t holy_phys_start;
static holy_addr_t holy_phys_end;

static holy_addr_t initrd_addr;
static holy_addr_t initrd_paddr;
static holy_size_t initrd_size;

static Elf64_Addr linux_entry;
static holy_addr_t linux_addr;
static holy_addr_t linux_paddr;
static holy_size_t linux_size;

static char *linux_args;

struct linux_bootstr_info {
	int len, valid;
	char buf[];
};

struct linux_hdrs {
	/* All HdrS versions support these fields.  */
	unsigned int start_insns[2];
	char magic[4]; /* "HdrS" */
	unsigned int linux_kernel_version; /* LINUX_VERSION_CODE */
	unsigned short hdrs_version;
	unsigned short root_flags;
	unsigned short root_dev;
	unsigned short ram_flags;
	unsigned int __deprecated_ramdisk_image;
	unsigned int ramdisk_size;

	/* HdrS versions 0x0201 and higher only */
	char *reboot_command;

	/* HdrS versions 0x0202 and higher only */
	struct linux_bootstr_info *bootstr_info;

	/* HdrS versions 0x0301 and higher only */
	unsigned long ramdisk_image;
};

static holy_err_t
holy_linux_boot (void)
{
  struct linux_bootstr_info *bp;
  struct linux_hdrs *hp;
  holy_addr_t addr;

  hp = (struct linux_hdrs *) linux_addr;

  /* Any pointer we dereference in the kernel image must be relocated
     to where we actually loaded the kernel.  */
  addr = (holy_addr_t) hp->bootstr_info;
  addr += (linux_addr - linux_entry);
  bp = (struct linux_bootstr_info *) addr;

  /* Set the command line arguments, unless the kernel has been
     built with a fixed CONFIG_CMDLINE.  */
  if (!bp->valid)
    {
      int len = holy_strlen (linux_args) + 1;
      if (bp->len < len)
	len = bp->len;
      holy_memcpy(bp->buf, linux_args, len);
      bp->buf[len-1] = '\0';
      bp->valid = 1;
    }

  if (initrd_addr)
    {
      /* The kernel expects the physical address, adjusted relative
	 to the lowest address advertised in "/memory"'s available
	 property.

	 The history of this is that back when the kernel only supported
	 specifying a 32-bit ramdisk address, this was the way to still
	 be able to specify the ramdisk physical address even if memory
	 started at some place above 4GB.

	 The magic 0x400000 is KERNBASE, I have no idea why SILO adds
	 that term into the address, but it does and thus we have to do
	 it too as this is what the kernel expects.  */
      hp->ramdisk_image = initrd_paddr - phys_base + 0x400000;
      hp->ramdisk_size = initrd_size;
    }

  holy_dprintf ("loader", "Entry point: 0x%lx\n", linux_addr);
  holy_dprintf ("loader", "Initrd at: 0x%lx, size 0x%lx\n", initrd_addr,
		initrd_size);
  holy_dprintf ("loader", "Boot arguments: %s\n", linux_args);
  holy_dprintf ("loader", "Jumping to Linux...\n");

  /* Boot the kernel.  */
  asm volatile ("ldx	%0, %%o4\n"
		"ldx	%1, %%o6\n"
		"ldx	%2, %%o5\n"
		"mov    %%g0, %%o0\n"
		"mov    %%g0, %%o2\n"
		"mov    %%g0, %%o3\n"
		"jmp    %%o5\n"
	        "mov    %%g0, %%o1\n": :
		"m"(holy_ieee1275_entry_fn),
		"m"(holy_ieee1275_original_stack),
		"m"(linux_addr));

  return holy_ERR_NONE;
}

static holy_err_t
holy_linux_release_mem (void)
{
  holy_free (linux_args);
  linux_args = 0;
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

#define FOUR_MB	(4 * 1024 * 1024)

/* Context for alloc_phys.  */
struct alloc_phys_ctx
{
  holy_addr_t size;
  holy_addr_t ret;
};

/* Helper for alloc_phys.  */
static int
alloc_phys_choose (holy_uint64_t addr, holy_uint64_t len,
		   holy_memory_type_t type, void *data)
{
  struct alloc_phys_ctx *ctx = data;
  holy_addr_t end = addr + len;

  if (type != holy_MEMORY_AVAILABLE)
    return 0;

  addr = ALIGN_UP (addr, FOUR_MB);
  if (addr + ctx->size >= end)
    return 0;

  /* OBP available region contains holy. Start at holy_phys_end. */
  /* holy_phys_start does not start at the beginning of the memory region */
  if ((holy_phys_start >= addr && holy_phys_end < end) ||
      (addr > holy_phys_start && addr < holy_phys_end))
    {
      addr = ALIGN_UP (holy_phys_end, FOUR_MB);
      if (addr + ctx->size >= end)
	return 0;
    }

  holy_dprintf("loader",
    "addr = 0x%lx holy_phys_start = 0x%lx holy_phys_end = 0x%lx\n",
    addr, holy_phys_start, holy_phys_end);

  if (loaded)
    {
      holy_addr_t linux_end = ALIGN_UP (linux_paddr + linux_size, FOUR_MB);

      if (addr >= linux_paddr && addr < linux_end)
	{
	  addr = linux_end;
	  if (addr + ctx->size >= end)
	    return 0;
	}
      if ((addr + ctx->size) >= linux_paddr
	  && (addr + ctx->size) < linux_end)
	{
	  addr = linux_end;
	  if (addr + ctx->size >= end)
	    return 0;
	}
    }

  ctx->ret = addr;
  return 1;
}

static holy_addr_t
alloc_phys (holy_addr_t size)
{
  struct alloc_phys_ctx ctx = {
    .size = size,
    .ret = (holy_addr_t) -1
  };

  holy_machine_mmap_iterate (alloc_phys_choose, &ctx);

  return ctx.ret;
}

static holy_err_t
holy_linux_load64 (holy_elf_t elf, const char *filename)
{
  holy_addr_t off, paddr, base;
  int ret;

  linux_entry = elf->ehdr.ehdr64.e_entry;
  linux_addr = 0x40004000;
  off = 0x4000;
  linux_size = holy_elf64_size (elf, 0, 0);
  if (linux_size == 0)
    return holy_errno;

  holy_dprintf ("loader", "Attempting to claim at 0x%lx, size 0x%lx.\n",
		linux_addr, linux_size);

  paddr = alloc_phys (linux_size + off);
  if (paddr == (holy_addr_t) -1)
    return holy_error (holy_ERR_OUT_OF_MEMORY,
		       "couldn't allocate physical memory");
  ret = holy_ieee1275_map (paddr, linux_addr - off,
			   linux_size + off, IEEE1275_MAP_DEFAULT);
  if (ret)
    return holy_error (holy_ERR_OUT_OF_MEMORY,
		       "couldn't map physical memory");

  holy_dprintf ("loader", "Loading Linux at vaddr 0x%lx, paddr 0x%lx, size 0x%lx\n",
		linux_addr, paddr, linux_size);

  linux_paddr = paddr;

  base = linux_entry - off;

  /* Now load the segments into the area we claimed.  */
  return holy_elf64_load (elf, filename, (void *) (linux_addr - off - base), holy_ELF_LOAD_FLAGS_NONE, 0, 0);
}

static holy_err_t
holy_cmd_linux (holy_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  holy_file_t file = 0;
  holy_elf_t elf = 0;
  int size;

  holy_dl_ref (my_mod);

  if (argc == 0)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto out;
    }

  file = holy_file_open (argv[0]);
  if (!file)
    goto out;

  elf = holy_elf_file (file, argv[0]);
  if (! elf)
    goto out;

  if (elf->ehdr.ehdr32.e_type != ET_EXEC)
    {
      holy_error (holy_ERR_UNKNOWN_OS,
		  N_("this ELF file is not of the right type"));
      goto out;
    }

  /* Release the previously used memory.  */
  holy_loader_unset ();

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
  else if (file)
    holy_file_close (file);

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
  holy_addr_t paddr;
  holy_addr_t addr;
  int ret;
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

  addr = 0x60000000;

  paddr = alloc_phys (size);
  if (paddr == (holy_addr_t) -1)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY,
		  "couldn't allocate physical memory");
      goto fail;
    }
  ret = holy_ieee1275_map (paddr, addr, size, IEEE1275_MAP_DEFAULT);
  if (ret)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY,
		  "couldn't map physical memory");
      goto fail;
    }

  holy_dprintf ("loader", "Loading initrd at vaddr 0x%lx, paddr 0x%lx, size 0x%lx\n",
		addr, paddr, size);

  if (holy_initrd_load (&initrd_ctx, argv, (void *) addr))
    goto fail;

  initrd_addr = addr;
  initrd_paddr = paddr;
  initrd_size = size;

 fail:
  holy_initrd_close (&initrd_ctx);

  return holy_errno;
}

/* Helper for determine_phys_base.  */
static int
get_physbase (holy_uint64_t addr, holy_uint64_t len __attribute__ ((unused)),
	      holy_memory_type_t type, void *data __attribute__ ((unused)))
{
  if (type != holy_MEMORY_AVAILABLE)
    return 0;
  if (addr < phys_base)
    phys_base = addr;
  return 0;
}

static void
determine_phys_base (void)
{
  phys_base = ~(holy_uint64_t) 0;
  holy_machine_mmap_iterate (get_physbase, NULL);
}

static void
fetch_translations (void)
{
  holy_ieee1275_phandle_t node;
  holy_ssize_t actual;
  int i;

  if (holy_ieee1275_finddevice ("/virtual-memory", &node))
    {
      holy_printf ("Cannot find /virtual-memory node.\n");
      return;
    }

  if (holy_ieee1275_get_property_length (node, "translations", &actual))
    {
      holy_printf ("Cannot find /virtual-memory/translations size.\n");
      return;
    }

  of_trans = holy_malloc (actual);
  if (!of_trans)
    {
      holy_printf ("Cannot allocate translations buffer.\n");
      return;
    }

  if (holy_ieee1275_get_property (node, "translations", of_trans, actual, &actual))
    {
      holy_printf ("Cannot fetch /virtual-memory/translations property.\n");
      return;
    }

  of_num_trans = actual / sizeof(struct holy_ieee1275_translation);

  for (i = 0; i < of_num_trans; i++)
    {
      struct holy_ieee1275_translation *p = &of_trans[i];

      if (p->vaddr == 0x2000)
	{
	  holy_addr_t phys, tte = p->data;

	  phys = tte & ~(0xff00000000001fffULL);

	  holy_phys_start = phys;
	  holy_phys_end = holy_phys_start + p->size;
	  holy_dprintf ("loader", "holy lives at phys_start[%lx] phys_end[%lx]\n",
			(unsigned long) holy_phys_start,
			(unsigned long) holy_phys_end);
	  break;
	}
    }
}


static holy_command_t cmd_linux, cmd_initrd;

holy_MOD_INIT(linux)
{
  determine_phys_base ();
  fetch_translations ();

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
