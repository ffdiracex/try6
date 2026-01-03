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
#include <holy/command.h>
#include <holy/mips/relocator.h>
#include <holy/memory.h>
#include <holy/i18n.h>
#include <holy/lib/cmdline.h>
#include <holy/linux.h>

holy_MOD_LICENSE ("GPLv2+");

#pragma GCC diagnostic ignored "-Wcast-align"

/* For frequencies.  */
#include <holy/machine/time.h>

#ifdef holy_MACHINE_MIPS_LOONGSON
#include <holy/pci.h>
#include <holy/machine/kernel.h>

const char loongson_machtypes[][60] =
  {
    [holy_ARCH_MACHINE_YEELOONG] = "machtype=lemote-yeeloong-2f-8.9inches",
    [holy_ARCH_MACHINE_FULOONG2F]  = "machtype=lemote-fuloong-2f-box",
    [holy_ARCH_MACHINE_FULOONG2E]  = "machtype=lemote-fuloong-2e-unknown"
  };
#endif

static holy_dl_t my_mod;

static int loaded;

static holy_size_t linux_size;

static struct holy_relocator *relocator;
static holy_uint8_t *playground;
static holy_addr_t target_addr, entry_addr;
#ifdef holy_MACHINE_MIPS_QEMU_MIPS
static char *params;
#else
static int linux_argc;
static holy_off_t argv_off;
#ifdef holy_MACHINE_MIPS_LOONGSON
static holy_off_t envp_off;
#endif
static holy_off_t rd_addr_arg_off, rd_size_arg_off;
#endif
static int initrd_loaded = 0;

static holy_err_t
holy_linux_boot (void)
{
  struct holy_relocator32_state state;

  holy_memset (&state, 0, sizeof (state));

  /* Boot the kernel.  */
  state.gpr[1] = entry_addr;

#ifdef holy_MACHINE_MIPS_QEMU_MIPS
  {
    holy_err_t err;
    holy_relocator_chunk_t ch;
    holy_uint32_t *memsize;
    holy_uint32_t *magic;
    char *str;

    err = holy_relocator_alloc_chunk_addr (relocator, &ch,
					   ((16 << 20) - 264),
					   holy_strlen (params) + 1 + 8);
    if (err)
      return err;
    memsize = get_virtual_current_address (ch);
    magic = memsize + 1;
    *memsize = holy_mmap_get_lower ();
    *magic = 0x12345678;
    str = (char *) (magic + 1);
    holy_strcpy (str, params);
  }
#endif  

#ifndef holy_MACHINE_MIPS_QEMU_MIPS
  state.gpr[4] = linux_argc;
  state.gpr[5] = target_addr + argv_off;
#ifdef holy_MACHINE_MIPS_LOONGSON
  state.gpr[6] = target_addr + envp_off;
#else
  state.gpr[6] = 0;
#endif
  state.gpr[7] = 0;
#endif
  state.jumpreg = 1;
  holy_relocator32_boot (relocator, state);

  return holy_ERR_NONE;
}

static holy_err_t
holy_linux_unload (void)
{
  holy_relocator_unload (relocator);
  holy_dl_unref (my_mod);

#ifdef holy_MACHINE_MIPS_QEMU_MIPS
  holy_free (params);
  params = 0;
#endif

  loaded = 0;

  return holy_ERR_NONE;
}

static holy_err_t
holy_linux_load32 (holy_elf_t elf, const char *filename,
		   void **extra_mem, holy_size_t extra_size)
{
  Elf32_Addr base;
  int extraoff;
  holy_err_t err;

  /* Linux's entry point incorrectly contains a virtual address.  */
  entry_addr = elf->ehdr.ehdr32.e_entry;

  linux_size = holy_elf32_size (elf, &base, 0);
  if (linux_size == 0)
    return holy_errno;
  target_addr = base;
  /* Pad it; the kernel scribbles over memory beyond its load address.  */
  linux_size += 0x100000;
  linux_size = ALIGN_UP (base + linux_size, 4) - base;
  extraoff = linux_size;
  linux_size += extra_size;

  relocator = holy_relocator_new ();
  if (!relocator)
    return holy_errno;

  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (relocator, &ch,
					   target_addr & 0x1fffffff,
					   linux_size);
    if (err)
      return err;
    playground = get_virtual_current_address (ch);
  }

  *extra_mem = playground + extraoff;

  /* Now load the segments into the area we claimed.  */
  return holy_elf32_load (elf, filename, playground - base, holy_ELF_LOAD_FLAGS_NONE, 0, 0);
}

static holy_err_t
holy_linux_load64 (holy_elf_t elf, const char *filename,
		   void **extra_mem, holy_size_t extra_size)
{
  Elf64_Addr base;
  int extraoff;
  holy_err_t err;

  /* Linux's entry point incorrectly contains a virtual address.  */
  entry_addr = elf->ehdr.ehdr64.e_entry;

  linux_size = holy_elf64_size (elf, &base, 0);
  if (linux_size == 0)
    return holy_errno;
  target_addr = base;
  /* Pad it; the kernel scribbles over memory beyond its load address.  */
  linux_size += 0x100000;
  linux_size = ALIGN_UP (base + linux_size, 4) - base;
  extraoff = linux_size;
  linux_size += extra_size;

  relocator = holy_relocator_new ();
  if (!relocator)
    return holy_errno;

  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (relocator, &ch,
					   target_addr & 0x1fffffff,
					   linux_size);
    if (err)
      return err;
    playground = get_virtual_current_address (ch);
  }

  *extra_mem = playground + extraoff;

  /* Now load the segments into the area we claimed.  */
  return holy_elf64_load (elf, filename, playground - base, holy_ELF_LOAD_FLAGS_NONE, 0, 0);
}

static holy_err_t
holy_cmd_linux (holy_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  holy_elf_t elf = 0;
  int size;
  void *extra = NULL;
#ifndef holy_MACHINE_MIPS_QEMU_MIPS
  int i;
  holy_uint32_t *linux_argv;
  char *linux_args;
#endif
  holy_err_t err;
#ifdef holy_MACHINE_MIPS_LOONGSON
  char *linux_envs;
  holy_uint32_t *linux_envp;
#endif

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  elf = holy_elf_open (argv[0]);
  if (! elf)
    return holy_errno;

  if (elf->ehdr.ehdr32.e_type != ET_EXEC)
    {
      holy_elf_close (elf);
      return holy_error (holy_ERR_UNKNOWN_OS,
			 N_("this ELF file is not of the right type"));
    }

  /* Release the previously used memory.  */
  holy_loader_unset ();
  loaded = 0;

#ifdef holy_MACHINE_MIPS_QEMU_MIPS
  size = 0;
#else
  /* For arguments.  */
  linux_argc = argc;
#ifdef holy_MACHINE_MIPS_LOONGSON
  linux_argc++;
#endif
  /* Main arguments.  */
  size = (linux_argc) * sizeof (holy_uint32_t);
  /* Initrd address and size.  */
  size += 2 * sizeof (holy_uint32_t);
  /* NULL terminator.  */
  size += sizeof (holy_uint32_t);

  /* First argument is always "a0".  */
  size += ALIGN_UP (sizeof ("a0"), 4);
  /* Normal arguments.  */
  for (i = 1; i < argc; i++)
    size += ALIGN_UP (holy_strlen (argv[i]) + 1, 4);
#ifdef holy_MACHINE_MIPS_LOONGSON
  size += ALIGN_UP (sizeof (loongson_machtypes[0]), 4);
#endif

  /* rd arguments.  */
  size += ALIGN_UP (sizeof ("rd_start=0xXXXXXXXXXXXXXXXX"), 4);
  size += ALIGN_UP (sizeof ("rd_size=0xXXXXXXXXXXXXXXXX"), 4);

  /* For the environment.  */
  size += sizeof (holy_uint32_t);
  size += 4 * sizeof (holy_uint32_t);
  size += ALIGN_UP (sizeof ("memsize=XXXXXXXXXXXXXXXXXXXX"), 4)
    + ALIGN_UP (sizeof ("highmemsize=XXXXXXXXXXXXXXXXXXXX"), 4)
    + ALIGN_UP (sizeof ("busclock=XXXXXXXXXX"), 4)
    + ALIGN_UP (sizeof ("cpuclock=XXXXXXXXXX"), 4);
#endif

  if (holy_elf_is_elf32 (elf))
    err = holy_linux_load32 (elf, argv[0], &extra, size);
  else
  if (holy_elf_is_elf64 (elf))
    err = holy_linux_load64 (elf, argv[0], &extra, size);
  else
    err = holy_error (holy_ERR_BAD_OS, N_("invalid arch-dependent ELF magic"));

  holy_elf_close (elf);

  if (err)
    return err;

#ifdef holy_MACHINE_MIPS_QEMU_MIPS
  /* Create kernel command line.  */
  size = holy_loader_cmdline_size(argc, argv);
  params = holy_malloc (size + sizeof (LINUX_IMAGE));
  if (! params)
    {
      holy_linux_unload ();
      return holy_errno;
    }

  holy_memcpy (params, LINUX_IMAGE, sizeof (LINUX_IMAGE));
  holy_create_loader_cmdline (argc, argv, params + sizeof (LINUX_IMAGE) - 1,
			      size);
#else
  linux_argv = extra;
  argv_off = (holy_uint8_t *) linux_argv - (holy_uint8_t *) playground;
  extra = linux_argv + (linux_argc + 1 + 2);
  linux_args = extra;

  holy_memcpy (linux_args, "a0", sizeof ("a0"));
  *linux_argv = (holy_uint8_t *) linux_args - (holy_uint8_t *) playground
    + target_addr;
  linux_argv++;
  linux_args += ALIGN_UP (sizeof ("a0"), 4);

#ifdef holy_MACHINE_MIPS_LOONGSON
  {
    unsigned mtype = holy_arch_machine;
    if (mtype >= ARRAY_SIZE (loongson_machtypes))
      mtype = 0;
    /* In Loongson platform, it is the responsibility of the bootloader/firmware
       to supply the OS kernel with machine type information.  */
    holy_memcpy (linux_args, loongson_machtypes[mtype],
		 sizeof (loongson_machtypes[mtype]));
    *linux_argv = (holy_uint8_t *) linux_args - (holy_uint8_t *) playground
      + target_addr;
    linux_argv++;
    linux_args += ALIGN_UP (sizeof (loongson_machtypes[mtype]), 4);
  }
#endif

  for (i = 1; i < argc; i++)
    {
      holy_memcpy (linux_args, argv[i], holy_strlen (argv[i]) + 1);
      *linux_argv = (holy_uint8_t *) linux_args - (holy_uint8_t *) playground
	+ target_addr;
      linux_argv++;
      linux_args += ALIGN_UP (holy_strlen (argv[i]) + 1, 4);
    }

  /* Reserve space for rd arguments.  */
  rd_addr_arg_off = (holy_uint8_t *) linux_args - (holy_uint8_t *) playground;
  linux_args += ALIGN_UP (sizeof ("rd_start=0xXXXXXXXXXXXXXXXX"), 4);
  *linux_argv = 0;
  linux_argv++;

  rd_size_arg_off = (holy_uint8_t *) linux_args - (holy_uint8_t *) playground;
  linux_args += ALIGN_UP (sizeof ("rd_size=0xXXXXXXXXXXXXXXXX"), 4);
  *linux_argv = 0;
  linux_argv++;

  *linux_argv = 0;

  extra = linux_args;

#ifdef holy_MACHINE_MIPS_LOONGSON
  linux_envp = extra;
  envp_off = (holy_uint8_t *) linux_envp - (holy_uint8_t *) playground;
  linux_envs = (char *) (linux_envp + 5);
  holy_snprintf (linux_envs, sizeof ("memsize=XXXXXXXXXXXXXXXXXXXX"),
		 "memsize=%lld",
		 (unsigned long long) holy_mmap_get_lower () >> 20);
  linux_envp[0] = (holy_uint8_t *) linux_envs - (holy_uint8_t *) playground
    + target_addr;
  linux_envs += ALIGN_UP (holy_strlen (linux_envs) + 1, 4);
  holy_snprintf (linux_envs, sizeof ("highmemsize=XXXXXXXXXXXXXXXXXXXX"),
		 "highmemsize=%lld",
		 (unsigned long long) holy_mmap_get_upper () >> 20);
  linux_envp[1] = (holy_uint8_t *) linux_envs - (holy_uint8_t *) playground
    + target_addr;
  linux_envs += ALIGN_UP (holy_strlen (linux_envs) + 1, 4);

  holy_snprintf (linux_envs, sizeof ("busclock=XXXXXXXXXX"),
		 "busclock=%d", holy_arch_busclock);
  linux_envp[2] = (holy_uint8_t *) linux_envs - (holy_uint8_t *) playground
    + target_addr;
  linux_envs += ALIGN_UP (holy_strlen (linux_envs) + 1, 4);
  holy_snprintf (linux_envs, sizeof ("cpuclock=XXXXXXXXXX"),
		 "cpuclock=%d", holy_arch_cpuclock);
  linux_envp[3] = (holy_uint8_t *) linux_envs - (holy_uint8_t *) playground
    + target_addr;
  linux_envs += ALIGN_UP (holy_strlen (linux_envs) + 1, 4);

  linux_envp[4] = 0;
#endif
#endif

  holy_loader_set (holy_linux_boot, holy_linux_unload, 1);
  initrd_loaded = 0;
  loaded = 1;
  holy_dl_ref (my_mod);

  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_initrd (holy_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  holy_size_t size = 0;
  void *initrd_src;
  holy_addr_t initrd_dest;
  holy_err_t err;
  struct holy_linux_initrd_context initrd_ctx = { 0, 0, 0 };

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  if (!loaded)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("you need to load the kernel first"));

  if (initrd_loaded)
    return holy_error (holy_ERR_BAD_ARGUMENT, "only one initrd command can be issued.");

  if (holy_initrd_init (argc, argv, &initrd_ctx))
    goto fail;

  size = holy_get_initrd_size (&initrd_ctx);

  {
    holy_relocator_chunk_t ch;

    err = holy_relocator_alloc_chunk_align (relocator, &ch,
					    (target_addr & 0x1fffffff)
					    + linux_size + 0x10000,
					    (0x10000000 - size),
					    size, 0x10000,
					    holy_RELOCATOR_PREFERENCE_NONE, 0);

    if (err)
      goto fail;
    initrd_src = get_virtual_current_address (ch);
    initrd_dest = get_physical_target_address (ch) | 0x80000000;
  }

  if (holy_initrd_load (&initrd_ctx, argv, initrd_src))
    goto fail;

#ifdef holy_MACHINE_MIPS_QEMU_MIPS
  {
    char *tmp;
    tmp = holy_xasprintf ("%s rd_start=0x%" PRIxholy_ADDR
			  " rd_size=0x%" PRIxholy_ADDR, params,
			  initrd_dest, size);
    if (!tmp)
      goto fail;
    holy_free (params);
    params = tmp;
  }
#else
  holy_snprintf ((char *) playground + rd_addr_arg_off,
		 sizeof ("rd_start=0xXXXXXXXXXXXXXXXX"), "rd_start=0x%llx",
		(unsigned long long) initrd_dest);
  ((holy_uint32_t *) (playground + argv_off))[linux_argc]
    = target_addr + rd_addr_arg_off;
  linux_argc++;

  holy_snprintf ((char *) playground + rd_size_arg_off,
		sizeof ("rd_size=0xXXXXXXXXXXXXXXXXX"), "rd_size=0x%llx",
		(unsigned long long) size);
  ((holy_uint32_t *) (playground + argv_off))[linux_argc]
    = target_addr + rd_size_arg_off;
  linux_argc++;
#endif

  initrd_loaded = 1;

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
