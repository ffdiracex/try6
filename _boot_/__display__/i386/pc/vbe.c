/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define holy_video_render_target holy_video_fbrender_target

#include <holy/err.h>
#include <holy/machine/memory.h>
#include <holy/i386/pc/vbe.h>
#include <holy/video_fb.h>
#include <holy/types.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/video.h>
#include <holy/i386/pc/int.h>
#include <holy/i18n.h>
#include <holy/cpu/cpuid.h>

holy_MOD_LICENSE ("GPLv2+");

static int vbe_detected = -1;

static struct holy_vbe_info_block controller_info;

/* Track last mode to support cards which fail on get_mode.  */
static holy_uint32_t last_set_mode = 3;

static struct
{
  struct holy_video_mode_info mode_info;

  holy_uint8_t *ptr;
  int mtrr;
} framebuffer;

static holy_uint32_t initial_vbe_mode;
static holy_uint16_t *vbe_mode_list;

static void *
real2pm (holy_vbe_farptr_t ptr)
{
  return (void *) ((((unsigned long) ptr & 0xFFFF0000) >> 12UL)
                   + ((unsigned long) ptr & 0x0000FFFF));
}

#define rdmsr(num,a,d) \
  asm volatile ("rdmsr" : "=a" (a), "=d" (d) : "c" (num))

#define wrmsr(num,lo,hi) \
  asm volatile ("wrmsr" : : "c" (num), "a" (lo), "d" (hi) : "memory")

#define mtrr_base(reg) (0x200 + (reg) * 2)
#define mtrr_mask(reg) (0x200 + (reg) * 2 + 1)

/* Try to set up a variable-range write-combining MTRR for a memory region.
   This is best-effort; if it seems too hard, we just accept the performance
   degradation rather than risking undefined behaviour.  It is intended
   exclusively to work around BIOS bugs, as the BIOS should already be
   setting up a suitable MTRR.  */
static void
holy_vbe_enable_mtrr_entry (int mtrr)
{
  holy_uint32_t eax, edx;
  holy_uint32_t mask_lo, mask_hi;

  rdmsr (mtrr_mask (mtrr), eax, edx);
  mask_lo = eax;
  mask_hi = edx;

  mask_lo |= 0x800 /* valid */;
  wrmsr (mtrr_mask (mtrr), mask_lo, mask_hi);
}

static void
holy_vbe_enable_mtrr (holy_uint8_t *base, holy_size_t size)
{
  holy_uint32_t eax, ebx, ecx, edx;
  holy_uint32_t features;
  holy_uint32_t mtrrcap;
  int var_mtrrs;
  holy_uint32_t max_extended_cpuid;
  holy_uint32_t maxphyaddr;
  holy_uint64_t fb_base, fb_size;
  holy_uint64_t size_bits, fb_mask;
  holy_uint32_t bits_lo, bits_hi;
  holy_uint64_t bits;
  int i, first_unused = -1;
  holy_uint32_t base_lo, base_hi, mask_lo, mask_hi;

  fb_base = (holy_uint64_t) (holy_size_t) base;
  fb_size = (holy_uint64_t) size;

  /* Check that fb_base and fb_size can be represented using a single
     MTRR.  */

  if (fb_base < (1 << 20))
    return; /* under 1MB, so covered by fixed-range MTRRs */
  if (fb_base >= (1LL << 36))
    return; /* over 36 bits, so out of range */
  if (fb_size < (1 << 12))
    return; /* variable-range MTRRs must cover at least 4KB */

  size_bits = fb_size;
  while (size_bits > 1)
    size_bits >>= 1;
  if (size_bits != 1)
    return; /* not a power of two */

  if (fb_base & (fb_size - 1))
    return; /* not aligned on size boundary */

  fb_mask = ~(fb_size - 1);

  /* Check CPU capabilities.  */

  if (! holy_cpu_is_cpuid_supported ())
    return;

  holy_cpuid (1, eax, ebx, ecx, edx);
  features = edx;
  if (! (features & 0x00001000)) /* MTRR */
    return;

  rdmsr (0xFE, eax, edx);
  mtrrcap = eax;
  if (! (mtrrcap & 0x00000400)) /* write-combining */
    return;
  var_mtrrs = (mtrrcap & 0xFF);

  holy_cpuid (0x80000000, eax, ebx, ecx, edx);
  max_extended_cpuid = eax;
  if (max_extended_cpuid >= 0x80000008)
    {
      holy_cpuid (0x80000008, eax, ebx, ecx, edx);
      maxphyaddr = (eax & 0xFF);
    }
  else
    maxphyaddr = 36;
  bits_lo = 0xFFFFF000; /* assume maxphyaddr >= 36 */
  bits_hi = (1 << (maxphyaddr - 32)) - 1;
  bits = bits_lo | ((holy_uint64_t) bits_hi << 32);

  /* Check whether an MTRR already covers this region.  If not, take an
     unused one if possible.  */
  for (i = 0; i < var_mtrrs; i++)
    {
      rdmsr (mtrr_mask (i), eax, edx);
      mask_lo = eax;
      mask_hi = edx;
      if (mask_lo & 0x800) /* valid */
	{
	  holy_uint64_t real_base, real_mask;

	  rdmsr (mtrr_base (i), eax, edx);
	  base_lo = eax;
	  base_hi = edx;

	  real_base = ((holy_uint64_t) (base_hi & bits_hi) << 32) |
		      (base_lo & bits_lo);
	  real_mask = ((holy_uint64_t) (mask_hi & bits_hi) << 32) |
		       (mask_lo & bits_lo);
	  if (real_base < (fb_base + fb_size) &&
	      real_base + (~real_mask & bits) >= fb_base)
	    return; /* existing MTRR overlaps this region */
	}
      else if (first_unused < 0)
	first_unused = i;
    }

  if (first_unused < 0)
    return; /* all MTRRs in use */

  /* Set up the first unused MTRR we found.  */
  rdmsr (mtrr_base (first_unused), eax, edx);
  base_lo = eax;
  base_hi = edx;
  rdmsr (mtrr_mask (first_unused), eax, edx);
  mask_lo = eax;
  mask_hi = edx;

  base_lo = (base_lo & ~bits_lo & ~0xFF) |
	    (fb_base & bits_lo) | 0x01 /* WC */;
  base_hi = (base_hi & ~bits_hi) | ((fb_base >> 32) & bits_hi);
  wrmsr (mtrr_base (first_unused), base_lo, base_hi);
  mask_lo = (mask_lo & ~bits_lo) | (fb_mask & bits_lo) | 0x800 /* valid */;
  mask_hi = (mask_hi & ~bits_hi) | ((fb_mask >> 32) & bits_hi);
  wrmsr (mtrr_mask (first_unused), mask_lo, mask_hi);

  framebuffer.mtrr = first_unused;
}

static void
holy_vbe_disable_mtrr (int mtrr)
{
  holy_uint32_t eax, edx;
  holy_uint32_t mask_lo, mask_hi;

  rdmsr (mtrr_mask (mtrr), eax, edx);
  mask_lo = eax;
  mask_hi = edx;

  mask_lo &= ~0x800 /* valid */;
  wrmsr (mtrr_mask (mtrr), mask_lo, mask_hi);
}

/* Call VESA BIOS 0x4f09 to set palette data, return status.  */
static holy_vbe_status_t
holy_vbe_bios_set_palette_data (holy_uint32_t color_count,
				holy_uint32_t start_index,
				struct holy_vbe_palette_data *palette_data)
{
  struct holy_bios_int_registers regs;
  regs.eax = 0x4f09;
  regs.ebx = 0;
  regs.ecx = color_count;
  regs.edx = start_index;
  regs.es = (((holy_addr_t) palette_data) & 0xffff0000) >> 4;
  regs.edi = ((holy_addr_t) palette_data) & 0xffff;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f00 to get VBE Controller Information, return status.  */
holy_vbe_status_t
holy_vbe_bios_get_controller_info (struct holy_vbe_info_block *ci)
{
  struct holy_bios_int_registers regs;
  /* Store *controller_info to %es:%di.  */
  regs.es = (((holy_addr_t) ci) & 0xffff0000) >> 4;
  regs.edi = ((holy_addr_t) ci) & 0xffff;
  regs.eax = 0x4f00;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f01 to get VBE Mode Information, return status.  */
holy_vbe_status_t
holy_vbe_bios_get_mode_info (holy_uint32_t mode,
			     struct holy_vbe_mode_info_block *mode_info)
{
  struct holy_bios_int_registers regs;
  regs.eax = 0x4f01;
  regs.ecx = mode;
  /* Store *mode_info to %es:%di.  */
  regs.es = ((holy_addr_t) mode_info & 0xffff0000) >> 4;
  regs.edi = (holy_addr_t) mode_info & 0x0000ffff;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f02 to set video mode, return status.  */
static holy_vbe_status_t
holy_vbe_bios_set_mode (holy_uint32_t mode,
			struct holy_vbe_crtc_info_block *crtc_info)
{
  struct holy_bios_int_registers regs;

  regs.eax = 0x4f02;
  regs.ebx = mode;
  /* Store *crtc_info to %es:%di.  */
  regs.es = (((holy_addr_t) crtc_info) & 0xffff0000) >> 4;
  regs.edi = ((holy_addr_t) crtc_info) & 0xffff;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);

  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f03 to return current VBE Mode, return status.  */
holy_vbe_status_t
holy_vbe_bios_get_mode (holy_uint32_t *mode)
{
  struct holy_bios_int_registers regs;

  regs.eax = 0x4f03;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);
  *mode = regs.ebx & 0xffff;

  return regs.eax & 0xffff;
}

holy_vbe_status_t
holy_vbe_bios_getset_dac_palette_width (int set, int *dac_mask_size)
{
  struct holy_bios_int_registers regs;

  regs.eax = 0x4f08;
  regs.ebx = ((*dac_mask_size & 0xff) << 8) | (set ? 1 : 0);
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);
  *dac_mask_size = (regs.ebx >> 8) & 0xff;

  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f05 to set memory window, return status.  */
holy_vbe_status_t
holy_vbe_bios_set_memory_window (holy_uint32_t window,
				 holy_uint32_t position)
{
  struct holy_bios_int_registers regs;

  /* BL = window, BH = 0, Set memory window.  */
  regs.ebx = window & 0x00ff;
  regs.edx = position;
  regs.eax = 0x4f05;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f05 to return memory window, return status.  */
holy_vbe_status_t
holy_vbe_bios_get_memory_window (holy_uint32_t window,
				 holy_uint32_t *position)
{
  struct holy_bios_int_registers regs;

  regs.eax = 0x4f05;
  /* BH = 1, Get memory window. BL = window.  */
  regs.ebx = (window & 0x00ff) | 0x100;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);

  *position = regs.edx & 0xffff;
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f06 to set scanline length (in bytes), return status.  */
holy_vbe_status_t
holy_vbe_bios_set_scanline_length (holy_uint32_t length)
{
  struct holy_bios_int_registers regs;

  regs.ecx = length;
  regs.eax = 0x4f06;
  /* BL = 2, Set Scan Line in Bytes.  */
  regs.ebx = 0x0002;	
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f06 to return scanline length (in bytes), return status.  */
holy_vbe_status_t
holy_vbe_bios_get_scanline_length (holy_uint32_t *length)
{
  struct holy_bios_int_registers regs;

  regs.eax = 0x4f06;
  regs.ebx = 0x0001;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  /* BL = 1, Get Scan Line Length (in bytes).  */
  holy_bios_interrupt (0x10, &regs);

  *length = regs.ebx & 0xffff;
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f07 to set display start, return status.  */
static holy_vbe_status_t
holy_vbe_bios_set_display_start (holy_uint32_t x, holy_uint32_t y)
{
  struct holy_bios_int_registers regs;

  if (framebuffer.mtrr >= 0)
    holy_vbe_disable_mtrr (framebuffer.mtrr);

  /* Store x in %ecx.  */
  regs.ecx = x;
  regs.edx = y;
  regs.eax = 0x4f07;
  /* BL = 80h, Set Display Start during Vertical Retrace.  */
  regs.ebx = 0x0080;	
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);

  if (framebuffer.mtrr >= 0)
    holy_vbe_enable_mtrr_entry (framebuffer.mtrr);

  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f07 to get display start, return status.  */
holy_vbe_status_t
holy_vbe_bios_get_display_start (holy_uint32_t *x,
				 holy_uint32_t *y)
{
  struct holy_bios_int_registers regs;

  regs.eax = 0x4f07;
  /* BL = 1, Get Display Start.  */
  regs.ebx = 0x0001;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);

  *x = regs.ecx & 0xffff;
  *y = regs.edx & 0xffff;
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f0a.  */
holy_vbe_status_t
holy_vbe_bios_get_pm_interface (holy_uint16_t *segment, holy_uint16_t *offset,
				holy_uint16_t *length)
{
  struct holy_bios_int_registers regs;

  regs.eax = 0x4f0a;
  regs.ebx = 0x0000;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);

  if ((regs.eax & 0xffff) != holy_VBE_STATUS_OK)
    {
      *segment = 0;
      *offset = 0;
      *length = 0;
    }

  *segment = regs.es & 0xffff;
  *offset = regs.edi & 0xffff;
  *length = regs.ecx & 0xffff;
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f11 to get flat panel information, return status.  */
static holy_vbe_status_t
holy_vbe_bios_get_flat_panel_info (struct holy_vbe_flat_panel_info *flat_panel_info)
{
  struct holy_bios_int_registers regs;

  regs.eax = 0x4f11;
  regs.ebx = 0x0001;
  regs.es = (((holy_addr_t) flat_panel_info) & 0xffff0000) >> 4;
  regs.edi = ((holy_addr_t) flat_panel_info) & 0xffff;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f15 to get DDC availability, return status.  */
static holy_vbe_status_t
holy_vbe_bios_get_ddc_capabilities (holy_uint8_t *level)
{
  struct holy_bios_int_registers regs;

  regs.eax = 0x4f15;
  regs.ebx = 0x0000;
  regs.ecx = 0x0000;
  regs.es = 0x0000;
  regs.edi = 0x0000;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);

  *level = regs.ebx & 0xff;
  return regs.eax & 0xffff;
}

/* Call VESA BIOS 0x4f15 to read EDID information, return status.  */
static holy_vbe_status_t
holy_vbe_bios_read_edid (struct holy_video_edid_info *edid_info)
{
  struct holy_bios_int_registers regs;

  regs.eax = 0x4f15;
  regs.ebx = 0x0001;
  regs.ecx = 0x0000;
  regs.edx = 0x0000;
  regs.es = (((holy_addr_t) edid_info) & 0xffff0000) >> 4;
  regs.edi = ((holy_addr_t) edid_info) & 0xffff;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);
  return regs.eax & 0xffff;
}

holy_err_t
holy_vbe_probe (struct holy_vbe_info_block *info_block)
{
  struct holy_vbe_info_block *vbe_ib;
  holy_vbe_status_t status;

  /* Clear caller's controller info block.  */
  if (info_block)
    holy_memset (info_block, 0, sizeof (*info_block));

  /* Do not probe more than one time, if not necessary.  */
  if (vbe_detected == -1 || info_block)
    {
      /* Clear old copy of controller info block.  */
      holy_memset (&controller_info, 0, sizeof (controller_info));

      /* Mark VESA BIOS extension as undetected.  */
      vbe_detected = 0;

      /* Use low memory scratch area as temporary storage
         for VESA BIOS call.  */
      vbe_ib = (struct holy_vbe_info_block *) holy_MEMORY_MACHINE_SCRATCH_ADDR;

      /* Prepare info block.  */
      holy_memset (vbe_ib, 0, sizeof (*vbe_ib));

      vbe_ib->signature[0] = 'V';
      vbe_ib->signature[1] = 'B';
      vbe_ib->signature[2] = 'E';
      vbe_ib->signature[3] = '2';

      /* Try to get controller info block.  */
      status = holy_vbe_bios_get_controller_info (vbe_ib);
      if (status == holy_VBE_STATUS_OK)
        {
          /* Copy it for later usage.  */
          holy_memcpy (&controller_info, vbe_ib, sizeof (controller_info));

          /* Mark VESA BIOS extension as detected.  */
          vbe_detected = 1;
        }
    }

  if (! vbe_detected)
    return holy_error (holy_ERR_BAD_DEVICE, "VESA BIOS Extension not found");

  /* Make copy of controller info block to caller.  */
  if (info_block)
    holy_memcpy (info_block, &controller_info, sizeof (*info_block));

  return holy_ERR_NONE;
}

static holy_err_t
holy_video_vbe_get_edid (struct holy_video_edid_info *edid_info)
{
  struct holy_video_edid_info *edid_info_lowmem;

  /* Use low memory scratch area as temporary storage for VESA BIOS calls.  */
  edid_info_lowmem =
    (struct holy_video_edid_info *) holy_MEMORY_MACHINE_SCRATCH_ADDR;
  holy_memset (edid_info_lowmem, 0, sizeof (*edid_info_lowmem));

  if (holy_vbe_bios_read_edid (edid_info_lowmem) != holy_VBE_STATUS_OK)
    return holy_error (holy_ERR_BAD_DEVICE, "EDID information not available");

  holy_memcpy (edid_info, edid_info_lowmem, sizeof (*edid_info));

  return holy_ERR_NONE;
}

static holy_err_t
holy_vbe_get_preferred_mode (unsigned int *width, unsigned int *height)
{
  holy_vbe_status_t status;
  holy_uint8_t ddc_level;
  struct holy_video_edid_info edid_info;
  struct holy_vbe_flat_panel_info *flat_panel_info;

  /* Use low memory scratch area as temporary storage for VESA BIOS calls.  */
  flat_panel_info = (struct holy_vbe_flat_panel_info *)
    (holy_MEMORY_MACHINE_SCRATCH_ADDR + sizeof (struct holy_video_edid_info));

  if (controller_info.version >= 0x200
      && (holy_vbe_bios_get_ddc_capabilities (&ddc_level) & 0xff)
	 == holy_VBE_STATUS_OK)
    {
      if (holy_video_vbe_get_edid (&edid_info) == holy_ERR_NONE
	  && holy_video_edid_checksum (&edid_info) == holy_ERR_NONE
	  && holy_video_edid_preferred_mode (&edid_info, width, height)
	      == holy_ERR_NONE && *width < 4096 && *height < 4096)
	return holy_ERR_NONE;

      holy_errno = holy_ERR_NONE;
    }

  holy_memset (flat_panel_info, 0, sizeof (*flat_panel_info));
  status = holy_vbe_bios_get_flat_panel_info (flat_panel_info);
  if (status == holy_VBE_STATUS_OK
      && flat_panel_info->horizontal_size && flat_panel_info->vertical_size
      && flat_panel_info->horizontal_size < 4096
      && flat_panel_info->vertical_size < 4096)
    {
      *width = flat_panel_info->horizontal_size;
      *height = flat_panel_info->vertical_size;
      return holy_ERR_NONE;
    }

  return holy_error (holy_ERR_UNKNOWN_DEVICE, "cannot get preferred mode");
}

static holy_err_t
holy_vbe_set_video_mode (holy_uint32_t vbe_mode,
			 struct holy_vbe_mode_info_block *vbe_mode_info)
{
  holy_vbe_status_t status;
  holy_uint32_t old_vbe_mode;
  struct holy_vbe_mode_info_block new_vbe_mode_info;
  holy_err_t err;

  /* Make sure that VBE is supported.  */
  holy_vbe_probe (0);
  if (holy_errno != holy_ERR_NONE)
    return holy_errno;

  /* Try to get mode info.  */
  holy_vbe_get_video_mode_info (vbe_mode, &new_vbe_mode_info);
  if (holy_errno != holy_ERR_NONE)
    return holy_errno;

  /* For all VESA BIOS modes, force linear frame buffer.  */
  if (vbe_mode >= 0x100)
    {
      /* We only want linear frame buffer modes.  */
      vbe_mode |= 1 << 14;

      /* Determine frame buffer pixel format.  */
      if (new_vbe_mode_info.memory_model != holy_VBE_MEMORY_MODEL_PACKED_PIXEL
	  && new_vbe_mode_info.memory_model
	  != holy_VBE_MEMORY_MODEL_DIRECT_COLOR)
	return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
			   "unsupported pixel format 0x%x",
			   new_vbe_mode_info.memory_model);
    }

  /* Get current mode.  */
  holy_vbe_get_video_mode (&old_vbe_mode);
  if (holy_errno != holy_ERR_NONE)
    return holy_errno;

  /* Try to set video mode.  */
  status = holy_vbe_bios_set_mode (vbe_mode, 0);
  if (status != holy_VBE_STATUS_OK)
    return holy_error (holy_ERR_BAD_DEVICE, "cannot set VBE mode %x", vbe_mode);
  last_set_mode = vbe_mode;

  if (vbe_mode < 0x100)
    {
      /* If this is not a VESA mode, guess address.  */
      framebuffer.ptr = (holy_uint8_t *) 0xa0000;
    }
  else
    {
      framebuffer.ptr = (holy_uint8_t *) new_vbe_mode_info.phys_base_addr;
    }

  /* Check whether mode is text mode or graphics mode.  */
  if (new_vbe_mode_info.memory_model == holy_VBE_MEMORY_MODEL_TEXT)
    {
      /* Text mode.  */

      /* No special action needed for text mode as it is not supported for
         graphical support.  */
    }
  else
    {
      /* Graphics mode.  */

      /* If video mode is in indexed color, setup default VGA palette.  */
      if (vbe_mode < 0x100 || new_vbe_mode_info.memory_model
	  == holy_VBE_MEMORY_MODEL_PACKED_PIXEL)
	{
	  struct holy_vbe_palette_data *palette
	    = (struct holy_vbe_palette_data *) holy_MEMORY_MACHINE_SCRATCH_ADDR;
	  unsigned i;

	  /* Make sure that the BIOS can reach the palette.  */
	  for (i = 0; i < holy_VIDEO_FBSTD_NUMCOLORS; i++)
	    {
	      palette[i].red = holy_video_fbstd_colors[i].r;
	      palette[i].green = holy_video_fbstd_colors[i].g;
	      palette[i].blue = holy_video_fbstd_colors[i].b;
	      palette[i].alignment = 0;
	    }

	  status = holy_vbe_bios_set_palette_data (holy_VIDEO_FBSTD_NUMCOLORS,
						   0, palette);

	  /* Just ignore the status.  */
	  err = holy_video_fb_set_palette (0, holy_VIDEO_FBSTD_NUMCOLORS,
					   holy_video_fbstd_colors);
	  if (err)
	    return err;

	}
    }

  /* Copy mode info for caller.  */
  if (vbe_mode_info)
    holy_memcpy (vbe_mode_info, &new_vbe_mode_info, sizeof (*vbe_mode_info));

  return holy_ERR_NONE;
}

holy_err_t
holy_vbe_get_video_mode (holy_uint32_t *mode)
{
  holy_vbe_status_t status;

  /* Make sure that VBE is supported.  */
  holy_vbe_probe (0);
  if (holy_errno != holy_ERR_NONE)
    return holy_errno;

  /* Try to query current mode from VESA BIOS.  */
  status = holy_vbe_bios_get_mode (mode);
  /* XXX: ATI cards don't support get_mode.  */
  if (status != holy_VBE_STATUS_OK)
    *mode = last_set_mode;

  return holy_ERR_NONE;
}

holy_err_t
holy_vbe_get_video_mode_info (holy_uint32_t mode,
                              struct holy_vbe_mode_info_block *mode_info)
{
  struct holy_vbe_mode_info_block *mi_tmp
    = (struct holy_vbe_mode_info_block *) holy_MEMORY_MACHINE_SCRATCH_ADDR;
  holy_vbe_status_t status;

  /* Make sure that VBE is supported.  */
  holy_vbe_probe (0);
  if (holy_errno != holy_ERR_NONE)
    return holy_errno;

  /* If mode is not VESA mode, skip mode info query.  */
  if (mode >= 0x100)
    {
      /* Try to get mode info from VESA BIOS.  */
      status = holy_vbe_bios_get_mode_info (mode, mi_tmp);
      if (status != holy_VBE_STATUS_OK)
        return holy_error (holy_ERR_BAD_DEVICE,
                           "cannot get information on the mode %x", mode);

      /* Make copy of mode info block.  */
      holy_memcpy (mode_info, mi_tmp, sizeof (*mode_info));
    }
  else
    /* Just clear mode info block if it isn't a VESA mode.  */
    holy_memset (mode_info, 0, sizeof (*mode_info));

  return holy_ERR_NONE;
}

static holy_err_t
holy_video_vbe_init (void)
{
  holy_uint16_t *rm_vbe_mode_list;
  holy_uint16_t *p;
  holy_size_t vbe_mode_list_size;
  struct holy_vbe_info_block info_block;

  /* Check if there is adapter present.

     Firmware note: There has been a report that some cards store video mode
     list in temporary memory.  So we must first use vbe probe to get
     refreshed information to receive valid pointers and data, and then
     copy this information to somewhere safe.  */
  holy_vbe_probe (&info_block);
  if (holy_errno != holy_ERR_NONE)
    return holy_errno;

  /* Copy modelist to local memory.  */
  p = rm_vbe_mode_list = real2pm (info_block.video_mode_ptr);
  while(*p++ != 0xFFFF)
    ;

  vbe_mode_list_size = (holy_addr_t) p - (holy_addr_t) rm_vbe_mode_list;
  vbe_mode_list = holy_malloc (vbe_mode_list_size);
  if (! vbe_mode_list)
    return holy_errno;
  holy_memcpy (vbe_mode_list, rm_vbe_mode_list, vbe_mode_list_size);

  /* Adapter could be found, figure out initial video mode.  */
  holy_vbe_get_video_mode (&initial_vbe_mode);
  if (holy_errno != holy_ERR_NONE)
    {
      /* Free allocated resources.  */
      holy_free (vbe_mode_list);
      vbe_mode_list = NULL;

      return holy_errno;
    }

  /* Reset frame buffer.  */
  holy_memset (&framebuffer, 0, sizeof(framebuffer));
  framebuffer.mtrr = -1;

  return holy_video_fb_init ();
}

static holy_err_t
holy_video_vbe_fini (void)
{
  holy_vbe_status_t status;
  holy_err_t err;

  /* Restore old video mode.  */
  if (last_set_mode != initial_vbe_mode)
    {
      status = holy_vbe_bios_set_mode (initial_vbe_mode, 0);
      if (status != holy_VBE_STATUS_OK)
	/* TODO: Decide, is this something we want to do.  */
	return holy_errno;
    }
  last_set_mode = initial_vbe_mode;

  /* TODO: Free any resources allocated by driver.  */
  holy_free (vbe_mode_list);
  vbe_mode_list = NULL;

  err = holy_video_fb_fini ();
  if (framebuffer.mtrr >= 0)
    {
      holy_vbe_disable_mtrr (framebuffer.mtrr);
      framebuffer.mtrr = -1;
    }
  return err;
}

/*
  Set framebuffer render target page and display the proper page, based on
  `doublebuf_state.render_page' and `doublebuf_state.displayed_page',
  respectively.
*/
static holy_err_t
doublebuf_pageflipping_set_page (int page)
{
  /* Tell the video adapter to display the new front page.  */
  int display_start_line
    = framebuffer.mode_info.height * page;

  holy_vbe_status_t vbe_err =
    holy_vbe_bios_set_display_start (0, display_start_line);

  if (vbe_err != holy_VBE_STATUS_OK)
    return holy_error (holy_ERR_IO, "couldn't commit pageflip");

  return 0;
}

static void
vbe2videoinfo (holy_uint32_t mode,
	       const struct holy_vbe_mode_info_block *vbeinfo,
	       struct holy_video_mode_info *mode_info)
{
  mode_info->mode_number = mode;

  mode_info->width = vbeinfo->x_resolution;
  mode_info->height = vbeinfo->y_resolution;
  mode_info->mode_type = 0;
  switch (vbeinfo->memory_model)
    {
    case holy_VBE_MEMORY_MODEL_TEXT:
      mode_info->mode_type |= holy_VIDEO_MODE_TYPE_PURE_TEXT;
      break;

      /* CGA is basically 4-bit packed pixel.  */
    case holy_VBE_MEMORY_MODEL_CGA:
      mode_info->mode_type |= holy_VIDEO_MODE_TYPE_CGA;
      /* Fallthrough.  */
    case holy_VBE_MEMORY_MODEL_PACKED_PIXEL:
      mode_info->mode_type |= holy_VIDEO_MODE_TYPE_INDEX_COLOR;
      break;

    case holy_VBE_MEMORY_MODEL_HERCULES:
      mode_info->mode_type |= holy_VIDEO_MODE_TYPE_HERCULES
	| holy_VIDEO_MODE_TYPE_1BIT_BITMAP;
      break;

      /* Non chain 4 is a special case of planar.  */
    case holy_VBE_MEMORY_MODEL_NONCHAIN4_256:
      mode_info->mode_type |= holy_VIDEO_MODE_TYPE_NONCHAIN4;
      /* Fallthrough.  */
    case holy_VBE_MEMORY_MODEL_PLANAR:
      mode_info->mode_type |= holy_VIDEO_MODE_TYPE_PLANAR
	| holy_VIDEO_MODE_TYPE_INDEX_COLOR;
      break;

    case holy_VBE_MEMORY_MODEL_YUV:
      mode_info->mode_type |= holy_VIDEO_MODE_TYPE_YUV;
      break;
      
    case holy_VBE_MEMORY_MODEL_DIRECT_COLOR:
      mode_info->mode_type |= holy_VIDEO_MODE_TYPE_RGB;
      break;
    default:
      mode_info->mode_type |= holy_VIDEO_MODE_TYPE_UNKNOWN;
      break;
    }

  mode_info->bpp = vbeinfo->bits_per_pixel;
  /* Calculate bytes_per_pixel value.  */
  switch(vbeinfo->bits_per_pixel)
    {
    case 32:
      mode_info->bytes_per_pixel = 4;
      break;
    case 24:
      mode_info->bytes_per_pixel = 3;
      break;
    case 16:
      mode_info->bytes_per_pixel = 2;
      break;
    case 15:
      mode_info->bytes_per_pixel = 2;
      break;
    case 8:
      mode_info->bytes_per_pixel = 1;
      break;  
    case 4:
      mode_info->bytes_per_pixel = 0;
      break;  
    }

  if (controller_info.version >= 0x300)
    mode_info->pitch = vbeinfo->lin_bytes_per_scan_line;
  else
    mode_info->pitch = vbeinfo->bytes_per_scan_line;

  if (mode_info->mode_type & holy_VIDEO_MODE_TYPE_INDEX_COLOR)
    mode_info->number_of_colors = 16;
  else
    mode_info->number_of_colors = 256;
  mode_info->red_mask_size = vbeinfo->red_mask_size;
  mode_info->red_field_pos = vbeinfo->red_field_position;
  mode_info->green_mask_size = vbeinfo->green_mask_size;
  mode_info->green_field_pos = vbeinfo->green_field_position;
  mode_info->blue_mask_size = vbeinfo->blue_mask_size;
  mode_info->blue_field_pos = vbeinfo->blue_field_position;
  mode_info->reserved_mask_size = vbeinfo->rsvd_mask_size;
  mode_info->reserved_field_pos = vbeinfo->rsvd_field_position;

  mode_info->blit_format = holy_video_get_blit_format (mode_info);
}

static int
holy_video_vbe_iterate (int (*hook) (const struct holy_video_mode_info *info, void *hook_arg), void *hook_arg)
{
  holy_uint16_t *p;
  struct holy_vbe_mode_info_block vbe_mode_info;
  struct holy_video_mode_info mode_info;

  for (p = vbe_mode_list; *p != 0xFFFF; p++)
    {
      holy_vbe_get_video_mode_info (*p, &vbe_mode_info);
      if (holy_errno != holy_ERR_NONE)
        {
          /* Could not retrieve mode info, retreat.  */
          holy_errno = holy_ERR_NONE;
          break;
        }

      vbe2videoinfo (*p, &vbe_mode_info, &mode_info);
      if (hook (&mode_info, hook_arg))
	return 1;
    }
  return 0;
}

static holy_err_t
holy_video_vbe_setup (unsigned int width, unsigned int height,
                      holy_video_mode_type_t mode_type,
		      holy_video_mode_type_t mode_mask)
{
  holy_uint16_t *p;
  struct holy_vbe_mode_info_block vbe_mode_info;
  struct holy_vbe_mode_info_block best_vbe_mode_info;
  holy_uint32_t best_vbe_mode = 0;
  int depth;
  int preferred_mode = 0;

  /* Decode depth from mode_type.  If it is zero, then autodetect.  */
  depth = (mode_type & holy_VIDEO_MODE_TYPE_DEPTH_MASK)
          >> holy_VIDEO_MODE_TYPE_DEPTH_POS;

  if (width == 0 && height == 0)
    {
      holy_vbe_get_preferred_mode (&width, &height);
      if (holy_errno == holy_ERR_NONE)
	preferred_mode = 1;
      else
	{
	  /* Fall back to 640x480.  This is conservative, but the largest
	     mode supported by the graphics card may not be safe for the
	     display device.  */
	  holy_errno = holy_ERR_NONE;
	  width = 640;
	  height = 480;
	}
    }

  /* Walk thru mode list and try to find matching mode.  */
  for (p = vbe_mode_list; *p != 0xFFFF; p++)
    {
      holy_uint32_t vbe_mode = *p;

      holy_vbe_get_video_mode_info (vbe_mode, &vbe_mode_info);
      if (holy_errno != holy_ERR_NONE)
        {
          /* Could not retrieve mode info, retreat.  */
          holy_errno = holy_ERR_NONE;
          break;
        }

      if ((vbe_mode_info.mode_attributes & holy_VBE_MODEATTR_SUPPORTED) == 0)
        /* If not available, skip it.  */
        continue;

      if ((vbe_mode_info.mode_attributes & holy_VBE_MODEATTR_COLOR) == 0)
        /* Monochrome is unusable.  */
        continue;

      if ((vbe_mode_info.mode_attributes & holy_VBE_MODEATTR_LFB_AVAIL) == 0)
        /* We support only linear frame buffer modes.  */
        continue;

      if ((vbe_mode_info.mode_attributes & holy_VBE_MODEATTR_GRAPHICS) == 0)
        /* We allow only graphical modes.  */
        continue;

      if ((vbe_mode_info.memory_model != holy_VBE_MEMORY_MODEL_PACKED_PIXEL)
          && (vbe_mode_info.memory_model != holy_VBE_MEMORY_MODEL_DIRECT_COLOR))
        /* Not compatible memory model.  */
        continue;

      if (vbe_mode_info.bits_per_pixel != 8
	  && vbe_mode_info.bits_per_pixel != 15
	  && vbe_mode_info.bits_per_pixel != 16
	  && vbe_mode_info.bits_per_pixel != 24
	  && vbe_mode_info.bits_per_pixel != 32)
	/* Unsupported bitdepth . */
        continue;

      if (preferred_mode)
	{
	  if (vbe_mode_info.x_resolution > width
	      || vbe_mode_info.y_resolution > height)
	    /* Resolution exceeds that of preferred mode.  */
	    continue;
	}
      else
	{
	  if (((vbe_mode_info.x_resolution != width)
	       || (vbe_mode_info.y_resolution != height))
	      && width != 0 && height != 0)
	    /* Non matching resolution.  */
	    continue;
	}

      /* Check if user requested RGB or index color mode.  */
      if ((mode_mask & holy_VIDEO_MODE_TYPE_COLOR_MASK) != 0)
        {
	  unsigned my_mode_type = 0;

	  if (vbe_mode_info.memory_model == holy_VBE_MEMORY_MODEL_PACKED_PIXEL)
	    my_mode_type |= holy_VIDEO_MODE_TYPE_INDEX_COLOR;

	  if (vbe_mode_info.memory_model == holy_VBE_MEMORY_MODEL_DIRECT_COLOR)
	    my_mode_type |= holy_VIDEO_MODE_TYPE_RGB;

	  if ((my_mode_type & mode_mask
	       & (holy_VIDEO_MODE_TYPE_RGB | holy_VIDEO_MODE_TYPE_INDEX_COLOR))
	      != (mode_type & mode_mask
		  & (holy_VIDEO_MODE_TYPE_RGB
		     | holy_VIDEO_MODE_TYPE_INDEX_COLOR)))
	    continue;
        }

      /* If there is a request for specific depth, ignore others.  */
      if ((depth != 0) && (vbe_mode_info.bits_per_pixel != depth))
        continue;

      /* Select mode with most of "volume" (size of framebuffer in bits).  */
      if (best_vbe_mode != 0)
        if ((holy_uint64_t) vbe_mode_info.bits_per_pixel
	    * vbe_mode_info.x_resolution * vbe_mode_info.y_resolution
	    < (holy_uint64_t) best_vbe_mode_info.bits_per_pixel
	    * best_vbe_mode_info.x_resolution * best_vbe_mode_info.y_resolution)
          continue;

      /* Save so far best mode information for later use.  */
      best_vbe_mode = vbe_mode;
      holy_memcpy (&best_vbe_mode_info, &vbe_mode_info, sizeof (vbe_mode_info));
    }

  /* Try to initialize best mode found.  */
  if (best_vbe_mode != 0)
    {
      holy_err_t err;
      static struct holy_vbe_mode_info_block active_vbe_mode_info;
      /* If this fails, then we have mode selection heuristics problem,
         or adapter failure.  */
      holy_vbe_set_video_mode (best_vbe_mode, &active_vbe_mode_info);
      if (holy_errno != holy_ERR_NONE)
        return holy_errno;

      /* Fill mode info details.  */
      vbe2videoinfo (best_vbe_mode, &active_vbe_mode_info,
		     &framebuffer.mode_info);

      {
	/* Get video RAM size in bytes.  */
	holy_size_t vram_size = controller_info.total_memory << 16;
	holy_size_t page_size;        /* The size of a page in bytes.  */

	page_size = framebuffer.mode_info.pitch * framebuffer.mode_info.height;

	if (vram_size >= 2 * page_size)
	  err = holy_video_fb_setup (mode_type, mode_mask,
				     &framebuffer.mode_info,
				     framebuffer.ptr,
				     doublebuf_pageflipping_set_page,
				     framebuffer.ptr + page_size);
	else
	  err = holy_video_fb_setup (mode_type, mode_mask,
				     &framebuffer.mode_info,
				     framebuffer.ptr, 0, 0);
      }

      /* Copy default palette to initialize emulated palette.  */
      err = holy_video_fb_set_palette (0, holy_VIDEO_FBSTD_NUMCOLORS,
				       holy_video_fbstd_colors);

      holy_vbe_enable_mtrr (framebuffer.ptr,
			    controller_info.total_memory << 16);

      return err;
    }

  /* Couldn't found matching mode.  */
  return holy_error (holy_ERR_UNKNOWN_DEVICE, "no matching mode found");
}

static holy_err_t
holy_video_vbe_set_palette (unsigned int start, unsigned int count,
                            struct holy_video_palette_data *palette_data)
{
  if (framebuffer.mode_info.mode_type == holy_VIDEO_MODE_TYPE_INDEX_COLOR)
    {
      /* TODO: Implement setting indexed color mode palette to hardware.  */
      //status = holy_vbe_bios_set_palette_data (sizeof (vga_colors)
      //                                         / sizeof (struct holy_vbe_palette_data),
      //                                         0,
      //                                         palette);

    }

  /* Then set color to emulated palette.  */

  return holy_video_fb_set_palette (start, count, palette_data);
}

static holy_err_t
holy_video_vbe_get_info_and_fini (struct holy_video_mode_info *mode_info,
				  void **framebuf)
{
  holy_err_t err;
  holy_free (vbe_mode_list);
  vbe_mode_list = NULL;
  err = holy_video_fb_get_info_and_fini (mode_info, framebuf);
  if (err)
    return err;
  if (framebuffer.mtrr >= 0)
    {
      holy_vbe_disable_mtrr (framebuffer.mtrr);
      framebuffer.mtrr = -1;
    }
  return holy_ERR_NONE;
}

static void
holy_video_vbe_print_adapter_specific_info (void)
{
  holy_printf_ (N_("  VBE info:   version: %d.%d  OEM software rev: %d.%d\n"),
		controller_info.version >> 8,
		controller_info.version & 0xFF,
		controller_info.oem_software_rev >> 8,
		controller_info.oem_software_rev & 0xFF);
  
  /* The total_memory field is in 64 KiB units.  */
  holy_printf_ (N_("              total memory: %d KiB\n"),
		(controller_info.total_memory << 6));
}

static struct holy_video_adapter holy_video_vbe_adapter =
  {
    .name = "VESA BIOS Extension Video Driver",
    .id = holy_VIDEO_DRIVER_VBE,

    .prio = holy_VIDEO_ADAPTER_PRIO_FIRMWARE,

    .init = holy_video_vbe_init,
    .fini = holy_video_vbe_fini,
    .setup = holy_video_vbe_setup,
    .get_info = holy_video_fb_get_info,
    .get_info_and_fini = holy_video_vbe_get_info_and_fini,
    .set_palette = holy_video_vbe_set_palette,
    .get_palette = holy_video_fb_get_palette,
    .set_viewport = holy_video_fb_set_viewport,
    .get_viewport = holy_video_fb_get_viewport,
    .set_region = holy_video_fb_set_region,
    .get_region = holy_video_fb_get_region,
    .set_area_status = holy_video_fb_set_area_status,
    .get_area_status = holy_video_fb_get_area_status,
    .map_color = holy_video_fb_map_color,
    .map_rgb = holy_video_fb_map_rgb,
    .map_rgba = holy_video_fb_map_rgba,
    .unmap_color = holy_video_fb_unmap_color,
    .fill_rect = holy_video_fb_fill_rect,
    .blit_bitmap = holy_video_fb_blit_bitmap,
    .blit_render_target = holy_video_fb_blit_render_target,
    .scroll = holy_video_fb_scroll,
    .swap_buffers = holy_video_fb_swap_buffers,
    .create_render_target = holy_video_fb_create_render_target,
    .delete_render_target = holy_video_fb_delete_render_target,
    .set_active_render_target = holy_video_fb_set_active_render_target,
    .get_active_render_target = holy_video_fb_get_active_render_target,
    .iterate = holy_video_vbe_iterate,
    .get_edid = holy_video_vbe_get_edid,
    .print_adapter_specific_info = holy_video_vbe_print_adapter_specific_info,

    .next = 0
  };

holy_MOD_INIT(video_i386_pc_vbe)
{
  holy_video_register (&holy_video_vbe_adapter);
}

holy_MOD_FINI(video_i386_pc_vbe)
{
  holy_video_unregister (&holy_video_vbe_adapter);
}
