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
#include <holy/cpu/relocator.h>
#include <holy/disk.h>
#include <holy/device.h>
#include <holy/partition.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/env.h>
#include <holy/relocator.h>
#include <holy/video.h>
#include <holy/file.h>
#include <holy/net.h>
#include <holy/i18n.h>
#include <holy/lib/cmdline.h>
#include <holy/tpm.h>

#ifdef holy_MACHINE_EFI
#include <holy/efi/efi.h>
#endif

/* The bits in the required part of flags field we don't support.  */
#define UNSUPPORTED_FLAGS			0x0000fff8

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
static holy_uint32_t bootdev;
static int bootdev_set;
static holy_size_t elf_sec_num, elf_sec_entsize;
static unsigned elf_sec_shstrndx;
static void *elf_sections;
holy_multiboot_quirks_t holy_multiboot_quirks;

static holy_err_t
load_kernel (holy_file_t file, const char *filename,
	     char *buffer, struct multiboot_header *header)
{
  holy_err_t err;
  mbi_load_data_t mld;

  mld.file = file;
  mld.filename = filename;
  mld.buffer = buffer;
  mld.mbi_ver = 1;
  mld.relocatable = 0;
  mld.avoid_efi_boot_services = 0;

  if (holy_multiboot_quirks & holy_MULTIBOOT_QUIRK_BAD_KLUDGE)
    {
      err = holy_multiboot_load_elf (&mld);
      if (err == holy_ERR_NONE) {
	return holy_ERR_NONE;
      }
      if (err == holy_ERR_UNKNOWN_OS && (header->flags & MULTIBOOT_AOUT_KLUDGE))
	holy_errno = err = holy_ERR_NONE;
    }
  if (header->flags & MULTIBOOT_AOUT_KLUDGE)
    {
      int offset = ((char *) header - buffer -
		    (header->header_addr - header->load_addr));
      int load_size = ((header->load_end_addr == 0) ? file->size - offset :
		       header->load_end_addr - header->load_addr);
      holy_size_t code_size;
      void *source;
      holy_relocator_chunk_t ch;

      if (header->bss_end_addr)
	code_size = (header->bss_end_addr - header->load_addr);
      else
	code_size = load_size;

      err = holy_relocator_alloc_chunk_addr (holy_multiboot_relocator,
					     &ch, header->load_addr,
					     code_size);
      if (err)
	{
	  holy_dprintf ("multiboot_loader", "Error loading aout kludge\n");
	  return err;
	}
      source = get_virtual_current_address (ch);

      if ((holy_file_seek (file, offset)) == (holy_off_t) -1)
	{
	  return holy_errno;
	}

      holy_file_read (file, source, load_size);
      if (holy_errno)
	return holy_errno;

      if (header->bss_end_addr)
	holy_memset ((holy_uint8_t *) source + load_size, 0,
		     header->bss_end_addr - header->load_addr - load_size);

      holy_multiboot_payload_eip = header->entry_addr;
      return holy_ERR_NONE;
    }

  return holy_multiboot_load_elf (&mld);
}

static struct multiboot_header *
find_header (char *buffer, holy_ssize_t len)
{
  struct multiboot_header *header;

  /* Look for the multiboot header in the buffer.  The header should
     be at least 12 bytes and aligned on a 4-byte boundary.  */
  for (header = (struct multiboot_header *) buffer;
       ((char *) header <= buffer + len - 12);
       header = (struct multiboot_header *) ((char *) header + MULTIBOOT_HEADER_ALIGN))
    {
      if (header->magic == MULTIBOOT_HEADER_MAGIC
	  && !(header->magic + header->flags + header->checksum))
	return header;
    }
  return NULL;
}

holy_err_t
holy_multiboot_load (holy_file_t file, const char *filename)
{
  char *buffer;
  holy_ssize_t len;
  struct multiboot_header *header;
  holy_err_t err;

  buffer = holy_malloc (MULTIBOOT_SEARCH);
  if (!buffer)
    return holy_errno;

  len = holy_file_read (file, buffer, MULTIBOOT_SEARCH);
  if (len < 32)
    {
      holy_free (buffer);
      if (!holy_errno)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
		    filename);
      return holy_errno;
    }

  holy_tpm_measure((unsigned char*)buffer, len, holy_BINARY_PCR, "holy_multiboot", filename);
  holy_print_error();

  header = find_header (buffer, len);

  if (header == 0)
    {
      holy_free (buffer);
      return holy_error (holy_ERR_BAD_ARGUMENT, "no multiboot header found");
    }

  if (header->flags & UNSUPPORTED_FLAGS)
    {
      holy_free (buffer);
      return holy_error (holy_ERR_UNKNOWN_OS,
			 "unsupported flag: 0x%x", header->flags);
    }

  err = load_kernel (file, filename, buffer, header);
  if (err)
    {
      holy_free (buffer);
      return err;
    }

  if (header->flags & MULTIBOOT_VIDEO_MODE)
    {
      switch (header->mode_type)
	{
	case 1:
	  err = holy_multiboot_set_console (holy_MULTIBOOT_CONSOLE_EGA_TEXT,
					    holy_MULTIBOOT_CONSOLE_EGA_TEXT
					    | holy_MULTIBOOT_CONSOLE_FRAMEBUFFER,
					    0, 0, 0, 0);
	  break;
	case 0:
	  err = holy_multiboot_set_console (holy_MULTIBOOT_CONSOLE_FRAMEBUFFER,
					    holy_MULTIBOOT_CONSOLE_EGA_TEXT
					    | holy_MULTIBOOT_CONSOLE_FRAMEBUFFER,
					    header->width, header->height,
					    header->depth, 0);
	  break;
	default:
	  err = holy_error (holy_ERR_BAD_OS,
			    "unsupported graphical mode type %d",
			    header->mode_type);
	  break;
	}
    }
  else
    err = holy_multiboot_set_console (holy_MULTIBOOT_CONSOLE_EGA_TEXT,
				      holy_MULTIBOOT_CONSOLE_EGA_TEXT,
				      0, 0, 0, 0);
  return err;
}

#if holy_MACHINE_HAS_VBE || holy_MACHINE_HAS_VGA_TEXT
#include <holy/i386/pc/vbe.h>
#endif

static holy_size_t
holy_multiboot_get_mbi_size (void)
{
  holy_size_t ret;
  struct holy_net_network_level_interface *net;

  ret = sizeof (struct multiboot_info) + ALIGN_UP (cmdline_size, 4)
    + modcnt * sizeof (struct multiboot_mod_list) + total_modcmd
    + ALIGN_UP (sizeof(PACKAGE_STRING), 4) 
    + holy_get_multiboot_mmap_count () * sizeof (struct multiboot_mmap_entry)
    + elf_sec_entsize * elf_sec_num
    + 256 * sizeof (struct multiboot_color)
#if holy_MACHINE_HAS_VBE || holy_MACHINE_HAS_VGA_TEXT
    + sizeof (struct holy_vbe_info_block)
    + sizeof (struct holy_vbe_mode_info_block)
#endif
    + ALIGN_UP (sizeof (struct multiboot_apm_info), 4);

  FOR_NET_NETWORK_LEVEL_INTERFACES(net)
    if (net->dhcp_ack)
      {
	ret += net->dhcp_acklen;
	break;
      }

  return ret;
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
  (*mmap_entry)->size = sizeof (struct multiboot_mmap_entry) - sizeof ((*mmap_entry)->size);
  (*mmap_entry)++;

  return 0;
}

/* Fill previously allocated Multiboot mmap.  */
static void
holy_fill_multiboot_mmap (struct multiboot_mmap_entry *first_entry)
{
  struct multiboot_mmap_entry *mmap_entry = (struct multiboot_mmap_entry *) first_entry;

  holy_mmap_iterate (holy_fill_multiboot_mmap_iter, &mmap_entry);
}

#if holy_MACHINE_HAS_VBE || holy_MACHINE_HAS_VGA_TEXT

static holy_err_t
fill_vbe_info (struct multiboot_info *mbi, holy_uint8_t *ptrorig,
	       holy_uint32_t ptrdest, int fill_generic)
{
  holy_uint32_t vbe_mode;
  struct holy_vbe_mode_info_block *mode_info;
#if holy_MACHINE_HAS_VBE
  holy_vbe_status_t status;
  void *scratch = (void *) holy_MEMORY_MACHINE_SCRATCH_ADDR;
    
  status = holy_vbe_bios_get_controller_info (scratch);
  if (status != holy_VBE_STATUS_OK)
    return holy_error (holy_ERR_IO, "Can't get controller info.");
  
  mbi->vbe_control_info = ptrdest;
  holy_memcpy (ptrorig, scratch, sizeof (struct holy_vbe_info_block));
  ptrorig += sizeof (struct holy_vbe_info_block);
  ptrdest += sizeof (struct holy_vbe_info_block);
#else
  mbi->vbe_control_info = 0;
#endif

#if holy_MACHINE_HAS_VBE
  status = holy_vbe_bios_get_mode (scratch);
  vbe_mode = *(holy_uint32_t *) scratch;
  if (status != holy_VBE_STATUS_OK)
    return holy_error (holy_ERR_IO, "can't get VBE mode");
#else
  vbe_mode = 3;
#endif
  mbi->vbe_mode = vbe_mode;

  mode_info = (struct holy_vbe_mode_info_block *) ptrorig;
  mbi->vbe_mode_info = ptrdest;
  /* get_mode_info isn't available for mode 3.  */
  if (vbe_mode == 3)
    {
      holy_memset (mode_info, 0, sizeof (struct holy_vbe_mode_info_block));
      mode_info->memory_model = holy_VBE_MEMORY_MODEL_TEXT;
      mode_info->x_resolution = 80;
      mode_info->y_resolution = 25;
    }
  else
    {
#if holy_MACHINE_HAS_VBE
      status = holy_vbe_bios_get_mode_info (vbe_mode, scratch);
      if (status != holy_VBE_STATUS_OK)
	return holy_error (holy_ERR_IO, "can't get mode info");
      holy_memcpy (mode_info, scratch,
		   sizeof (struct holy_vbe_mode_info_block));
#endif
    }
  ptrorig += sizeof (struct holy_vbe_mode_info_block);
  ptrdest += sizeof (struct holy_vbe_mode_info_block);

#if holy_MACHINE_HAS_VBE
  holy_vbe_bios_get_pm_interface (&mbi->vbe_interface_seg,
				  &mbi->vbe_interface_off,
				  &mbi->vbe_interface_len);
#endif
  
  mbi->flags |= MULTIBOOT_INFO_VBE_INFO;

  if (fill_generic && mode_info->memory_model == holy_VBE_MEMORY_MODEL_TEXT)
    {
      mbi->framebuffer_addr = 0xb8000;

      mbi->framebuffer_pitch = 2 * mode_info->x_resolution;	
      mbi->framebuffer_width = mode_info->x_resolution;
      mbi->framebuffer_height = mode_info->y_resolution;

      mbi->framebuffer_bpp = 16;

      mbi->framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT;

      mbi->flags |= MULTIBOOT_INFO_FRAMEBUFFER_INFO;
    }

  return holy_ERR_NONE;
}
#endif

static holy_err_t
retrieve_video_parameters (struct multiboot_info *mbi,
			   holy_uint8_t *ptrorig, holy_uint32_t ptrdest)
{
  holy_err_t err;
  struct holy_video_mode_info mode_info;
  void *framebuffer;
  holy_video_driver_id_t driv_id;
  struct holy_video_palette_data palette[256];

  err = holy_multiboot_set_video_mode ();
  if (err)
    {
      holy_print_error ();
      holy_errno = holy_ERR_NONE;
    }

  holy_video_get_palette (0, ARRAY_SIZE (palette), palette);

  driv_id = holy_video_get_driver_id ();
#if holy_MACHINE_HAS_VGA_TEXT
  if (driv_id == holy_VIDEO_DRIVER_NONE)
    return fill_vbe_info (mbi, ptrorig, ptrdest, 1);
#else
  if (driv_id == holy_VIDEO_DRIVER_NONE)
    return holy_ERR_NONE;
#endif

  err = holy_video_get_info_and_fini (&mode_info, &framebuffer);
  if (err)
    return err;

  mbi->framebuffer_addr = (holy_addr_t) framebuffer;
  mbi->framebuffer_pitch = mode_info.pitch;

  mbi->framebuffer_width = mode_info.width;
  mbi->framebuffer_height = mode_info.height;

  mbi->framebuffer_bpp = mode_info.bpp;
      
  if (mode_info.mode_type & holy_VIDEO_MODE_TYPE_INDEX_COLOR)
    {
      struct multiboot_color *mb_palette;
      unsigned i;
      mbi->framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED;
      mbi->framebuffer_palette_addr = ptrdest;
      mbi->framebuffer_palette_num_colors = mode_info.number_of_colors;
      if (mbi->framebuffer_palette_num_colors > ARRAY_SIZE (palette))
	mbi->framebuffer_palette_num_colors = ARRAY_SIZE (palette);
      mb_palette = (struct multiboot_color *) ptrorig;
      for (i = 0; i < mbi->framebuffer_palette_num_colors; i++)
	{
	  mb_palette[i].red = palette[i].r;
	  mb_palette[i].green = palette[i].g;
	  mb_palette[i].blue = palette[i].b;
	}
      ptrorig += mbi->framebuffer_palette_num_colors
	* sizeof (struct multiboot_color);
      ptrdest += mbi->framebuffer_palette_num_colors
	* sizeof (struct multiboot_color);
    }
  else
    {
      mbi->framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_RGB;
      mbi->framebuffer_red_field_position = mode_info.red_field_pos;
      mbi->framebuffer_red_mask_size = mode_info.red_mask_size;
      mbi->framebuffer_green_field_position = mode_info.green_field_pos;
      mbi->framebuffer_green_mask_size = mode_info.green_mask_size;
      mbi->framebuffer_blue_field_position = mode_info.blue_field_pos;
      mbi->framebuffer_blue_mask_size = mode_info.blue_mask_size;
    }

  mbi->flags |= MULTIBOOT_INFO_FRAMEBUFFER_INFO;

#if holy_MACHINE_HAS_VBE
  if (driv_id == holy_VIDEO_DRIVER_VBE)
    return fill_vbe_info (mbi, ptrorig, ptrdest, 0);
#endif

  return holy_ERR_NONE;
}

holy_err_t
holy_multiboot_make_mbi (holy_uint32_t *target)
{
  struct multiboot_info *mbi;
  struct multiboot_mod_list *modlist;
  unsigned i;
  struct module *cur;
  holy_size_t mmap_size;
  holy_uint8_t *ptrorig;
  holy_addr_t ptrdest;

  holy_err_t err;
  holy_size_t bufsize;
  holy_relocator_chunk_t ch;

  bufsize = holy_multiboot_get_mbi_size ();

  err = holy_relocator_alloc_chunk_align (holy_multiboot_relocator, &ch,
					  0x10000, 0xa0000 - bufsize,
					  bufsize, 4,
					  holy_RELOCATOR_PREFERENCE_NONE, 0);
  if (err)
    return err;
  ptrorig = get_virtual_current_address (ch);
  ptrdest = get_physical_target_address (ch);

  *target = ptrdest;

  mbi = (struct multiboot_info *) ptrorig;
  ptrorig += sizeof (*mbi);
  ptrdest += sizeof (*mbi);
  holy_memset (mbi, 0, sizeof (*mbi));

  holy_memcpy (ptrorig, cmdline, cmdline_size);
  mbi->flags |= MULTIBOOT_INFO_CMDLINE;
  mbi->cmdline = ptrdest;
  ptrorig += ALIGN_UP (cmdline_size, 4);
  ptrdest += ALIGN_UP (cmdline_size, 4);

  holy_memcpy (ptrorig, PACKAGE_STRING, sizeof(PACKAGE_STRING));
  mbi->flags |= MULTIBOOT_INFO_BOOT_LOADER_NAME;
  mbi->boot_loader_name = ptrdest;
  ptrorig += ALIGN_UP (sizeof(PACKAGE_STRING), 4);
  ptrdest += ALIGN_UP (sizeof(PACKAGE_STRING), 4);

#ifdef holy_MACHINE_PCBIOS
  {
    struct holy_apm_info info;
    if (holy_apm_get_info (&info))
      {
	struct multiboot_apm_info *mbinfo = (void *) ptrorig;

	mbinfo->cseg = info.cseg;
	mbinfo->offset = info.offset;
	mbinfo->cseg_16 = info.cseg_16;
	mbinfo->dseg = info.dseg;
	mbinfo->flags = info.flags;
	mbinfo->cseg_len = info.cseg_len;
	mbinfo->dseg_len = info.dseg_len;
	mbinfo->cseg_16_len = info.cseg_16_len;
	mbinfo->version = info.version;

	ptrorig += ALIGN_UP (sizeof (struct multiboot_apm_info), 4);
	ptrdest += ALIGN_UP (sizeof (struct multiboot_apm_info), 4);
      }
  }
#endif

  if (modcnt)
    {
      mbi->flags |= MULTIBOOT_INFO_MODS;
      mbi->mods_addr = ptrdest;
      mbi->mods_count = modcnt;
      modlist = (struct multiboot_mod_list *) ptrorig;
      ptrorig += modcnt * sizeof (struct multiboot_mod_list);
      ptrdest += modcnt * sizeof (struct multiboot_mod_list);

      for (i = 0, cur = modules; i < modcnt; i++, cur = cur->next)
	{
	  modlist[i].mod_start = cur->start;
	  modlist[i].mod_end = modlist[i].mod_start + cur->size;
	  modlist[i].cmdline = ptrdest;
	  holy_memcpy (ptrorig, cur->cmdline, cur->cmdline_size);
	  ptrorig += ALIGN_UP (cur->cmdline_size, 4);
	  ptrdest += ALIGN_UP (cur->cmdline_size, 4);
	}
    }
  else
    {
      mbi->mods_addr = 0;
      mbi->mods_count = 0;
    }

  mmap_size = holy_get_multiboot_mmap_count ()
    * sizeof (struct multiboot_mmap_entry);
  holy_fill_multiboot_mmap ((struct multiboot_mmap_entry *) ptrorig);
  mbi->mmap_length = mmap_size;
  mbi->mmap_addr = ptrdest;
  mbi->flags |= MULTIBOOT_INFO_MEM_MAP;
  ptrorig += mmap_size;
  ptrdest += mmap_size;

  /* Convert from bytes to kilobytes.  */
  mbi->mem_lower = holy_mmap_get_lower () / 1024;
  mbi->mem_upper = holy_mmap_get_upper () / 1024;
  mbi->flags |= MULTIBOOT_INFO_MEMORY;

  if (bootdev_set)
    {
      mbi->boot_device = bootdev;
      mbi->flags |= MULTIBOOT_INFO_BOOTDEV;
    }

  {
    struct holy_net_network_level_interface *net;
    FOR_NET_NETWORK_LEVEL_INTERFACES(net)
      if (net->dhcp_ack)
	{
	  holy_memcpy (ptrorig, net->dhcp_ack, net->dhcp_acklen);
	  mbi->drives_addr = ptrdest;
	  mbi->drives_length = net->dhcp_acklen;
	  ptrorig += net->dhcp_acklen;
	  ptrdest += net->dhcp_acklen;
	  break;
	}
  }

  if (elf_sec_num)
    {
      mbi->u.elf_sec.addr = ptrdest;
      holy_memcpy (ptrorig, elf_sections, elf_sec_entsize * elf_sec_num);
      mbi->u.elf_sec.num = elf_sec_num;
      mbi->u.elf_sec.size = elf_sec_entsize;
      mbi->u.elf_sec.shndx = elf_sec_shstrndx;

      mbi->flags |= MULTIBOOT_INFO_ELF_SHDR;

      ptrorig += elf_sec_entsize * elf_sec_num;
      ptrdest += elf_sec_entsize * elf_sec_num;
    }

  err = retrieve_video_parameters (mbi, ptrorig, ptrdest);
  if (err)
    {
      holy_print_error ();
      holy_errno = holy_ERR_NONE;
    }

  if ((mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO)
      && mbi->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED)
    {
      ptrorig += mbi->framebuffer_palette_num_colors
	* sizeof (struct multiboot_color);
      ptrdest += mbi->framebuffer_palette_num_colors
	* sizeof (struct multiboot_color);
    }

#if holy_MACHINE_HAS_VBE
  ptrorig += sizeof (struct holy_vbe_info_block);
  ptrdest += sizeof (struct holy_vbe_info_block);
  ptrorig += sizeof (struct holy_vbe_mode_info_block);
  ptrdest += sizeof (struct holy_vbe_mode_info_block);
#endif

#ifdef holy_MACHINE_EFI
  err = holy_efi_finish_boot_services (NULL, NULL, NULL, NULL, NULL);
  if (err)
    return err;
#endif

  return holy_ERR_NONE;
}

void
holy_multiboot_add_elfsyms (holy_size_t num, holy_size_t entsize,
			    unsigned shndx, void *data)
{
  elf_sec_num = num;
  elf_sec_shstrndx = shndx;
  elf_sec_entsize = entsize;
  elf_sections = data;
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

  holy_free (elf_sections);
  elf_sections = NULL;
  elf_sec_entsize = 0;
  elf_sec_num = 0;
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
  newmod->next = 0;

  len = holy_loader_cmdline_size (argc, argv);

  newmod->cmdline = holy_malloc (len);
  if (! newmod->cmdline)
    {
      holy_free (newmod);
      return holy_errno;
    }
  newmod->cmdline_size = len;
  total_modcmd += ALIGN_UP (len, 4);

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
  holy_uint32_t biosdev, slice = ~0, part = ~0;
  holy_device_t dev;

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

  bootdev = ((biosdev & 0xff) << 24) | ((slice & 0xff) << 16) 
    | ((part & 0xff) << 8) | 0xff;
  bootdev_set = 1;
}
