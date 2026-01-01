/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_LINUX_MACHINE_HEADER
#define holy_LINUX_MACHINE_HEADER	1

#define holy_LINUX_MAGIC_SIGNATURE	0x53726448      /* "HdrS" */
#define holy_LINUX_DEFAULT_SETUP_SECTS	4
#define holy_LINUX_INITRD_MAX_ADDRESS	0x37FFFFFF
#define holy_LINUX_MAX_SETUP_SECTS	64
#define holy_LINUX_BOOT_LOADER_TYPE	0x72
#define holy_LINUX_HEAP_END_OFFSET	(0x9000 - 0x200)

#define holy_LINUX_BZIMAGE_ADDR		0x100000
#define holy_LINUX_ZIMAGE_ADDR		0x10000
#define holy_LINUX_OLD_REAL_MODE_ADDR	0x90000
#define holy_LINUX_SETUP_STACK		0x9000

#define holy_LINUX_FLAG_BIG_KERNEL	0x1
#define holy_LINUX_FLAG_QUIET		0x20
#define holy_LINUX_FLAG_CAN_USE_HEAP	0x80

/* Linux's video mode selection support. Actually I hate it!  */
#define holy_LINUX_VID_MODE_NORMAL	0xFFFF
#define holy_LINUX_VID_MODE_EXTENDED	0xFFFE
#define holy_LINUX_VID_MODE_ASK		0xFFFD
#define holy_LINUX_VID_MODE_VESA_START	0x0300

#define holy_LINUX_CL_MAGIC		0xA33F

#ifdef __x86_64__

#define holy_LINUX_EFI_SIGNATURE	\
  ('4' << 24 | '6' << 16 | 'L' << 8 | 'E')

#else

#define holy_LINUX_EFI_SIGNATURE	\
  ('2' << 24 | '3' << 16 | 'L' << 8 | 'E')

#endif

#define holy_LINUX_EFI_SIGNATURE_0204	\
  ('L' << 24 | 'I' << 16 | 'F' << 8 | 'E')

#define holy_LINUX_OFW_SIGNATURE	\
  (' ' << 24 | 'W' << 16 | 'F' << 8 | 'O')

#ifndef ASM_FILE

#define holy_E820_RAM        1
#define holy_E820_RESERVED   2
#define holy_E820_ACPI       3
#define holy_E820_NVS        4
#define holy_E820_BADRAM     5

struct holy_e820_mmap
{
  holy_uint64_t addr;
  holy_uint64_t size;
  holy_uint32_t type;
} holy_PACKED;

enum
  {
    holy_VIDEO_LINUX_TYPE_TEXT = 0x01,
    holy_VIDEO_LINUX_TYPE_VESA = 0x23,    /* VESA VGA in graphic mode.  */
    holy_VIDEO_LINUX_TYPE_EFIFB = 0x70,    /* EFI Framebuffer.  */
    holy_VIDEO_LINUX_TYPE_SIMPLE = 0x70    /* Linear framebuffer without any additional functions.  */
  };

/* For the Linux/i386 boot protocol version 2.10.  */
struct linux_kernel_header
{
  holy_uint8_t code1[0x0020];
  holy_uint16_t cl_magic;		/* Magic number 0xA33F */
  holy_uint16_t cl_offset;		/* The offset of command line */
  holy_uint8_t code2[0x01F1 - 0x0020 - 2 - 2];
  holy_uint8_t setup_sects;		/* The size of the setup in sectors */
  holy_uint16_t root_flags;		/* If the root is mounted readonly */
  holy_uint16_t syssize;		/* obsolete */
  holy_uint16_t swap_dev;		/* obsolete */
  holy_uint16_t ram_size;		/* obsolete */
  holy_uint16_t vid_mode;		/* Video mode control */
  holy_uint16_t root_dev;		/* Default root device number */
  holy_uint16_t boot_flag;		/* 0xAA55 magic number */
  holy_uint16_t jump;			/* Jump instruction */
  holy_uint32_t header;			/* Magic signature "HdrS" */
  holy_uint16_t version;		/* Boot protocol version supported */
  holy_uint32_t realmode_swtch;		/* Boot loader hook */
  holy_uint16_t start_sys;		/* The load-low segment (obsolete) */
  holy_uint16_t kernel_version;		/* Points to kernel version string */
  holy_uint8_t type_of_loader;		/* Boot loader identifier */
#define LINUX_LOADER_ID_LILO		0x0
#define LINUX_LOADER_ID_LOADLIN		0x1
#define LINUX_LOADER_ID_BOOTSECT	0x2
#define LINUX_LOADER_ID_SYSLINUX	0x3
#define LINUX_LOADER_ID_ETHERBOOT	0x4
#define LINUX_LOADER_ID_ELILO		0x5
#define LINUX_LOADER_ID_holy		0x7
#define LINUX_LOADER_ID_UBOOT		0x8
#define LINUX_LOADER_ID_XEN		0x9
#define LINUX_LOADER_ID_GUJIN		0xa
#define LINUX_LOADER_ID_QEMU		0xb
  holy_uint8_t loadflags;		/* Boot protocol option flags */
  holy_uint16_t setup_move_size;	/* Move to high memory size */
  holy_uint32_t code32_start;		/* Boot loader hook */
  holy_uint32_t ramdisk_image;		/* initrd load address */
  holy_uint32_t ramdisk_size;		/* initrd size */
  holy_uint32_t bootsect_kludge;	/* obsolete */
  holy_uint16_t heap_end_ptr;		/* Free memory after setup end */
  holy_uint16_t pad1;			/* Unused */
  holy_uint32_t cmd_line_ptr;		/* Points to the kernel command line */
  holy_uint32_t initrd_addr_max;        /* Highest address for initrd */
  holy_uint32_t kernel_alignment;
  holy_uint8_t relocatable;
  holy_uint8_t min_alignment;
  holy_uint8_t pad[2];
  holy_uint32_t cmdline_size;
  holy_uint32_t hardware_subarch;
  holy_uint64_t hardware_subarch_data;
  holy_uint32_t payload_offset;
  holy_uint32_t payload_length;
  holy_uint64_t setup_data;
  holy_uint64_t pref_address;
  holy_uint32_t init_size;
  holy_uint32_t handover_offset;
} holy_PACKED;

/* Boot parameters for Linux based on 2.6.12. This is used by the setup
   sectors of Linux, and must be simulated by holy on EFI, because
   the setup sectors depend on BIOS.  */
struct linux_kernel_params
{
  holy_uint8_t video_cursor_x;		/* 0 */
  holy_uint8_t video_cursor_y;

  holy_uint16_t ext_mem;		/* 2 */

  holy_uint16_t video_page;		/* 4 */
  holy_uint8_t video_mode;		/* 6 */
  holy_uint8_t video_width;		/* 7 */

  holy_uint8_t padding1[0xa - 0x8];

  holy_uint16_t video_ega_bx;		/* a */

  holy_uint8_t padding2[0xe - 0xc];

  holy_uint8_t video_height;		/* e */
  holy_uint8_t have_vga;		/* f */
  holy_uint16_t font_size;		/* 10 */

  holy_uint16_t lfb_width;		/* 12 */
  holy_uint16_t lfb_height;		/* 14 */
  holy_uint16_t lfb_depth;		/* 16 */
  holy_uint32_t lfb_base;		/* 18 */
  holy_uint32_t lfb_size;		/* 1c */

  holy_uint16_t cl_magic;		/* 20 */
  holy_uint16_t cl_offset;

  holy_uint16_t lfb_line_len;		/* 24 */
  holy_uint8_t red_mask_size;		/* 26 */
  holy_uint8_t red_field_pos;
  holy_uint8_t green_mask_size;
  holy_uint8_t green_field_pos;
  holy_uint8_t blue_mask_size;
  holy_uint8_t blue_field_pos;
  holy_uint8_t reserved_mask_size;
  holy_uint8_t reserved_field_pos;
  holy_uint16_t vesapm_segment;		/* 2e */
  holy_uint16_t vesapm_offset;		/* 30 */
  holy_uint16_t lfb_pages;		/* 32 */
  holy_uint16_t vesa_attrib;		/* 34 */
  holy_uint32_t capabilities;		/* 36 */

  holy_uint8_t padding3[0x40 - 0x3a];

  holy_uint16_t apm_version;		/* 40 */
  holy_uint16_t apm_code_segment;	/* 42 */
  holy_uint32_t apm_entry;		/* 44 */
  holy_uint16_t apm_16bit_code_segment;	/* 48 */
  holy_uint16_t apm_data_segment;	/* 4a */
  holy_uint16_t apm_flags;		/* 4c */
  holy_uint32_t apm_code_len;		/* 4e */
  holy_uint16_t apm_data_len;		/* 52 */

  holy_uint8_t padding4[0x60 - 0x54];

  holy_uint32_t ist_signature;		/* 60 */
  holy_uint32_t ist_command;		/* 64 */
  holy_uint32_t ist_event;		/* 68 */
  holy_uint32_t ist_perf_level;		/* 6c */

  holy_uint8_t padding5[0x80 - 0x70];

  holy_uint8_t hd0_drive_info[0x10];	/* 80 */
  holy_uint8_t hd1_drive_info[0x10];	/* 90 */
  holy_uint16_t rom_config_len;		/* a0 */

  holy_uint8_t padding6[0xb0 - 0xa2];

  holy_uint32_t ofw_signature;		/* b0 */
  holy_uint32_t ofw_num_items;		/* b4 */
  holy_uint32_t ofw_cif_handler;	/* b8 */
  holy_uint32_t ofw_idt;		/* bc */

  holy_uint8_t padding7[0x1b8 - 0xc0];

  union
    {
      struct
        {
          holy_uint32_t efi_system_table;	/* 1b8 */
          holy_uint32_t padding7_1;		/* 1bc */
          holy_uint32_t efi_signature;		/* 1c0 */
          holy_uint32_t efi_mem_desc_size;	/* 1c4 */
          holy_uint32_t efi_mem_desc_version;	/* 1c8 */
          holy_uint32_t efi_mmap_size;		/* 1cc */
          holy_uint32_t efi_mmap;		/* 1d0 */
        } v0204;
      struct
        {
          holy_uint32_t padding7_1;		/* 1b8 */
          holy_uint32_t padding7_2;		/* 1bc */
          holy_uint32_t efi_signature;		/* 1c0 */
          holy_uint32_t efi_system_table;	/* 1c4 */
          holy_uint32_t efi_mem_desc_size;	/* 1c8 */
          holy_uint32_t efi_mem_desc_version;	/* 1cc */
          holy_uint32_t efi_mmap;		/* 1d0 */
          holy_uint32_t efi_mmap_size;		/* 1d4 */
	} v0206;
      struct
        {
          holy_uint32_t padding7_1;		/* 1b8 */
          holy_uint32_t padding7_2;		/* 1bc */
          holy_uint32_t efi_signature;		/* 1c0 */
          holy_uint32_t efi_system_table;	/* 1c4 */
          holy_uint32_t efi_mem_desc_size;	/* 1c8 */
          holy_uint32_t efi_mem_desc_version;	/* 1cc */
          holy_uint32_t efi_mmap;		/* 1d0 */
          holy_uint32_t efi_mmap_size;		/* 1d4 */
          holy_uint32_t efi_system_table_hi;	/* 1d8 */
          holy_uint32_t efi_mmap_hi;		/* 1dc */
        } v0208;
    };

  holy_uint32_t alt_mem;		/* 1e0 */

  holy_uint8_t padding8[0x1e8 - 0x1e4];

  holy_uint8_t mmap_size;		/* 1e8 */

  holy_uint8_t padding9[0x1f1 - 0x1e9];

  holy_uint8_t setup_sects;		/* The size of the setup in sectors */
  holy_uint16_t root_flags;		/* If the root is mounted readonly */
  holy_uint16_t syssize;		/* obsolete */
  holy_uint16_t swap_dev;		/* obsolete */
  holy_uint16_t ram_size;		/* obsolete */
  holy_uint16_t vid_mode;		/* Video mode control */
  holy_uint16_t root_dev;		/* Default root device number */

  holy_uint8_t padding10;		/* 1fe */
  holy_uint8_t ps_mouse;		/* 1ff */

  holy_uint16_t jump;			/* Jump instruction */
  holy_uint32_t header;			/* Magic signature "HdrS" */
  holy_uint16_t version;		/* Boot protocol version supported */
  holy_uint32_t realmode_swtch;		/* Boot loader hook */
  holy_uint16_t start_sys;		/* The load-low segment (obsolete) */
  holy_uint16_t kernel_version;		/* Points to kernel version string */
  holy_uint8_t type_of_loader;		/* Boot loader identifier */
  holy_uint8_t loadflags;		/* Boot protocol option flags */
  holy_uint16_t setup_move_size;	/* Move to high memory size */
  holy_uint32_t code32_start;		/* Boot loader hook */
  holy_uint32_t ramdisk_image;		/* initrd load address */
  holy_uint32_t ramdisk_size;		/* initrd size */
  holy_uint32_t bootsect_kludge;	/* obsolete */
  holy_uint16_t heap_end_ptr;		/* Free memory after setup end */
  holy_uint8_t ext_loader_ver;		/* Extended loader version */
  holy_uint8_t ext_loader_type;		/* Extended loader type */
  holy_uint32_t cmd_line_ptr;		/* Points to the kernel command line */
  holy_uint32_t initrd_addr_max;	/* Maximum initrd address */
  holy_uint32_t kernel_alignment;	/* Alignment of the kernel */
  holy_uint8_t relocatable_kernel;	/* Is the kernel relocatable */
  holy_uint8_t pad1[3];
  holy_uint32_t cmdline_size;		/* Size of the kernel command line */
  holy_uint32_t hardware_subarch;
  holy_uint64_t hardware_subarch_data;
  holy_uint32_t payload_offset;
  holy_uint32_t payload_length;
  holy_uint64_t setup_data;
  holy_uint8_t pad2[120];		/* 258 */
  struct holy_e820_mmap e820_map[(0x400 - 0x2d0) / 20];	/* 2d0 */

} holy_PACKED;
#endif /* ! ASM_FILE */

#endif /* ! holy_LINUX_MACHINE_HEADER */
