/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/loader.h>
#include <holy/file.h>
#include <holy/err.h>
#include <holy/device.h>
#include <holy/disk.h>
#include <holy/misc.h>
#include <holy/types.h>
#include <holy/memory.h>
#include <holy/dl.h>
#include <holy/cpu/linux.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/mm.h>
#include <holy/cpu/relocator.h>
#include <holy/video.h>
#include <holy/i386/floppy.h>
#include <holy/lib/cmdline.h>
#include <holy/linux.h>
#include <holy/tpm.h>

holy_MOD_LICENSE ("GPLv2+");

#define holy_LINUX_CL_OFFSET		0x9000

static holy_dl_t my_mod;

static holy_size_t linux_mem_size;
static int loaded;
static struct holy_relocator *relocator = NULL;
static holy_addr_t holy_linux_real_target;
static char *holy_linux_real_chunk;
static holy_size_t holy_linux16_prot_size;
static holy_size_t maximal_cmdline_size;

static holy_err_t
holy_linux16_boot (void)
{
  holy_uint16_t segment;
  struct holy_relocator16_state state;

  segment = holy_linux_real_target >> 4;
  state.gs = state.fs = state.es = state.ds = state.ss = segment;
  state.sp = holy_LINUX_SETUP_STACK;
  state.cs = segment + 0x20;
  state.ip = 0;
  state.a20 = 1;

  holy_video_set_mode ("text", 0, 0);

  holy_stop_floppy ();
  
  return holy_relocator16_boot (relocator, state);
}

static holy_err_t
holy_linux_unload (void)
{
  holy_dl_unref (my_mod);
  loaded = 0;
  holy_relocator_unload (relocator);
  relocator = NULL;
  return holy_ERR_NONE;
}

static int
target_hook (holy_uint64_t addr, holy_uint64_t size, holy_memory_type_t type,
	    void *data)
{
  holy_uint64_t *result = data;
  holy_uint64_t candidate;

  if (type != holy_MEMORY_AVAILABLE)
    return 0;
  if (addr >= 0xa0000)
    return 0;
  if (addr + size >= 0xa0000)
    size = 0xa0000 - addr;

  /* Put the real mode part at as a high location as possible.  */
  candidate = addr + size - (holy_LINUX_CL_OFFSET + maximal_cmdline_size);
  /* But it must not exceed the traditional area.  */
  if (candidate > holy_LINUX_OLD_REAL_MODE_ADDR)
    candidate = holy_LINUX_OLD_REAL_MODE_ADDR;
  if (candidate < addr)
    return 0;

  if (candidate > *result || *result == (holy_uint64_t) -1)
    *result = candidate;
  return 0;
}

static holy_addr_t
holy_find_real_target (void)
{
  holy_uint64_t result = (holy_uint64_t) -1;

  holy_mmap_iterate (target_hook, &result);
  return result;
}

static holy_err_t
holy_cmd_linux (holy_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  holy_file_t file = 0;
  struct linux_kernel_header lh;
  holy_uint8_t setup_sects;
  holy_size_t real_size, kernel_offset = 0;
  holy_ssize_t len;
  int i;
  char *holy_linux_prot_chunk;
  int holy_linux_is_bzimage;
  holy_addr_t holy_linux_prot_target;
  holy_err_t err;
  holy_uint8_t *kernel = NULL;

  holy_dl_ref (my_mod);

  if (argc == 0)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto fail;
    }

  file = holy_file_open (argv[0]);
  if (! file)
    goto fail;

  len = holy_file_size (file);
  kernel = holy_malloc (len);
  if (!kernel)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, N_("cannot allocate kernel buffer"));
      goto fail;
    }

  if (holy_file_read (file, kernel, len) != len)
    {
      if (!holy_errno)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
		    argv[0]);
      goto fail;
    }

  holy_tpm_measure (kernel, len, holy_BINARY_PCR, "holy_linux16", "Kernel");
  holy_print_error();

  holy_memcpy (&lh, kernel, sizeof (lh));
  kernel_offset = sizeof (lh);

  if (lh.boot_flag != holy_cpu_to_le16_compile_time (0xaa55))
    {
      holy_error (holy_ERR_BAD_OS, "invalid magic number");
      goto fail;
    }

  if (lh.setup_sects > holy_LINUX_MAX_SETUP_SECTS)
    {
      holy_error (holy_ERR_BAD_OS, "too many setup sectors");
      goto fail;
    }

  holy_linux_is_bzimage = 0;
  setup_sects = lh.setup_sects;
  linux_mem_size = 0;

  maximal_cmdline_size = 256;

  if (lh.header == holy_cpu_to_le32_compile_time (holy_LINUX_MAGIC_SIGNATURE)
      && holy_le_to_cpu16 (lh.version) >= 0x0200)
    {
      holy_linux_is_bzimage = (lh.loadflags & holy_LINUX_FLAG_BIG_KERNEL);
      lh.type_of_loader = holy_LINUX_BOOT_LOADER_TYPE;

      if (holy_le_to_cpu16 (lh.version) >= 0x0206)
	maximal_cmdline_size = holy_le_to_cpu32 (lh.cmdline_size) + 1;

      holy_linux_real_target = holy_find_real_target ();
      if (holy_linux_real_target == (holy_addr_t)-1)
	{
	  holy_error (holy_ERR_OUT_OF_RANGE,
		      "no appropriate low memory found");
	  goto fail;
	}

      if (holy_le_to_cpu16 (lh.version) >= 0x0201)
	{
	  lh.heap_end_ptr = holy_cpu_to_le16_compile_time (holy_LINUX_HEAP_END_OFFSET);
	  lh.loadflags |= holy_LINUX_FLAG_CAN_USE_HEAP;
	}

      if (holy_le_to_cpu16 (lh.version) >= 0x0202)
	lh.cmd_line_ptr = holy_linux_real_target + holy_LINUX_CL_OFFSET;
      else
	{
	  lh.cl_magic = holy_cpu_to_le16_compile_time (holy_LINUX_CL_MAGIC);
	  lh.cl_offset = holy_cpu_to_le16_compile_time (holy_LINUX_CL_OFFSET);
	  lh.setup_move_size = holy_cpu_to_le16_compile_time (holy_LINUX_CL_OFFSET
						 + maximal_cmdline_size);
	}
    }
  else
    {
      /* Your kernel is quite old...  */
      lh.cl_magic = holy_cpu_to_le16_compile_time (holy_LINUX_CL_MAGIC);
      lh.cl_offset = holy_cpu_to_le16_compile_time (holy_LINUX_CL_OFFSET);

      setup_sects = holy_LINUX_DEFAULT_SETUP_SECTS;

      holy_linux_real_target = holy_LINUX_OLD_REAL_MODE_ADDR;
    }

  /* If SETUP_SECTS is not set, set it to the default (4).  */
  if (! setup_sects)
    setup_sects = holy_LINUX_DEFAULT_SETUP_SECTS;

  real_size = setup_sects << holy_DISK_SECTOR_BITS;
  holy_linux16_prot_size = holy_file_size (file)
    - real_size - holy_DISK_SECTOR_SIZE;

  if (! holy_linux_is_bzimage
      && holy_LINUX_ZIMAGE_ADDR + holy_linux16_prot_size
      > holy_linux_real_target)
    {
      holy_error (holy_ERR_BAD_OS, "too big zImage (0x%x > 0x%x), use bzImage instead",
		  (char *) holy_LINUX_ZIMAGE_ADDR + holy_linux16_prot_size,
		  (holy_size_t) holy_linux_real_target);
      goto fail;
    }

  holy_dprintf ("linux", "[Linux-%s, setup=0x%x, size=0x%x]\n",
		holy_linux_is_bzimage ? "bzImage" : "zImage",
		(unsigned) real_size,
		(unsigned) holy_linux16_prot_size);

  relocator = holy_relocator_new ();
  if (!relocator)
    goto fail;

  for (i = 1; i < argc; i++)
    if (holy_memcmp (argv[i], "vga=", 4) == 0)
      {
	/* Video mode selection support.  */
	holy_uint16_t vid_mode;
	char *val = argv[i] + 4;

	if (holy_strcmp (val, "normal") == 0)
	  vid_mode = holy_LINUX_VID_MODE_NORMAL;
	else if (holy_strcmp (val, "ext") == 0)
	  vid_mode = holy_LINUX_VID_MODE_EXTENDED;
	else if (holy_strcmp (val, "ask") == 0)
	  vid_mode = holy_LINUX_VID_MODE_ASK;
	else
	  vid_mode = (holy_uint16_t) holy_strtoul (val, 0, 0);

	if (holy_errno)
	  goto fail;

	lh.vid_mode = holy_cpu_to_le16 (vid_mode);
      }
    else if (holy_memcmp (argv[i], "mem=", 4) == 0)
      {
	char *val = argv[i] + 4;

	linux_mem_size = holy_strtoul (val, &val, 0);

	if (holy_errno)
	  {
	    holy_errno = holy_ERR_NONE;
	    linux_mem_size = 0;
	  }
	else
	  {
	    int shift = 0;

	    switch (holy_tolower (val[0]))
	      {
	      case 'g':
		shift += 10;
		/* Fallthrough.  */
	      case 'm':
		shift += 10;
		/* Fallthrough.  */
	      case 'k':
		shift += 10;
		/* Fallthrough.  */
	      default:
		break;
	      }

	    /* Check an overflow.  */
	    if (linux_mem_size > (~0UL >> shift))
	      linux_mem_size = 0;
	    else
	      linux_mem_size <<= shift;
	  }
      }

  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (relocator, &ch,
					   holy_linux_real_target,
					   holy_LINUX_CL_OFFSET
					   + maximal_cmdline_size);
    if (err)
      return err;
    holy_linux_real_chunk = get_virtual_current_address (ch);
  }

  /* Put the real mode code at the temporary address.  */
  holy_memmove (holy_linux_real_chunk, &lh, sizeof (lh));

  len = real_size + holy_DISK_SECTOR_SIZE - sizeof (lh);
  holy_memcpy (holy_linux_real_chunk + sizeof (lh), kernel + kernel_offset,
	       len);
  kernel_offset += len;

  if (lh.header != holy_cpu_to_le32_compile_time (holy_LINUX_MAGIC_SIGNATURE)
      || holy_le_to_cpu16 (lh.version) < 0x0200)
    /* Clear the heap space.  */
    holy_memset (holy_linux_real_chunk
		 + ((setup_sects + 1) << holy_DISK_SECTOR_BITS),
		 0,
		 ((holy_LINUX_MAX_SETUP_SECTS - setup_sects - 1)
		  << holy_DISK_SECTOR_BITS));

  /* Create kernel command line.  */
  holy_memcpy ((char *)holy_linux_real_chunk + holy_LINUX_CL_OFFSET,
		LINUX_IMAGE, sizeof (LINUX_IMAGE));
  holy_create_loader_cmdline (argc, argv,
			      (char *)holy_linux_real_chunk
			      + holy_LINUX_CL_OFFSET + sizeof (LINUX_IMAGE) - 1,
			      maximal_cmdline_size
			      - (sizeof (LINUX_IMAGE) - 1));

  if (holy_linux_is_bzimage)
    holy_linux_prot_target = holy_LINUX_BZIMAGE_ADDR;
  else
    holy_linux_prot_target = holy_LINUX_ZIMAGE_ADDR;
  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (relocator, &ch,
					   holy_linux_prot_target,
					   holy_linux16_prot_size);
    if (err)
      return err;
    holy_linux_prot_chunk = get_virtual_current_address (ch);
  }

  len = holy_linux16_prot_size;
  holy_memcpy (holy_linux_prot_chunk, kernel + kernel_offset, len);
  kernel_offset += len;

  if (holy_errno == holy_ERR_NONE)
    {
      holy_loader_set (holy_linux16_boot, holy_linux_unload, 0);
      loaded = 1;
    }

 fail:

  holy_free (kernel);

  if (file)
    holy_file_close (file);

  if (holy_errno != holy_ERR_NONE)
    {
      holy_dl_unref (my_mod);
      loaded = 0;
      holy_relocator_unload (relocator);
    }

  return holy_errno;
}

static holy_err_t
holy_cmd_initrd (holy_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  holy_size_t size = 0;
  holy_addr_t addr_max, addr_min;
  struct linux_kernel_header *lh;
  holy_uint8_t *initrd_chunk;
  holy_addr_t initrd_addr;
  holy_err_t err;
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

  lh = (struct linux_kernel_header *) holy_linux_real_chunk;

  if (!(lh->header == holy_cpu_to_le32_compile_time (holy_LINUX_MAGIC_SIGNATURE)
	&& holy_le_to_cpu16 (lh->version) >= 0x0200))
    {
      holy_error (holy_ERR_BAD_OS, "the kernel is too old for initrd");
      goto fail;
    }

  /* Get the highest address available for the initrd.  */
  if (holy_le_to_cpu16 (lh->version) >= 0x0203)
    {
      addr_max = holy_cpu_to_le32 (lh->initrd_addr_max);

      /* XXX in reality, Linux specifies a bogus value, so
	 it is necessary to make sure that ADDR_MAX does not exceed
	 0x3fffffff.  */
      if (addr_max > holy_LINUX_INITRD_MAX_ADDRESS)
	addr_max = holy_LINUX_INITRD_MAX_ADDRESS;
    }
  else
    addr_max = holy_LINUX_INITRD_MAX_ADDRESS;

  if (linux_mem_size != 0 && linux_mem_size < addr_max)
    addr_max = linux_mem_size;

  /* Linux 2.3.xx has a bug in the memory range check, so avoid
     the last page.
     Linux 2.2.xx has a bug in the memory range check, which is
     worse than that of Linux 2.3.xx, so avoid the last 64kb.  */
  addr_max -= 0x10000;

  addr_min = holy_LINUX_BZIMAGE_ADDR + holy_linux16_prot_size;

  if (holy_initrd_init (argc, argv, &initrd_ctx))
    goto fail;

  size = holy_get_initrd_size (&initrd_ctx);

  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_align (relocator, &ch,
					    addr_min, addr_max - size,
					    size, 0x1000,
					    holy_RELOCATOR_PREFERENCE_HIGH, 0);
    if (err)
      return err;
    initrd_chunk = get_virtual_current_address (ch);
    initrd_addr = get_physical_target_address (ch);
  }

  if (holy_initrd_load (&initrd_ctx, argv, initrd_chunk))
    goto fail;

  lh->ramdisk_image = initrd_addr;
  lh->ramdisk_size = size;

 fail:
  holy_initrd_close (&initrd_ctx);

  return holy_errno;
}

static holy_command_t cmd_linux, cmd_initrd;

holy_MOD_INIT(linux16)
{
  cmd_linux =
    holy_register_command ("linux16", holy_cmd_linux,
			   0, N_("Load Linux."));
  cmd_initrd =
    holy_register_command ("initrd16", holy_cmd_initrd,
			   0, N_("Load initrd."));
  my_mod = mod;
}

holy_MOD_FINI(linux16)
{
  holy_unregister_command (cmd_linux);
  holy_unregister_command (cmd_initrd);
}
