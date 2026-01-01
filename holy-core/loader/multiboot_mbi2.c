/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/memory.h>
#ifdef holy_MACHINE_PCBIOS
#include <holy/machine/biosnum.h>
#include <holy/machine/apm.h>
#include <holy/machine/memory.h>
#endif
#include <holy/multiboot.h>
#include <holy/cpu/multiboot.h>
#include <holy/cpu/relocator.h>
#include <holy/disk.h>
#include <holy/device.h>
#include <holy/partition.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/env.h>
#include <holy/video.h>
#include <holy/acpi.h>
#include <holy/i18n.h>
#include <holy/net.h>
#include <holy/lib/cmdline.h>
#include <holy/tpm.h>

#if defined (holy_MACHINE_EFI)
#include <holy/efi/efi.h>
#endif

#if defined (holy_MACHINE_PCBIOS) || defined (holy_MACHINE_COREBOOT) || defined (holy_MACHINE_MULTIBOOT) || defined (holy_MACHINE_QEMU)
#include <holy/i386/pc/vbe.h>
#define HAS_VGA_TEXT 1
#else
#define HAS_VGA_TEXT 0
#endif

struct module
{
  struct module *next;
  holy_addr_t start;
  holy_size_t size;
  char *cmdline;
  int cmdline_size;
};

static struct module *modules, *modules_last;
static holy_size_t cmdline_size;
static holy_size_t total_modcmd;
static unsigned modcnt;
static char *cmdline = NULL;
static int bootdev_set;
static holy_uint32_t biosdev, slice, part;
static holy_size_t elf_sec_num, elf_sec_entsize;
static unsigned elf_sec_shstrndx;
static void *elf_sections;
static int keep_bs = 0;
static holy_uint32_t load_base_addr;

void
holy_multiboot_add_elfsyms (holy_size_t num, holy_size_t entsize,
			    unsigned shndx, void *data)
{
  elf_sec_num = num;
  elf_sec_shstrndx = shndx;
  elf_sec_entsize = entsize;
  elf_sections = data;
}

static struct multiboot_header *
find_header (holy_properly_aligned_t *buffer, holy_ssize_t len)
{
  struct multiboot_header *header;
  /* Look for the multiboot header in the buffer.  The header should
     be at least 12 bytes and aligned on a 4-byte boundary.  */
  for (header = (struct multiboot_header *) buffer;
       ((char *) header <= (char *) buffer + len - 12);
       header = (struct multiboot_header *) ((holy_uint32_t *) header + MULTIBOOT_HEADER_ALIGN / 4))
    {
      if (header->magic == MULTIBOOT_HEADER_MAGIC
	  && !(header->magic + header->architecture
	       + header->header_length + header->checksum)
	  && header->architecture == MULTIBOOT_ARCHITECTURE_CURRENT)
	return header;
    }
  return NULL;
}

holy_err_t
holy_multiboot_load (holy_file_t file, const char *filename)
{
  holy_ssize_t len;
  struct multiboot_header *header;
  holy_err_t err;
  struct multiboot_header_tag *tag;
  struct multiboot_header_tag_address *addr_tag = NULL;
  struct multiboot_header_tag_relocatable *rel_tag;
  int entry_specified = 0, efi_entry_specified = 0;
  holy_addr_t entry = 0, efi_entry = 0;
  holy_uint32_t console_required = 0;
  struct multiboot_header_tag_framebuffer *fbtag = NULL;
  int accepted_consoles = holy_MULTIBOOT_CONSOLE_EGA_TEXT;
  mbi_load_data_t mld;

  mld.mbi_ver = 2;
  mld.relocatable = 0;

  mld.buffer = holy_malloc (MULTIBOOT_SEARCH);
  if (!mld.buffer)
    return holy_errno;

  len = holy_file_read (file, mld.buffer, MULTIBOOT_SEARCH);
  if (len < 32)
    {
      holy_free (mld.buffer);
      return holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"), filename);
    }

  COMPILE_TIME_ASSERT (MULTIBOOT_HEADER_ALIGN % 4 == 0);

  holy_tpm_measure ((unsigned char *)mld.buffer, len, holy_BINARY_PCR, "holy_multiboot", filename);
  holy_print_error();

  header = find_header (mld.buffer, len);

  if (header == 0)
    {
      holy_free (mld.buffer);
      return holy_error (holy_ERR_BAD_ARGUMENT, "no multiboot header found");
    }

  COMPILE_TIME_ASSERT (MULTIBOOT_TAG_ALIGN % 4 == 0);

  keep_bs = 0;

  for (tag = (struct multiboot_header_tag *) (header + 1);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct multiboot_header_tag *) ((holy_uint32_t *) tag + ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN) / 4))
    switch (tag->type)
      {
      case MULTIBOOT_HEADER_TAG_INFORMATION_REQUEST:
	{
	  unsigned i;
	  struct multiboot_header_tag_information_request *request_tag
	    = (struct multiboot_header_tag_information_request *) tag;
	  if (request_tag->flags & MULTIBOOT_HEADER_TAG_OPTIONAL)
	    break;
	  for (i = 0; i < (request_tag->size - sizeof (*request_tag))
		 / sizeof (request_tag->requests[0]); i++)
	    switch (request_tag->requests[i])
	      {
	      case MULTIBOOT_TAG_TYPE_END:
	      case MULTIBOOT_TAG_TYPE_CMDLINE:
	      case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
	      case MULTIBOOT_TAG_TYPE_MODULE:
	      case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
	      case MULTIBOOT_TAG_TYPE_BOOTDEV:
	      case MULTIBOOT_TAG_TYPE_MMAP:
	      case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
	      case MULTIBOOT_TAG_TYPE_VBE:
	      case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
	      case MULTIBOOT_TAG_TYPE_APM:
	      case MULTIBOOT_TAG_TYPE_EFI32:
	      case MULTIBOOT_TAG_TYPE_EFI64:
	      case MULTIBOOT_TAG_TYPE_ACPI_OLD:
	      case MULTIBOOT_TAG_TYPE_ACPI_NEW:
	      case MULTIBOOT_TAG_TYPE_NETWORK:
	      case MULTIBOOT_TAG_TYPE_EFI_MMAP:
	      case MULTIBOOT_TAG_TYPE_EFI_BS:
	      case MULTIBOOT_TAG_TYPE_EFI32_IH:
	      case MULTIBOOT_TAG_TYPE_EFI64_IH:
	      case MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR:
		break;

	      default:
		holy_free (mld.buffer);
		return holy_error (holy_ERR_UNKNOWN_OS,
				   "unsupported information tag: 0x%x",
				   request_tag->requests[i]);
	      }
	  break;
	}
	       
      case MULTIBOOT_HEADER_TAG_ADDRESS:
	addr_tag = (struct multiboot_header_tag_address *) tag;
	break;

      case MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS:
	entry_specified = 1;
	entry = ((struct multiboot_header_tag_entry_address *) tag)->entry_addr;
	break;

      case MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS_EFI64:
#if defined (holy_MACHINE_EFI) && defined (__x86_64__)
	efi_entry_specified = 1;
	efi_entry = ((struct multiboot_header_tag_entry_address *) tag)->entry_addr;
#endif
	break;

      case MULTIBOOT_HEADER_TAG_CONSOLE_FLAGS:
	if (!(((struct multiboot_header_tag_console_flags *) tag)->console_flags
	    & MULTIBOOT_CONSOLE_FLAGS_EGA_TEXT_SUPPORTED))
	  accepted_consoles &= ~holy_MULTIBOOT_CONSOLE_EGA_TEXT;
	if (((struct multiboot_header_tag_console_flags *) tag)->console_flags
	    & MULTIBOOT_CONSOLE_FLAGS_CONSOLE_REQUIRED)
	  console_required = 1;
	break;

      case MULTIBOOT_HEADER_TAG_FRAMEBUFFER:
	fbtag = (struct multiboot_header_tag_framebuffer *) tag;
	accepted_consoles |= holy_MULTIBOOT_CONSOLE_FRAMEBUFFER;
	break;

      case MULTIBOOT_HEADER_TAG_RELOCATABLE:
	mld.relocatable = 1;
	rel_tag = (struct multiboot_header_tag_relocatable *) tag;
	mld.min_addr = rel_tag->min_addr;
	mld.max_addr = rel_tag->max_addr;
	mld.align = rel_tag->align;
	switch (rel_tag->preference)
	  {
	  case MULTIBOOT_LOAD_PREFERENCE_LOW:
	    mld.preference = holy_RELOCATOR_PREFERENCE_LOW;
	    break;

	  case MULTIBOOT_LOAD_PREFERENCE_HIGH:
	    mld.preference = holy_RELOCATOR_PREFERENCE_HIGH;
	    break;

	  default:
	    mld.preference = holy_RELOCATOR_PREFERENCE_NONE;
	  }
	break;

	/* holy always page-aligns modules.  */
      case MULTIBOOT_HEADER_TAG_MODULE_ALIGN:
	break;

      case MULTIBOOT_HEADER_TAG_EFI_BS:
#ifdef holy_MACHINE_EFI
	keep_bs = 1;
#endif
	break;

      default:
	if (! (tag->flags & MULTIBOOT_HEADER_TAG_OPTIONAL))
	  {
	    holy_free (mld.buffer);
	    return holy_error (holy_ERR_UNKNOWN_OS,
			       "unsupported tag: 0x%x", tag->type);
	  }
	break;
      }

  if (addr_tag && !entry_specified && !(keep_bs && efi_entry_specified))
    {
      holy_free (mld.buffer);
      return holy_error (holy_ERR_UNKNOWN_OS,
			 "load address tag without entry address tag");
    }
 
  if (addr_tag)
    {
      holy_uint64_t load_addr = (addr_tag->load_addr + 1)
	? addr_tag->load_addr : (addr_tag->header_addr
				 - ((char *) header - (char *) mld.buffer));
      int offset = ((char *) header - (char *) mld.buffer -
	   (addr_tag->header_addr - load_addr));
      int load_size = ((addr_tag->load_end_addr == 0) ? file->size - offset :
		       addr_tag->load_end_addr - addr_tag->load_addr);
      holy_size_t code_size;
      void *source;
      holy_relocator_chunk_t ch;

      if (addr_tag->bss_end_addr)
	code_size = (addr_tag->bss_end_addr - load_addr);
      else
	code_size = load_size;

      if (mld.relocatable)
	{
	  if (code_size > mld.max_addr || mld.min_addr > mld.max_addr - code_size)
	    {
	      holy_free (mld.buffer);
	      return holy_error (holy_ERR_BAD_OS, "invalid min/max address and/or load size");
	    }

	  err = holy_relocator_alloc_chunk_align (holy_multiboot_relocator, &ch,
						  mld.min_addr, mld.max_addr - code_size,
						  code_size, mld.align ? mld.align : 1,
						  mld.preference, keep_bs);
	}
      else
	err = holy_relocator_alloc_chunk_addr (holy_multiboot_relocator,
					       &ch, load_addr, code_size);
      if (err)
	{
	  holy_dprintf ("multiboot_loader", "Error loading aout kludge\n");
	  holy_free (mld.buffer);
	  return err;
	}
      mld.link_base_addr = load_addr;
      mld.load_base_addr = get_physical_target_address (ch);
      source = get_virtual_current_address (ch);

      holy_dprintf ("multiboot_loader", "link_base_addr=0x%x, load_base_addr=0x%x, "
		    "load_size=0x%lx, relocatable=%d\n", mld.link_base_addr,
		    mld.load_base_addr, (long) code_size, mld.relocatable);

      if (mld.relocatable)
	holy_dprintf ("multiboot_loader", "align=0x%lx, preference=0x%x, avoid_efi_boot_services=%d\n",
		      (long) mld.align, mld.preference, keep_bs);

      if ((holy_file_seek (file, offset)) == (holy_off_t) -1)
	{
	  holy_free (mld.buffer);
	  return holy_errno;
	}

      holy_file_read (file, source, load_size);
      if (holy_errno)
	{
	  holy_free (mld.buffer);
	  return holy_errno;
	}

      if (addr_tag->bss_end_addr)
	holy_memset ((holy_uint8_t *) source + load_size, 0,
		     addr_tag->bss_end_addr - load_addr - load_size);
    }
  else
    {
      mld.file = file;
      mld.filename = filename;
      mld.avoid_efi_boot_services = keep_bs;
      err = holy_multiboot_load_elf (&mld);
      if (err)
	{
	  holy_free (mld.buffer);
	  return err;
	}
    }

  load_base_addr = mld.load_base_addr;

  if (keep_bs && efi_entry_specified)
    holy_multiboot_payload_eip = efi_entry;
  else if (entry_specified)
    holy_multiboot_payload_eip = entry;

  if (mld.relocatable)
    {
      /*
       * Both branches are mathematically equivalent. However, it looks
       * that real life (C?) is more complicated. I am trying to avoid
       * wrap around here if mld.load_base_addr < mld.link_base_addr.
       * If you look at C operator precedence then everything should work.
       * However, I am not 100% sure that a given compiler will not
       * optimize/break this stuff. So, maybe we should use signed
       * 64-bit int here.
       */
      if (mld.load_base_addr >= mld.link_base_addr)
	holy_multiboot_payload_eip += mld.load_base_addr - mld.link_base_addr;
      else
	holy_multiboot_payload_eip -= mld.link_base_addr - mld.load_base_addr;
    }

  if (fbtag)
    err = holy_multiboot_set_console (holy_MULTIBOOT_CONSOLE_FRAMEBUFFER,
				      accepted_consoles,
				      fbtag->width, fbtag->height,
				      fbtag->depth, console_required);
  else
    err = holy_multiboot_set_console (holy_MULTIBOOT_CONSOLE_EGA_TEXT,
				      accepted_consoles,
				      0, 0, 0, console_required);
  return err;
}

static holy_size_t
acpiv2_size (void)
{
#if holy_MACHINE_HAS_ACPI
  struct holy_acpi_rsdp_v20 *p = holy_acpi_get_rsdpv2 ();

  if (!p)
    return 0;

  return ALIGN_UP (sizeof (struct multiboot_tag_old_acpi)
		   + p->length, MULTIBOOT_TAG_ALIGN);
#else
  return 0;
#endif
}

#ifdef holy_MACHINE_EFI

static holy_efi_uintn_t efi_mmap_size = 0;

/* Find the optimal number of pages for the memory map. Is it better to
   move this code to efi/mm.c?  */
static void
find_efi_mmap_size (void)
{
  efi_mmap_size = (1 << 12);
  while (1)
    {
      int ret;
      holy_efi_memory_descriptor_t *mmap;
      holy_efi_uintn_t desc_size;
      holy_efi_uintn_t cur_mmap_size = efi_mmap_size;

      mmap = holy_malloc (cur_mmap_size);
      if (! mmap)
	return;

      ret = holy_efi_get_memory_map (&cur_mmap_size, mmap, 0, &desc_size, 0);
      holy_free (mmap);

      if (ret < 0)
	return;
      else if (ret > 0)
	break;

      if (efi_mmap_size < cur_mmap_size)
	efi_mmap_size = cur_mmap_size;
      efi_mmap_size += (1 << 12);
    }

  /* Increase the size a bit for safety, because holy allocates more on
     later, and EFI itself may allocate more.  */
  efi_mmap_size += (3 << 12);

  efi_mmap_size = ALIGN_UP (efi_mmap_size, 4096);
}
#endif

static holy_size_t
net_size (void)
{
  struct holy_net_network_level_interface *net;
  holy_size_t ret = 0;

  FOR_NET_NETWORK_LEVEL_INTERFACES(net)
    if (net->dhcp_ack)
      ret += ALIGN_UP (sizeof (struct multiboot_tag_network) + net->dhcp_acklen,
		       MULTIBOOT_TAG_ALIGN);
  return ret;
}

static holy_size_t
holy_multiboot_get_mbi_size (void)
{
#ifdef holy_MACHINE_EFI
  if (!keep_bs && !efi_mmap_size)
    find_efi_mmap_size ();    
#endif
  return 2 * sizeof (holy_uint32_t) + sizeof (struct multiboot_tag)
    + sizeof (struct multiboot_tag)
    + (sizeof (struct multiboot_tag_string)
       + ALIGN_UP (cmdline_size, MULTIBOOT_TAG_ALIGN))
    + (sizeof (struct multiboot_tag_string)
       + ALIGN_UP (sizeof (PACKAGE_STRING), MULTIBOOT_TAG_ALIGN))
    + (modcnt * sizeof (struct multiboot_tag_module) + total_modcmd)
    + ALIGN_UP (sizeof (struct multiboot_tag_basic_meminfo),
		MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP (sizeof (struct multiboot_tag_bootdev), MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP (sizeof (struct multiboot_tag_elf_sections), MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP (elf_sec_entsize * elf_sec_num, MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP ((sizeof (struct multiboot_tag_mmap)
		 + holy_get_multiboot_mmap_count ()
		 * sizeof (struct multiboot_mmap_entry)), MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP (sizeof (struct multiboot_tag_framebuffer), MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP (sizeof (struct multiboot_tag_old_acpi)
		+ sizeof (struct holy_acpi_rsdp_v10), MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP (sizeof (struct multiboot_tag_load_base_addr), MULTIBOOT_TAG_ALIGN)
    + acpiv2_size ()
    + net_size ()
#ifdef holy_MACHINE_EFI
    + ALIGN_UP (sizeof (struct multiboot_tag_efi32), MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP (sizeof (struct multiboot_tag_efi32_ih), MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP (sizeof (struct multiboot_tag_efi64), MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP (sizeof (struct multiboot_tag_efi64_ih), MULTIBOOT_TAG_ALIGN)
    + ALIGN_UP (sizeof (struct multiboot_tag_efi_mmap)
		+ efi_mmap_size, MULTIBOOT_TAG_ALIGN)
#endif
    + sizeof (struct multiboot_tag_vbe) + MULTIBOOT_TAG_ALIGN - 1
    + sizeof (struct multiboot_tag_apm) + MULTIBOOT_TAG_ALIGN - 1;
}

/* Helper for holy_fill_multiboot_mmap.  */
static int
holy_fill_multiboot_mmap_iter (holy_uint64_t addr, holy_uint64_t size,
			       holy_memory_type_t type, void *data)
{
  struct multiboot_mmap_entry **mmap_entry = data;

  (*mmap_entry)->addr = addr;
  (*mmap_entry)->len = size;
  (*mmap_entry)->type = type;
  (*mmap_entry)->zero = 0;
  (*mmap_entry)++;

  return 0;
}

/* Fill previously allocated Multiboot mmap.  */
static void
holy_fill_multiboot_mmap (struct multiboot_tag_mmap *tag)
{
  struct multiboot_mmap_entry *mmap_entry = tag->entries;

  tag->type = MULTIBOOT_TAG_TYPE_MMAP;
  tag->size = sizeof (struct multiboot_tag_mmap)
    + sizeof (struct multiboot_mmap_entry) * holy_get_multiboot_mmap_count ();
  tag->entry_size = sizeof (struct multiboot_mmap_entry);
  tag->entry_version = 0;

  holy_mmap_iterate (holy_fill_multiboot_mmap_iter, &mmap_entry);
}

#if defined (holy_MACHINE_PCBIOS)
static void
fill_vbe_tag (struct multiboot_tag_vbe *tag)
{
  holy_vbe_status_t status;
  void *scratch = (void *) holy_MEMORY_MACHINE_SCRATCH_ADDR;

  tag->type = MULTIBOOT_TAG_TYPE_VBE;
  tag->size = 0;
    
  status = holy_vbe_bios_get_controller_info (scratch);
  if (status != holy_VBE_STATUS_OK)
    return;
  
  holy_memcpy (&tag->vbe_control_info, scratch,
	       sizeof (struct holy_vbe_info_block));
  
  status = holy_vbe_bios_get_mode (scratch);
  tag->vbe_mode = *(holy_uint32_t *) scratch;
  if (status != holy_VBE_STATUS_OK)
    return;

  /* get_mode_info isn't available for mode 3.  */
  if (tag->vbe_mode == 3)
    {
      struct holy_vbe_mode_info_block *mode_info = (void *) &tag->vbe_mode_info;
      holy_memset (mode_info, 0,
		   sizeof (struct holy_vbe_mode_info_block));
      mode_info->memory_model = holy_VBE_MEMORY_MODEL_TEXT;
      mode_info->x_resolution = 80;
      mode_info->y_resolution = 25;
    }
  else
    {
      status = holy_vbe_bios_get_mode_info (tag->vbe_mode, scratch);
      if (status != holy_VBE_STATUS_OK)
	return;
      holy_memcpy (&tag->vbe_mode_info, scratch,
		   sizeof (struct holy_vbe_mode_info_block));
    }      
  holy_vbe_bios_get_pm_interface (&tag->vbe_interface_seg,
				  &tag->vbe_interface_off,
				  &tag->vbe_interface_len);

  tag->size = sizeof (*tag);
}
#endif

static holy_err_t
retrieve_video_parameters (holy_properly_aligned_t **ptrorig)
{
  holy_err_t err;
  struct holy_video_mode_info mode_info;
  void *framebuffer;
  holy_video_driver_id_t driv_id;
  struct holy_video_palette_data palette[256];
  struct multiboot_tag_framebuffer *tag
    = (struct multiboot_tag_framebuffer *) *ptrorig;

  err = holy_multiboot_set_video_mode ();
  if (err)
    {
      holy_print_error ();
      holy_errno = holy_ERR_NONE;
    }

  holy_video_get_palette (0, ARRAY_SIZE (palette), palette);

  driv_id = holy_video_get_driver_id ();
#if HAS_VGA_TEXT
  if (driv_id == holy_VIDEO_DRIVER_NONE)
    {
      struct holy_vbe_mode_info_block vbe_mode_info;
      holy_uint32_t vbe_mode;

#if defined (holy_MACHINE_PCBIOS)
      {
	holy_vbe_status_t status;
	void *scratch = (void *) holy_MEMORY_MACHINE_SCRATCH_ADDR;
	status = holy_vbe_bios_get_mode (scratch);
	vbe_mode = *(holy_uint32_t *) scratch;
	if (status != holy_VBE_STATUS_OK)
	  return holy_ERR_NONE;
      }
#else
      vbe_mode = 3;
#endif

      /* get_mode_info isn't available for mode 3.  */
      if (vbe_mode == 3)
	{
	  holy_memset (&vbe_mode_info, 0,
		       sizeof (struct holy_vbe_mode_info_block));
	  vbe_mode_info.memory_model = holy_VBE_MEMORY_MODEL_TEXT;
	  vbe_mode_info.x_resolution = 80;
	  vbe_mode_info.y_resolution = 25;
	}
#if defined (holy_MACHINE_PCBIOS)
      else
	{
	  holy_vbe_status_t status;
	  void *scratch = (void *) holy_MEMORY_MACHINE_SCRATCH_ADDR;
	  status = holy_vbe_bios_get_mode_info (vbe_mode, scratch);
	  if (status != holy_VBE_STATUS_OK)
	    return holy_ERR_NONE;
	  holy_memcpy (&vbe_mode_info, scratch,
		       sizeof (struct holy_vbe_mode_info_block));
	}
#endif

      if (vbe_mode_info.memory_model == holy_VBE_MEMORY_MODEL_TEXT)
	{
	  tag = (struct multiboot_tag_framebuffer *) *ptrorig;
	  tag->common.type = MULTIBOOT_TAG_TYPE_FRAMEBUFFER;
	  tag->common.size = 0;

	  tag->common.framebuffer_addr = 0xb8000;
	  
	  tag->common.framebuffer_pitch = 2 * vbe_mode_info.x_resolution;	
	  tag->common.framebuffer_width = vbe_mode_info.x_resolution;
	  tag->common.framebuffer_height = vbe_mode_info.y_resolution;

	  tag->common.framebuffer_bpp = 16;
	  
	  tag->common.framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT;
	  tag->common.size = sizeof (tag->common);
	  tag->common.reserved = 0;
	  *ptrorig += ALIGN_UP (tag->common.size, MULTIBOOT_TAG_ALIGN)
	    / sizeof (holy_properly_aligned_t);
	}
      return holy_ERR_NONE;
    }
#else
  if (driv_id == holy_VIDEO_DRIVER_NONE)
    return holy_ERR_NONE;
#endif

#if holy_MACHINE_HAS_VBE
  {
    struct multiboot_tag_vbe *tag_vbe = (struct multiboot_tag_vbe *) *ptrorig;

    fill_vbe_tag (tag_vbe);

    *ptrorig += ALIGN_UP (tag_vbe->size, MULTIBOOT_TAG_ALIGN)
      / sizeof (holy_properly_aligned_t);
  }
#endif

  err = holy_video_get_info_and_fini (&mode_info, &framebuffer);
  if (err)
    return err;

  tag = (struct multiboot_tag_framebuffer *) *ptrorig;
  tag->common.type = MULTIBOOT_TAG_TYPE_FRAMEBUFFER;
  tag->common.size = 0;

  tag->common.framebuffer_addr = (holy_addr_t) framebuffer;
  tag->common.framebuffer_pitch = mode_info.pitch;

  tag->common.framebuffer_width = mode_info.width;
  tag->common.framebuffer_height = mode_info.height;

  tag->common.framebuffer_bpp = mode_info.bpp;

  tag->common.reserved = 0;
      
  if (mode_info.mode_type & holy_VIDEO_MODE_TYPE_INDEX_COLOR)
    {
      unsigned i;
      tag->common.framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED;
      tag->framebuffer_palette_num_colors = mode_info.number_of_colors;
      if (tag->framebuffer_palette_num_colors > ARRAY_SIZE (palette))
	tag->framebuffer_palette_num_colors = ARRAY_SIZE (palette);
      tag->common.size = sizeof (struct multiboot_tag_framebuffer_common)
	+ sizeof (multiboot_uint16_t) + tag->framebuffer_palette_num_colors
	* sizeof (struct multiboot_color);
      for (i = 0; i < tag->framebuffer_palette_num_colors; i++)
	{
	  tag->framebuffer_palette[i].red = palette[i].r;
	  tag->framebuffer_palette[i].green = palette[i].g;
	  tag->framebuffer_palette[i].blue = palette[i].b;
	}
    }
  else
    {
      tag->common.framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_RGB;
      tag->framebuffer_red_field_position = mode_info.red_field_pos;
      tag->framebuffer_red_mask_size = mode_info.red_mask_size;
      tag->framebuffer_green_field_position = mode_info.green_field_pos;
      tag->framebuffer_green_mask_size = mode_info.green_mask_size;
      tag->framebuffer_blue_field_position = mode_info.blue_field_pos;
      tag->framebuffer_blue_mask_size = mode_info.blue_mask_size;

      tag->common.size = sizeof (struct multiboot_tag_framebuffer_common) + 6;
    }
  *ptrorig += ALIGN_UP (tag->common.size, MULTIBOOT_TAG_ALIGN)
    / sizeof (holy_properly_aligned_t);

  return holy_ERR_NONE;
}

holy_err_t
holy_multiboot_make_mbi (holy_uint32_t *target)
{
  holy_properly_aligned_t *ptrorig;
  holy_properly_aligned_t *mbistart;
  holy_err_t err;
  holy_size_t bufsize;
  holy_relocator_chunk_t ch;

  bufsize = holy_multiboot_get_mbi_size ();

  COMPILE_TIME_ASSERT (MULTIBOOT_TAG_ALIGN % sizeof (holy_properly_aligned_t) == 0);

  err = holy_relocator_alloc_chunk_align (holy_multiboot_relocator, &ch,
					  0, 0xffffffff - bufsize,
					  bufsize, MULTIBOOT_TAG_ALIGN,
					  holy_RELOCATOR_PREFERENCE_NONE, 1);
  if (err)
    return err;

  ptrorig = get_virtual_current_address (ch);
#if defined (__i386__) || defined (__x86_64__)
  *target = get_physical_target_address (ch);
#elif defined (__mips)
  *target = get_physical_target_address (ch) | 0x80000000;
#else
#error Please complete this
#endif

  mbistart = ptrorig;
  COMPILE_TIME_ASSERT ((2 * sizeof (holy_uint32_t))
		       % sizeof (holy_properly_aligned_t) == 0);
  COMPILE_TIME_ASSERT (MULTIBOOT_TAG_ALIGN
		       % sizeof (holy_properly_aligned_t) == 0);
  ptrorig += (2 * sizeof (holy_uint32_t)) / sizeof (holy_properly_aligned_t);

  {
    struct multiboot_tag_load_base_addr *tag = (struct multiboot_tag_load_base_addr *) ptrorig;
    tag->type = MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR;
    tag->size = sizeof (struct multiboot_tag_load_base_addr);
    tag->load_base_addr = load_base_addr;
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
       / sizeof (holy_properly_aligned_t);
  }

  {
    struct multiboot_tag_string *tag = (struct multiboot_tag_string *) ptrorig;
    tag->type = MULTIBOOT_TAG_TYPE_CMDLINE;
    tag->size = sizeof (struct multiboot_tag_string) + cmdline_size; 
    holy_memcpy (tag->string, cmdline, cmdline_size);
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
       / sizeof (holy_properly_aligned_t);
  }

  {
    struct multiboot_tag_string *tag = (struct multiboot_tag_string *) ptrorig;
    tag->type = MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME;
    tag->size = sizeof (struct multiboot_tag_string) + sizeof (PACKAGE_STRING); 
    holy_memcpy (tag->string, PACKAGE_STRING, sizeof (PACKAGE_STRING));
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
      / sizeof (holy_properly_aligned_t);
  }

#ifdef holy_MACHINE_PCBIOS
  {
    struct holy_apm_info info;
    if (holy_apm_get_info (&info))
      {
	struct multiboot_tag_apm *tag = (struct multiboot_tag_apm *) ptrorig;

	tag->type = MULTIBOOT_TAG_TYPE_APM;
	tag->size = sizeof (struct multiboot_tag_apm); 

	tag->cseg = info.cseg;
	tag->offset = info.offset;
	tag->cseg_16 = info.cseg_16;
	tag->dseg = info.dseg;
	tag->flags = info.flags;
	tag->cseg_len = info.cseg_len;
	tag->dseg_len = info.dseg_len;
	tag->cseg_16_len = info.cseg_16_len;
	tag->version = info.version;

	ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
	  / sizeof (holy_properly_aligned_t);
      }
  }
#endif

  {
    unsigned i;
    struct module *cur;

    for (i = 0, cur = modules; i < modcnt; i++, cur = cur->next)
      {
	struct multiboot_tag_module *tag
	  = (struct multiboot_tag_module *) ptrorig;
	tag->type = MULTIBOOT_TAG_TYPE_MODULE;
	tag->size = sizeof (struct multiboot_tag_module) + cur->cmdline_size;
	tag->mod_start = cur->start;
	tag->mod_end = tag->mod_start + cur->size;
	holy_memcpy (tag->cmdline, cur->cmdline, cur->cmdline_size);
	ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
	  / sizeof (holy_properly_aligned_t);
      }
  }

  if (!keep_bs)
    {
      struct multiboot_tag_mmap *tag = (struct multiboot_tag_mmap *) ptrorig;
      holy_fill_multiboot_mmap (tag);
      ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
	/ sizeof (holy_properly_aligned_t);
    }

  {
    struct multiboot_tag_elf_sections *tag
      = (struct multiboot_tag_elf_sections *) ptrorig;
    tag->type = MULTIBOOT_TAG_TYPE_ELF_SECTIONS;
    tag->size = sizeof (struct multiboot_tag_elf_sections)
      + elf_sec_entsize * elf_sec_num;
    holy_memcpy (tag->sections, elf_sections, elf_sec_entsize * elf_sec_num);
    tag->num = elf_sec_num;
    tag->entsize = elf_sec_entsize;
    tag->shndx = elf_sec_shstrndx;
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
      / sizeof (holy_properly_aligned_t);
  }

  if (!keep_bs)
    {
      struct multiboot_tag_basic_meminfo *tag
	= (struct multiboot_tag_basic_meminfo *) ptrorig;
      tag->type = MULTIBOOT_TAG_TYPE_BASIC_MEMINFO;
      tag->size = sizeof (struct multiboot_tag_basic_meminfo);

      /* Convert from bytes to kilobytes.  */
      tag->mem_lower = holy_mmap_get_lower () / 1024;
      tag->mem_upper = holy_mmap_get_upper () / 1024;
      ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
	/ sizeof (holy_properly_aligned_t);
    }

  {
    struct holy_net_network_level_interface *net;

    FOR_NET_NETWORK_LEVEL_INTERFACES(net)
      if (net->dhcp_ack)
	{
	  struct multiboot_tag_network *tag
	    = (struct multiboot_tag_network *) ptrorig;
	  tag->type = MULTIBOOT_TAG_TYPE_NETWORK;
	  tag->size = sizeof (struct multiboot_tag_network) + net->dhcp_acklen;
	  holy_memcpy (tag->dhcpack, net->dhcp_ack, net->dhcp_acklen);
	  ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
	    / sizeof (holy_properly_aligned_t);
	}
  }

  if (bootdev_set)
    {
      struct multiboot_tag_bootdev *tag
	= (struct multiboot_tag_bootdev *) ptrorig;
      tag->type = MULTIBOOT_TAG_TYPE_BOOTDEV;
      tag->size = sizeof (struct multiboot_tag_bootdev); 

      tag->biosdev = biosdev;
      tag->slice = slice;
      tag->part = part;
      ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
	/ sizeof (holy_properly_aligned_t);
    }

  {
    err = retrieve_video_parameters (&ptrorig);
    if (err)
      {
	holy_print_error ();
	holy_errno = holy_ERR_NONE;
      }
  }

#if defined (holy_MACHINE_EFI) && defined (__x86_64__)
  {
    struct multiboot_tag_efi64 *tag = (struct multiboot_tag_efi64 *) ptrorig;
    tag->type = MULTIBOOT_TAG_TYPE_EFI64;
    tag->size = sizeof (*tag);
    tag->pointer = (holy_addr_t) holy_efi_system_table;
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
      / sizeof (holy_properly_aligned_t);
  }
#endif

#if defined (holy_MACHINE_EFI) && defined (__i386__)
  {
    struct multiboot_tag_efi32 *tag = (struct multiboot_tag_efi32 *) ptrorig;
    tag->type = MULTIBOOT_TAG_TYPE_EFI32;
    tag->size = sizeof (*tag);
    tag->pointer = (holy_addr_t) holy_efi_system_table;
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
      / sizeof (holy_properly_aligned_t);
  }
#endif

#if holy_MACHINE_HAS_ACPI
  {
    struct multiboot_tag_old_acpi *tag = (struct multiboot_tag_old_acpi *)
      ptrorig;
    struct holy_acpi_rsdp_v10 *a = holy_acpi_get_rsdpv1 ();
    if (a)
      {
	tag->type = MULTIBOOT_TAG_TYPE_ACPI_OLD;
	tag->size = sizeof (*tag) + sizeof (*a);
	holy_memcpy (tag->rsdp, a, sizeof (*a));
	ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
	  / sizeof (holy_properly_aligned_t);
      }
  }

  {
    struct multiboot_tag_new_acpi *tag = (struct multiboot_tag_new_acpi *)
      ptrorig;
    struct holy_acpi_rsdp_v20 *a = holy_acpi_get_rsdpv2 ();
    if (a)
      {
	tag->type = MULTIBOOT_TAG_TYPE_ACPI_NEW;
	tag->size = sizeof (*tag) + a->length;
	holy_memcpy (tag->rsdp, a, a->length);
	ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
	  / sizeof (holy_properly_aligned_t);
      }
  }
#endif

#ifdef holy_MACHINE_EFI
  {
    struct multiboot_tag_efi_mmap *tag = (struct multiboot_tag_efi_mmap *) ptrorig;
    holy_efi_uintn_t efi_desc_size;
    holy_efi_uint32_t efi_desc_version;

    if (!keep_bs)
      {
	tag->type = MULTIBOOT_TAG_TYPE_EFI_MMAP;
	tag->size = sizeof (*tag) + efi_mmap_size;

	err = holy_efi_finish_boot_services (&efi_mmap_size, tag->efi_mmap, NULL,
					     &efi_desc_size, &efi_desc_version);

	if (err)
	  return err;

	tag->descr_size = efi_desc_size;
	tag->descr_vers = efi_desc_version;
	tag->size = sizeof (*tag) + efi_mmap_size;

	ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
	  / sizeof (holy_properly_aligned_t);
      }
  }

  if (keep_bs)
    {
      {
	struct multiboot_tag *tag = (struct multiboot_tag *) ptrorig;
	tag->type = MULTIBOOT_TAG_TYPE_EFI_BS;
	tag->size = sizeof (struct multiboot_tag);
	ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
	  / sizeof (holy_properly_aligned_t);
      }

#ifdef __i386__
      {
	struct multiboot_tag_efi32_ih *tag = (struct multiboot_tag_efi32_ih *) ptrorig;
	tag->type = MULTIBOOT_TAG_TYPE_EFI32_IH;
	tag->size = sizeof (struct multiboot_tag_efi32_ih);
	tag->pointer = (holy_addr_t) holy_efi_image_handle;
	ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
	  / sizeof (holy_properly_aligned_t);
      }
#endif

#ifdef __x86_64__
      {
	struct multiboot_tag_efi64_ih *tag = (struct multiboot_tag_efi64_ih *) ptrorig;
	tag->type = MULTIBOOT_TAG_TYPE_EFI64_IH;
	tag->size = sizeof (struct multiboot_tag_efi64_ih);
	tag->pointer = (holy_addr_t) holy_efi_image_handle;
	ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
	  / sizeof (holy_properly_aligned_t);
      }
#endif
    }
#endif

  {
    struct multiboot_tag *tag = (struct multiboot_tag *) ptrorig;
    tag->type = MULTIBOOT_TAG_TYPE_END;
    tag->size = sizeof (struct multiboot_tag);
    ptrorig += ALIGN_UP (tag->size, MULTIBOOT_TAG_ALIGN)
      / sizeof (holy_properly_aligned_t);
  }

  ((holy_uint32_t *) mbistart)[0] = (char *) ptrorig - (char *) mbistart;
  ((holy_uint32_t *) mbistart)[1] = 0;

  return holy_ERR_NONE;
}

void
holy_multiboot_free_mbi (void)
{
  struct module *cur, *next;

  cmdline_size = 0;
  total_modcmd = 0;
  modcnt = 0;
  holy_free (cmdline);
  cmdline = NULL;
  bootdev_set = 0;

  for (cur = modules; cur; cur = next)
    {
      next = cur->next;
      holy_free (cur->cmdline);
      holy_free (cur);
    }
  modules = NULL;
  modules_last = NULL;
}

holy_err_t
holy_multiboot_init_mbi (int argc, char *argv[])
{
  holy_ssize_t len = 0;

  holy_multiboot_free_mbi ();

  len = holy_loader_cmdline_size (argc, argv);

  cmdline = holy_malloc (len);
  if (! cmdline)
    return holy_errno;
  cmdline_size = len;

  holy_create_loader_cmdline (argc, argv, cmdline,
			      cmdline_size);

  return holy_ERR_NONE;
}

holy_err_t
holy_multiboot_add_module (holy_addr_t start, holy_size_t size,
			   int argc, char *argv[])
{
  struct module *newmod;
  holy_size_t len = 0;

  newmod = holy_malloc (sizeof (*newmod));
  if (!newmod)
    return holy_errno;
  newmod->start = start;
  newmod->size = size;

  len = holy_loader_cmdline_size (argc, argv);

  newmod->cmdline = holy_malloc (len);
  if (! newmod->cmdline)
    {
      holy_free (newmod);
      return holy_errno;
    }
  newmod->cmdline_size = len;
  total_modcmd += ALIGN_UP (len, MULTIBOOT_TAG_ALIGN);

  holy_create_loader_cmdline (argc, argv, newmod->cmdline,
			      newmod->cmdline_size);

  if (modules_last)
    modules_last->next = newmod;
  else
    modules = newmod;
  modules_last = newmod;

  modcnt++;

  return holy_ERR_NONE;
}

void
holy_multiboot_set_bootdev (void)
{
  holy_device_t dev;

  slice = ~0;
  part = ~0;

#ifdef holy_MACHINE_PCBIOS
  biosdev = holy_get_root_biosnumber ();
#else
  biosdev = 0xffffffff;
#endif

  if (biosdev == 0xffffffff)
    return;

  dev = holy_device_open (0);
  if (dev && dev->disk && dev->disk->partition)
    {
      if (dev->disk->partition->parent)
 	{
	  part = dev->disk->partition->number;
	  slice = dev->disk->partition->parent->number;
	}
      else
	slice = dev->disk->partition->number;
    }
  if (dev)
    holy_device_close (dev);

  bootdev_set = 1;
}
