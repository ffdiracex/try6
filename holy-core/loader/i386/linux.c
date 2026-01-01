/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/loader.h>
#include <holy/memory.h>
#include <holy/normal.h>
#include <holy/file.h>
#include <holy/disk.h>
#include <holy/err.h>
#include <holy/misc.h>
#include <holy/types.h>
#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/term.h>
#include <holy/cpu/linux.h>
#include <holy/video.h>
#include <holy/video_fb.h>
#include <holy/command.h>
#include <holy/i386/relocator.h>
#include <holy/i18n.h>
#include <holy/lib/cmdline.h>
#include <holy/linux.h>
#include <holy/tpm.h>

#include <holy/verity-hash.h>

holy_MOD_LICENSE ("GPLv2+");

#ifdef holy_MACHINE_PCBIOS
#include <holy/i386/pc/vesa_modes_table.h>
#endif

#ifdef holy_MACHINE_EFI
#include <holy/efi/efi.h>
#define HAS_VGA_TEXT 0
#define DEFAULT_VIDEO_MODE "auto"
#define ACCEPTS_PURE_TEXT 0
#elif defined (holy_MACHINE_IEEE1275)
#include <holy/ieee1275/ieee1275.h>
#define HAS_VGA_TEXT 0
#define DEFAULT_VIDEO_MODE "text"
#define ACCEPTS_PURE_TEXT 1
#else
#include <holy/i386/pc/vbe.h>
#include <holy/i386/pc/console.h>
#define HAS_VGA_TEXT 1
#define DEFAULT_VIDEO_MODE "text"
#define ACCEPTS_PURE_TEXT 1
#endif

static holy_dl_t my_mod;

static holy_size_t linux_mem_size;
static int loaded;
static void *prot_mode_mem;
static holy_addr_t prot_mode_target;
static void *initrd_mem;
static holy_addr_t initrd_mem_target;
static holy_size_t prot_init_space;
static struct holy_relocator *relocator = NULL;
static void *efi_mmap_buf;
static holy_size_t maximal_cmdline_size;
static struct linux_kernel_params linux_params;
static char *linux_cmdline;
#ifdef holy_MACHINE_EFI
static holy_efi_uintn_t efi_mmap_size;
#else
static const holy_size_t efi_mmap_size = 0;
#endif

/* FIXME */
#if 0
struct idt_descriptor
{
  holy_uint16_t limit;
  void *base;
} holy_PACKED;

static struct idt_descriptor idt_desc =
  {
    0,
    0
  };
#endif

static inline holy_size_t
page_align (holy_size_t size)
{
  return (size + (1 << 12) - 1) & (~((1 << 12) - 1));
}

#ifdef holy_MACHINE_EFI
/* Find the optimal number of pages for the memory map. Is it better to
   move this code to efi/mm.c?  */
static holy_efi_uintn_t
find_efi_mmap_size (void)
{
  static holy_efi_uintn_t mmap_size = 0;

  if (mmap_size != 0)
    return mmap_size;

  mmap_size = (1 << 12);
  while (1)
    {
      int ret;
      holy_efi_memory_descriptor_t *mmap;
      holy_efi_uintn_t desc_size;
      holy_efi_uintn_t cur_mmap_size = mmap_size;

      mmap = holy_malloc (cur_mmap_size);
      if (! mmap)
	return 0;

      ret = holy_efi_get_memory_map (&cur_mmap_size, mmap, 0, &desc_size, 0);
      holy_free (mmap);

      if (ret < 0)
	{
	  holy_error (holy_ERR_IO, "cannot get memory map");
	  return 0;
	}
      else if (ret > 0)
	break;

      if (mmap_size < cur_mmap_size)
	mmap_size = cur_mmap_size;
      mmap_size += (1 << 12);
    }

  /* Increase the size a bit for safety, because holy allocates more on
     later, and EFI itself may allocate more.  */
  mmap_size += (3 << 12);

  mmap_size = page_align (mmap_size);
  return mmap_size;
}

#endif

/* Helper for find_mmap_size.  */
static int
count_hook (holy_uint64_t addr __attribute__ ((unused)),
	    holy_uint64_t size __attribute__ ((unused)),
	    holy_memory_type_t type __attribute__ ((unused)), void *data)
{
  holy_size_t *count = data;

  (*count)++;
  return 0;
}

/* Find the optimal number of pages for the memory map. */
static holy_size_t
find_mmap_size (void)
{
  holy_size_t count = 0, mmap_size;

  holy_mmap_iterate (count_hook, &count);

  mmap_size = count * sizeof (struct holy_e820_mmap);

  /* Increase the size a bit for safety, because holy allocates more on
     later.  */
  mmap_size += (1 << 12);

  return page_align (mmap_size);
}

static void
free_pages (void)
{
  holy_relocator_unload (relocator);
  relocator = NULL;
  prot_mode_mem = initrd_mem = 0;
  prot_mode_target = initrd_mem_target = 0;
}

/* Allocate pages for the real mode code and the protected mode code
   for linux as well as a memory map buffer.  */
static holy_err_t
allocate_pages (holy_size_t prot_size, holy_size_t *align,
		holy_size_t min_align, int relocatable,
		holy_uint64_t preferred_address)
{
  holy_err_t err;

  if (prot_size == 0)
    prot_size = 1;

  prot_size = page_align (prot_size);

  /* Initialize the memory pointers with NULL for convenience.  */
  free_pages ();

  relocator = holy_relocator_new ();
  if (!relocator)
    {
      err = holy_errno;
      goto fail;
    }

  /* FIXME: Should request low memory from the heap when this feature is
     implemented.  */

  {
    holy_relocator_chunk_t ch;
    if (relocatable)
      {
	err = holy_relocator_alloc_chunk_align (relocator, &ch,
						preferred_address,
						preferred_address,
						prot_size, 1,
						holy_RELOCATOR_PREFERENCE_LOW,
						1);
	for (; err && *align + 1 > min_align; (*align)--)
	  {
	    holy_errno = holy_ERR_NONE;
	    err = holy_relocator_alloc_chunk_align (relocator, &ch,
						    0x1000000,
						    0xffffffff & ~prot_size,
						    prot_size, 1 << *align,
						    holy_RELOCATOR_PREFERENCE_LOW,
						    1);
	  }
	if (err)
	  goto fail;
      }
    else
      err = holy_relocator_alloc_chunk_addr (relocator, &ch,
					     preferred_address,
					     prot_size);
    if (err)
      goto fail;
    prot_mode_mem = get_virtual_current_address (ch);
    prot_mode_target = get_physical_target_address (ch);
  }

  holy_dprintf ("linux", "prot_mode_mem = %p, prot_mode_target = %lx, prot_size = %x\n",
                prot_mode_mem, (unsigned long) prot_mode_target,
		(unsigned) prot_size);
  return holy_ERR_NONE;

 fail:
  free_pages ();
  return err;
}

static holy_err_t
holy_e820_add_region (struct holy_e820_mmap *e820_map, int *e820_num,
                      holy_uint64_t start, holy_uint64_t size,
                      holy_uint32_t type)
{
  int n = *e820_num;

  if ((n > 0) && (e820_map[n - 1].addr + e820_map[n - 1].size == start) &&
      (e820_map[n - 1].type == type))
      e820_map[n - 1].size += size;
  else
    {
      e820_map[n].addr = start;
      e820_map[n].size = size;
      e820_map[n].type = type;
      (*e820_num)++;
    }
  return holy_ERR_NONE;
}

static holy_err_t
holy_linux_setup_video (struct linux_kernel_params *params)
{
  struct holy_video_mode_info mode_info;
  void *framebuffer;
  holy_err_t err;
  holy_video_driver_id_t driver_id;
  const char *gfxlfbvar = holy_env_get ("gfxpayloadforcelfb");

  driver_id = holy_video_get_driver_id ();

  if (driver_id == holy_VIDEO_DRIVER_NONE)
    return 1;

  err = holy_video_get_info_and_fini (&mode_info, &framebuffer);

  if (err)
    {
      holy_errno = holy_ERR_NONE;
      return 1;
    }

  params->lfb_width = mode_info.width;
  params->lfb_height = mode_info.height;
  params->lfb_depth = mode_info.bpp;
  params->lfb_line_len = mode_info.pitch;

  params->lfb_base = (holy_size_t) framebuffer;
  params->lfb_size = ALIGN_UP (params->lfb_line_len * params->lfb_height, 65536);

  params->red_mask_size = mode_info.red_mask_size;
  params->red_field_pos = mode_info.red_field_pos;
  params->green_mask_size = mode_info.green_mask_size;
  params->green_field_pos = mode_info.green_field_pos;
  params->blue_mask_size = mode_info.blue_mask_size;
  params->blue_field_pos = mode_info.blue_field_pos;
  params->reserved_mask_size = mode_info.reserved_mask_size;
  params->reserved_field_pos = mode_info.reserved_field_pos;

  if (gfxlfbvar && (gfxlfbvar[0] == '1' || gfxlfbvar[0] == 'y'))
    params->have_vga = holy_VIDEO_LINUX_TYPE_SIMPLE;
  else
    {
      switch (driver_id)
	{
	case holy_VIDEO_DRIVER_VBE:
	  params->lfb_size >>= 16;
	  params->have_vga = holy_VIDEO_LINUX_TYPE_VESA;
	  break;
	
	case holy_VIDEO_DRIVER_EFI_UGA:
	case holy_VIDEO_DRIVER_EFI_GOP:
	  params->have_vga = holy_VIDEO_LINUX_TYPE_EFIFB;
	  break;

	  /* FIXME: check if better id is available.  */
	case holy_VIDEO_DRIVER_SM712:
	case holy_VIDEO_DRIVER_SIS315PRO:
	case holy_VIDEO_DRIVER_VGA:
	case holy_VIDEO_DRIVER_CIRRUS:
	case holy_VIDEO_DRIVER_BOCHS:
	case holy_VIDEO_DRIVER_RADEON_FULOONG2E:
	case holy_VIDEO_DRIVER_RADEON_YEELOONG3A:
	case holy_VIDEO_DRIVER_IEEE1275:
	case holy_VIDEO_DRIVER_COREBOOT:
	  /* Make gcc happy. */
	case holy_VIDEO_DRIVER_XEN:
	case holy_VIDEO_DRIVER_SDL:
	case holy_VIDEO_DRIVER_NONE:
	case holy_VIDEO_ADAPTER_CAPTURE:
	  params->have_vga = holy_VIDEO_LINUX_TYPE_SIMPLE;
	  break;
	}
    }

#ifdef holy_MACHINE_PCBIOS
  /* VESA packed modes may come with zeroed mask sizes, which need
     to be set here according to DAC Palette width.  If we don't,
     this results in Linux displaying a black screen.  */
  if (driver_id == holy_VIDEO_DRIVER_VBE && mode_info.bpp <= 8)
    {
      struct holy_vbe_info_block controller_info;
      int status;
      int width = 8;

      status = holy_vbe_bios_get_controller_info (&controller_info);

      if (status == holy_VBE_STATUS_OK &&
	  (controller_info.capabilities & holy_VBE_CAPABILITY_DACWIDTH))
	status = holy_vbe_bios_set_dac_palette_width (&width);

      if (status != holy_VBE_STATUS_OK)
	/* 6 is default after mode reset.  */
	width = 6;

      params->red_mask_size = params->green_mask_size
	= params->blue_mask_size = width;
      params->reserved_mask_size = 0;
    }
#endif

  return holy_ERR_NONE;
}

/* Context for holy_linux_boot.  */
struct holy_linux_boot_ctx
{
  holy_addr_t real_mode_target;
  holy_size_t real_size;
  struct linux_kernel_params *params;
  int e820_num;
};

/* Helper for holy_linux_boot.  */
static int
holy_linux_boot_mmap_find (holy_uint64_t addr, holy_uint64_t size,
			   holy_memory_type_t type, void *data)
{
  struct holy_linux_boot_ctx *ctx = data;

  /* We must put real mode code in the traditional space.  */
  if (type != holy_MEMORY_AVAILABLE || addr > 0x90000)
    return 0;

  if (addr + size < 0x10000)
    return 0;

  if (addr < 0x10000)
    {
      size += addr - 0x10000;
      addr = 0x10000;
    }

  if (addr + size > 0x90000)
    size = 0x90000 - addr;

  if (ctx->real_size + efi_mmap_size > size)
    return 0;

  holy_dprintf ("linux", "addr = %lx, size = %x, need_size = %x\n",
		(unsigned long) addr,
		(unsigned) size,
		(unsigned) (ctx->real_size + efi_mmap_size));
  ctx->real_mode_target = ((addr + size) - (ctx->real_size + efi_mmap_size));
  return 1;
}

/* holy types conveniently match E820 types.  */
static int
holy_linux_boot_mmap_fill (holy_uint64_t addr, holy_uint64_t size,
			   holy_memory_type_t type, void *data)
{
  struct holy_linux_boot_ctx *ctx = data;

  if (holy_e820_add_region (ctx->params->e820_map, &ctx->e820_num,
			    addr, size, type))
    return 1;

  return 0;
}

static holy_err_t
holy_linux_boot (void)
{
  holy_err_t err = 0;
  const char *modevar;
  char *tmp;
  struct holy_relocator32_state state;
  void *real_mode_mem;
  struct holy_linux_boot_ctx ctx = {
    .real_mode_target = 0
  };
  holy_size_t mmap_size;
  holy_size_t cl_offset;

#ifdef holy_MACHINE_IEEE1275
  {
    const char *bootpath;
    holy_ssize_t len;

    bootpath = holy_env_get ("root");
    if (bootpath)
      holy_ieee1275_set_property (holy_ieee1275_chosen,
				  "bootpath", bootpath,
				  holy_strlen (bootpath) + 1,
				  &len);
    linux_params.ofw_signature = holy_LINUX_OFW_SIGNATURE;
    linux_params.ofw_num_items = 1;
    linux_params.ofw_cif_handler = (holy_uint32_t) holy_ieee1275_entry_fn;
    linux_params.ofw_idt = 0;
  }
#endif

  modevar = holy_env_get ("gfxpayload");

  /* Now all graphical modes are acceptable.
     May change in future if we have modes without framebuffer.  */
  if (modevar && *modevar != 0)
    {
      tmp = holy_xasprintf ("%s;" DEFAULT_VIDEO_MODE, modevar);
      if (! tmp)
	return holy_errno;
#if ACCEPTS_PURE_TEXT
      err = holy_video_set_mode (tmp, 0, 0);
#else
      err = holy_video_set_mode (tmp, holy_VIDEO_MODE_TYPE_PURE_TEXT, 0);
#endif
      holy_free (tmp);
    }
  else       /* We can't go back to text mode from coreboot fb.  */
#ifdef holy_MACHINE_COREBOOT
    if (holy_video_get_driver_id () == holy_VIDEO_DRIVER_COREBOOT)
      err = holy_ERR_NONE;
    else
#endif
      {
#if ACCEPTS_PURE_TEXT
	err = holy_video_set_mode (DEFAULT_VIDEO_MODE, 0, 0);
#else
	err = holy_video_set_mode (DEFAULT_VIDEO_MODE,
				 holy_VIDEO_MODE_TYPE_PURE_TEXT, 0);
#endif
      }

  if (err)
    {
      holy_print_error ();
      holy_puts_ (N_("Booting in blind mode"));
      holy_errno = holy_ERR_NONE;
    }

  if (holy_linux_setup_video (&linux_params))
    {
#if defined (holy_MACHINE_PCBIOS) || defined (holy_MACHINE_COREBOOT) || defined (holy_MACHINE_QEMU)
      linux_params.have_vga = holy_VIDEO_LINUX_TYPE_TEXT;
      linux_params.video_mode = 0x3;
#else
      linux_params.have_vga = 0;
      linux_params.video_mode = 0;
      linux_params.video_width = 0;
      linux_params.video_height = 0;
#endif
    }


#ifndef holy_MACHINE_IEEE1275
  if (linux_params.have_vga == holy_VIDEO_LINUX_TYPE_TEXT)
#endif
    {
      holy_term_output_t term;
      int found = 0;
      FOR_ACTIVE_TERM_OUTPUTS(term)
	if (holy_strcmp (term->name, "vga_text") == 0
	    || holy_strcmp (term->name, "console") == 0
	    || holy_strcmp (term->name, "ofconsole") == 0)
	  {
	    struct holy_term_coordinate pos = holy_term_getxy (term);
	    linux_params.video_cursor_x = pos.x;
	    linux_params.video_cursor_y = pos.y;
	    linux_params.video_width = holy_term_width (term);
	    linux_params.video_height = holy_term_height (term);
	    found = 1;
	    break;
	  }
      if (!found)
	{
	  linux_params.video_cursor_x = 0;
	  linux_params.video_cursor_y = 0;
	  linux_params.video_width = 80;
	  linux_params.video_height = 25;
	}
    }

  mmap_size = find_mmap_size ();
  /* Make sure that each size is aligned to a page boundary.  */
  cl_offset = ALIGN_UP (mmap_size + sizeof (linux_params), 4096);
  if (cl_offset < ((holy_size_t) linux_params.setup_sects << holy_DISK_SECTOR_BITS))
    cl_offset = ALIGN_UP ((holy_size_t) (linux_params.setup_sects
					 << holy_DISK_SECTOR_BITS), 4096);
  ctx.real_size = ALIGN_UP (cl_offset + maximal_cmdline_size, 4096);

#ifdef holy_MACHINE_EFI
  efi_mmap_size = find_efi_mmap_size ();
  if (efi_mmap_size == 0)
    return holy_errno;
#endif

  holy_dprintf ("linux", "real_size = %x, mmap_size = %x\n",
		(unsigned) ctx.real_size, (unsigned) mmap_size);

#ifdef holy_MACHINE_EFI
  holy_efi_mmap_iterate (holy_linux_boot_mmap_find, &ctx, 1);
  if (! ctx.real_mode_target)
    holy_efi_mmap_iterate (holy_linux_boot_mmap_find, &ctx, 0);
#else
  holy_mmap_iterate (holy_linux_boot_mmap_find, &ctx);
#endif
  holy_dprintf ("linux", "real_mode_target = %lx, real_size = %x, efi_mmap_size = %x\n",
                (unsigned long) ctx.real_mode_target,
		(unsigned) ctx.real_size,
		(unsigned) efi_mmap_size);

  if (! ctx.real_mode_target)
    return holy_error (holy_ERR_OUT_OF_MEMORY, "cannot allocate real mode pages");

  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (relocator, &ch,
					   ctx.real_mode_target,
					   (ctx.real_size + efi_mmap_size));
    if (err)
     return err;
    real_mode_mem = get_virtual_current_address (ch);
  }
  efi_mmap_buf = (holy_uint8_t *) real_mode_mem + ctx.real_size;

  holy_dprintf ("linux", "real_mode_mem = %p\n",
                real_mode_mem);

  ctx.params = real_mode_mem;

  *ctx.params = linux_params;
  ctx.params->cmd_line_ptr = ctx.real_mode_target + cl_offset;
  holy_memcpy ((char *) ctx.params + cl_offset, linux_cmdline,
	       maximal_cmdline_size);

  holy_dprintf ("linux", "code32_start = %x\n",
		(unsigned) ctx.params->code32_start);

  ctx.e820_num = 0;
  if (holy_mmap_iterate (holy_linux_boot_mmap_fill, &ctx))
    return holy_errno;
  ctx.params->mmap_size = ctx.e820_num;

#ifdef holy_MACHINE_EFI
  {
    holy_efi_uintn_t efi_desc_size;
    holy_size_t efi_mmap_target;
    holy_efi_uint32_t efi_desc_version;
    err = holy_efi_finish_boot_services (&efi_mmap_size, efi_mmap_buf, NULL,
					 &efi_desc_size, &efi_desc_version);
    if (err)
      return err;
    
    /* Note that no boot services are available from here.  */
    efi_mmap_target = ctx.real_mode_target 
      + ((holy_uint8_t *) efi_mmap_buf - (holy_uint8_t *) real_mode_mem);
    /* Pass EFI parameters.  */
    if (holy_le_to_cpu16 (ctx.params->version) >= 0x0208)
      {
	ctx.params->v0208.efi_mem_desc_size = efi_desc_size;
	ctx.params->v0208.efi_mem_desc_version = efi_desc_version;
	ctx.params->v0208.efi_mmap = efi_mmap_target;
	ctx.params->v0208.efi_mmap_size = efi_mmap_size;

#ifdef __x86_64__
	ctx.params->v0208.efi_mmap_hi = (efi_mmap_target >> 32);
#endif
      }
    else if (holy_le_to_cpu16 (ctx.params->version) >= 0x0206)
      {
	ctx.params->v0206.efi_mem_desc_size = efi_desc_size;
	ctx.params->v0206.efi_mem_desc_version = efi_desc_version;
	ctx.params->v0206.efi_mmap = efi_mmap_target;
	ctx.params->v0206.efi_mmap_size = efi_mmap_size;
      }
    else if (holy_le_to_cpu16 (ctx.params->version) >= 0x0204)
      {
	ctx.params->v0204.efi_mem_desc_size = efi_desc_size;
	ctx.params->v0204.efi_mem_desc_version = efi_desc_version;
	ctx.params->v0204.efi_mmap = efi_mmap_target;
	ctx.params->v0204.efi_mmap_size = efi_mmap_size;
      }
  }
#endif

  /* FIXME.  */
  /*  asm volatile ("lidt %0" : : "m" (idt_desc)); */
  state.ebp = state.edi = state.ebx = 0;
  state.esi = ctx.real_mode_target;
  state.esp = ctx.real_mode_target;
  state.eip = ctx.params->code32_start;
  return holy_relocator32_boot (relocator, state, 0);
}

static holy_err_t
holy_linux_unload (void)
{
  holy_dl_unref (my_mod);
  loaded = 0;
  holy_free (linux_cmdline);
  linux_cmdline = 0;
  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_linux (holy_command_t cmd __attribute__ ((unused)),
		int argc, char *argv[])
{
  holy_file_t file = 0;
  struct linux_kernel_header lh;
  holy_uint8_t setup_sects;
  holy_size_t real_size, prot_size, prot_file_size, kernel_offset;
  holy_ssize_t len;
  int i;
  holy_size_t align, min_align;
  int relocatable;
  holy_uint64_t preferred_address = holy_LINUX_BZIMAGE_ADDR;
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

  holy_tpm_measure (kernel, len, holy_BINARY_PCR, "holy_linux", "Kernel");
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

  /* FIXME: 2.03 is not always good enough (Linux 2.4 can be 2.03 and
     still not support 32-bit boot.  */
  if (lh.header != holy_cpu_to_le32_compile_time (holy_LINUX_MAGIC_SIGNATURE)
      || holy_le_to_cpu16 (lh.version) < 0x0203)
    {
      holy_error (holy_ERR_BAD_OS, "version too old for 32-bit boot"
#ifdef holy_MACHINE_PCBIOS
		  " (try with `linux16')"
#endif
		  );
      goto fail;
    }

  if (! (lh.loadflags & holy_LINUX_FLAG_BIG_KERNEL))
    {
      holy_error (holy_ERR_BAD_OS, "zImage doesn't support 32-bit boot"
#ifdef holy_MACHINE_PCBIOS
		  " (try with `linux16')"
#endif
		  );
      goto fail;
    }

  if (holy_le_to_cpu16 (lh.version) >= 0x0206)
    maximal_cmdline_size = holy_le_to_cpu32 (lh.cmdline_size) + 1;
  else
    maximal_cmdline_size = 256;

  if (maximal_cmdline_size < 128)
    maximal_cmdline_size = 128;

  setup_sects = lh.setup_sects;

  /* If SETUP_SECTS is not set, set it to the default (4).  */
  if (! setup_sects)
    setup_sects = holy_LINUX_DEFAULT_SETUP_SECTS;

  real_size = setup_sects << holy_DISK_SECTOR_BITS;
  prot_file_size = holy_file_size (file) - real_size - holy_DISK_SECTOR_SIZE;

  if (holy_le_to_cpu16 (lh.version) >= 0x205
      && lh.kernel_alignment != 0
      && ((lh.kernel_alignment - 1) & lh.kernel_alignment) == 0)
    {
      for (align = 0; align < 32; align++)
	if (holy_le_to_cpu32 (lh.kernel_alignment) & (1 << align))
	  break;
      relocatable = holy_le_to_cpu32 (lh.relocatable);
    }
  else
    {
      align = 0;
      relocatable = 0;
    }
    
  if (holy_le_to_cpu16 (lh.version) >= 0x020a)
    {
      min_align = lh.min_alignment;
      prot_size = holy_le_to_cpu32 (lh.init_size);
      prot_init_space = page_align (prot_size);
      if (relocatable)
	preferred_address = holy_le_to_cpu64 (lh.pref_address);
      else
	preferred_address = holy_LINUX_BZIMAGE_ADDR;
    }
  else
    {
      min_align = align;
      prot_size = prot_file_size;
      preferred_address = holy_LINUX_BZIMAGE_ADDR;
      /* Usually, the compression ratio is about 50%.  */
      prot_init_space = page_align (prot_size) * 3;
    }

  if (allocate_pages (prot_size, &align,
		      min_align, relocatable,
		      preferred_address))
    goto fail;

  holy_memset (&linux_params, 0, sizeof (linux_params));
  holy_memcpy (&linux_params.setup_sects, &lh.setup_sects, sizeof (lh) - 0x1F1);

  linux_params.code32_start = prot_mode_target + lh.code32_start - holy_LINUX_BZIMAGE_ADDR;
  linux_params.kernel_alignment = (1 << align);
  linux_params.ps_mouse = linux_params.padding10 =  0;

  len = sizeof (linux_params) - sizeof (lh);

  holy_memcpy ((char *) &linux_params + sizeof (lh), kernel + kernel_offset, len);
  kernel_offset += len;

  linux_params.type_of_loader = holy_LINUX_BOOT_LOADER_TYPE;

  /* These two are used (instead of cmd_line_ptr) by older versions of Linux,
     and otherwise ignored.  */
  linux_params.cl_magic = holy_LINUX_CL_MAGIC;
  linux_params.cl_offset = 0x1000;

  linux_params.ramdisk_image = 0;
  linux_params.ramdisk_size = 0;

  linux_params.heap_end_ptr = holy_LINUX_HEAP_END_OFFSET;
  linux_params.loadflags |= holy_LINUX_FLAG_CAN_USE_HEAP;

  /* These are not needed to be precise, because Linux uses these values
     only to raise an error when the decompression code cannot find good
     space.  */
  linux_params.ext_mem = ((32 * 0x100000) >> 10);
  linux_params.alt_mem = ((32 * 0x100000) >> 10);

  /* Ignored by Linux.  */
  linux_params.video_page = 0;

  /* Only used when `video_mode == 0x7', otherwise ignored.  */
  linux_params.video_ega_bx = 0;

  linux_params.font_size = 16; /* XXX */

#ifdef holy_MACHINE_EFI
#ifdef __x86_64__
  if (holy_le_to_cpu16 (linux_params.version) < 0x0208 &&
      ((holy_addr_t) holy_efi_system_table >> 32) != 0)
    return holy_error(holy_ERR_BAD_OS,
		      "kernel does not support 64-bit addressing");
#endif

  if (holy_le_to_cpu16 (linux_params.version) >= 0x0208)
    {
      linux_params.v0208.efi_signature = holy_LINUX_EFI_SIGNATURE;
      linux_params.v0208.efi_system_table = (holy_uint32_t) (holy_addr_t) holy_efi_system_table;
#ifdef __x86_64__
      linux_params.v0208.efi_system_table_hi = (holy_uint32_t) ((holy_uint64_t) holy_efi_system_table >> 32);
#endif
    }
  else if (holy_le_to_cpu16 (linux_params.version) >= 0x0206)
    {
      linux_params.v0206.efi_signature = holy_LINUX_EFI_SIGNATURE;
      linux_params.v0206.efi_system_table = (holy_uint32_t) (holy_addr_t) holy_efi_system_table;
    }
  else if (holy_le_to_cpu16 (linux_params.version) >= 0x0204)
    {
      linux_params.v0204.efi_signature = holy_LINUX_EFI_SIGNATURE_0204;
      linux_params.v0204.efi_system_table = (holy_uint32_t) (holy_addr_t) holy_efi_system_table;
    }
#endif

  /* The other parameters are filled when booting.  */

  kernel_offset = real_size + holy_DISK_SECTOR_SIZE;

  holy_dprintf ("linux", "bzImage, setup=0x%x, size=0x%x\n",
		(unsigned) real_size, (unsigned) prot_size);

  /* Look for memory size and video mode specified on the command line.  */
  linux_mem_size = 0;
  for (i = 1; i < argc; i++)
#ifdef holy_MACHINE_PCBIOS
    if (holy_memcmp (argv[i], "vga=", 4) == 0)
      {
	/* Video mode selection support.  */
	char *val = argv[i] + 4;
	unsigned vid_mode = holy_LINUX_VID_MODE_NORMAL;
	struct holy_vesa_mode_table_entry *linux_mode;
	holy_err_t err;
	char *buf;

	holy_dl_load ("vbe");

	if (holy_strcmp (val, "normal") == 0)
	  vid_mode = holy_LINUX_VID_MODE_NORMAL;
	else if (holy_strcmp (val, "ext") == 0)
	  vid_mode = holy_LINUX_VID_MODE_EXTENDED;
	else if (holy_strcmp (val, "ask") == 0)
	  {
	    holy_puts_ (N_("Legacy `ask' parameter no longer supported."));

	    /* We usually would never do this in a loader, but "vga=ask" means user
	       requested interaction, so it can't hurt to request keyboard input.  */
	    holy_wait_after_message ();

	    goto fail;
	  }
	else
	  vid_mode = (holy_uint16_t) holy_strtoul (val, 0, 0);

	switch (vid_mode)
	  {
	  case 0:
	  case holy_LINUX_VID_MODE_NORMAL:
	    holy_env_set ("gfxpayload", "text");
	    holy_printf_ (N_("%s is deprecated. "
			     "Use set gfxpayload=%s before "
			     "linux command instead.\n"),
			  argv[i], "text");
	    break;

	  case 1:
	  case holy_LINUX_VID_MODE_EXTENDED:
	    /* FIXME: support 80x50 text. */
	    holy_env_set ("gfxpayload", "text");
	    holy_printf_ (N_("%s is deprecated. "
			     "Use set gfxpayload=%s before "
			     "linux command instead.\n"),
			  argv[i], "text");
	    break;
	  default:
	    /* Ignore invalid values.  */
	    if (vid_mode < holy_VESA_MODE_TABLE_START ||
		vid_mode > holy_VESA_MODE_TABLE_END)
	      {
		holy_env_set ("gfxpayload", "text");
		/* TRANSLATORS: "x" has to be entered in, like an identifier,
		   so please don't use better Unicode codepoints.  */
		holy_printf_ (N_("%s is deprecated. VGA mode %d isn't recognized. "
				 "Use set gfxpayload=WIDTHxHEIGHT[xDEPTH] "
				 "before linux command instead.\n"),
			     argv[i], vid_mode);
		break;
	      }

	    linux_mode = &holy_vesa_mode_table[vid_mode
					       - holy_VESA_MODE_TABLE_START];

	    buf = holy_xasprintf ("%ux%ux%u,%ux%u",
				 linux_mode->width, linux_mode->height,
				 linux_mode->depth,
				 linux_mode->width, linux_mode->height);
	    if (! buf)
	      goto fail;

	    holy_printf_ (N_("%s is deprecated. "
			     "Use set gfxpayload=%s before "
			     "linux command instead.\n"),
			 argv[i], buf);
	    err = holy_env_set ("gfxpayload", buf);
	    holy_free (buf);
	    if (err)
	      goto fail;
	  }
      }
    else
#endif /* holy_MACHINE_PCBIOS */
    if (holy_memcmp (argv[i], "mem=", 4) == 0)
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
		/* FALLTHROUGH */
	      case 'm':
		shift += 10;
		/* FALLTHROUGH */
	      case 'k':
		shift += 10;
		/* FALLTHROUGH */
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
    else if (holy_memcmp (argv[i], "quiet", sizeof ("quiet") - 1) == 0)
      {
	linux_params.loadflags |= holy_LINUX_FLAG_QUIET;
      }

  /* Create kernel command line.  */
  linux_cmdline = holy_zalloc (maximal_cmdline_size + 1);
  if (!linux_cmdline)
    goto fail;
  holy_memcpy (linux_cmdline, LINUX_IMAGE, sizeof (LINUX_IMAGE));
  holy_create_loader_cmdline (argc, argv,
			      linux_cmdline
			      + sizeof (LINUX_IMAGE) - 1,
			      maximal_cmdline_size
			      - (sizeof (LINUX_IMAGE) - 1));

  holy_pass_verity_hash(&lh, linux_cmdline, maximal_cmdline_size);
  len = prot_file_size;
  holy_memcpy (prot_mode_mem, kernel + kernel_offset, len);
  kernel_offset += len;

  if (holy_errno == holy_ERR_NONE)
    {
      holy_loader_set (holy_linux_boot, holy_linux_unload,
		       0 /* set noreturn=0 in order to avoid holy_console_fini() */);
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
    }

  return holy_errno;
}

static holy_err_t
holy_cmd_initrd (holy_command_t cmd __attribute__ ((unused)),
		 int argc, char *argv[])
{
  holy_size_t size = 0, aligned_size = 0;
  holy_addr_t addr_min, addr_max;
  holy_addr_t addr;
  holy_err_t err;
  struct holy_linux_initrd_context initrd_ctx = { 0, 0, 0 };

  if (argc == 0)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));
      goto fail;
    }

  if (! loaded)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, N_("you need to load the kernel first"));
      goto fail;
    }

  if (holy_initrd_init (argc, argv, &initrd_ctx))
    goto fail;

  size = holy_get_initrd_size (&initrd_ctx);
  aligned_size = ALIGN_UP (size, 4096);

  /* Get the highest address available for the initrd.  */
  if (holy_le_to_cpu16 (linux_params.version) >= 0x0203)
    {
      addr_max = holy_cpu_to_le32 (linux_params.initrd_addr_max);

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

  addr_min = (holy_addr_t) prot_mode_target + prot_init_space;

  /* Put the initrd as high as possible, 4KiB aligned.  */
  addr = (addr_max - aligned_size) & ~0xFFF;

  if (addr < addr_min)
    {
      holy_error (holy_ERR_OUT_OF_RANGE, "the initrd is too big");
      goto fail;
    }

  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_align (relocator, &ch,
					    addr_min, addr, aligned_size,
					    0x1000,
					    holy_RELOCATOR_PREFERENCE_HIGH,
					    1);
    if (err)
      return err;
    initrd_mem = get_virtual_current_address (ch);
    initrd_mem_target = get_physical_target_address (ch);
  }

  if (holy_initrd_load (&initrd_ctx, argv, initrd_mem))
    goto fail;

  holy_dprintf ("linux", "Initrd, addr=0x%x, size=0x%x\n",
		(unsigned) addr, (unsigned) size);

  linux_params.ramdisk_image = initrd_mem_target;
  linux_params.ramdisk_size = size;
  linux_params.root_dev = 0x0100; /* XXX */

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
