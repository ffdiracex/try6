/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_UTIL_MKIMAGE_HEADER
#define holy_UTIL_MKIMAGE_HEADER	1

struct holy_mkimage_layout
{
  size_t exec_size;
  size_t kernel_size;
  size_t bss_size;
  holy_uint64_t start_address;
  void *reloc_section;
  size_t reloc_size;
  size_t align;
  holy_size_t ia64jmp_off;
  holy_size_t tramp_off;
  holy_size_t got_off;
  holy_size_t got_size;
  unsigned ia64jmpnum;
  holy_uint32_t bss_start;
  holy_uint32_t end;
};

/* Private header. Use only in mkimage-related sources.  */
char *
holy_mkimage_load_image32 (const char *kernel_path,
			   size_t total_module_size,
			   struct holy_mkimage_layout *layout,
			   const struct holy_install_image_target_desc *image_target);
char *
holy_mkimage_load_image64 (const char *kernel_path,
			   size_t total_module_size,
			   struct holy_mkimage_layout *layout,
			   const struct holy_install_image_target_desc *image_target);
void
holy_mkimage_generate_elf32 (const struct holy_install_image_target_desc *image_target,
			     int note, char **core_img, size_t *core_size,
			     Elf32_Addr target_addr, holy_size_t align,
			     size_t kernel_size, size_t bss_size);
void
holy_mkimage_generate_elf64 (const struct holy_install_image_target_desc *image_target,
			     int note, char **core_img, size_t *core_size,
			     Elf64_Addr target_addr, holy_size_t align,
			     size_t kernel_size, size_t bss_size);

struct holy_install_image_target_desc
{
  const char *dirname;
  const char *names[6];
  holy_size_t voidp_sizeof;
  int bigendian;
  enum {
    IMAGE_I386_PC, IMAGE_EFI, IMAGE_COREBOOT,
    IMAGE_SPARC64_AOUT, IMAGE_SPARC64_RAW, IMAGE_SPARC64_CDCORE,
    IMAGE_I386_IEEE1275,
    IMAGE_LOONGSON_ELF, IMAGE_QEMU, IMAGE_PPC, IMAGE_YEELOONG_FLASH,
    IMAGE_FULOONG2F_FLASH, IMAGE_I386_PC_PXE, IMAGE_MIPS_ARC,
    IMAGE_QEMU_MIPS_FLASH, IMAGE_UBOOT, IMAGE_XEN, IMAGE_I386_PC_ELTORITO
  } id;
  enum
    {
      PLATFORM_FLAGS_NONE = 0,
      PLATFORM_FLAGS_DECOMPRESSORS = 2,
      PLATFORM_FLAGS_MODULES_BEFORE_KERNEL = 4,
    } flags;
  unsigned total_module_size;
  unsigned decompressor_compressed_size;
  unsigned decompressor_uncompressed_size;
  unsigned decompressor_uncompressed_addr;
  unsigned reloc_table_offset;
  unsigned link_align;
  holy_uint16_t elf_target;
  unsigned section_align;
  signed vaddr_offset;
  holy_uint64_t link_addr;
  unsigned mod_gap, mod_align;
  holy_compression_t default_compression;
  holy_uint16_t pe_target;
};

#define holy_target_to_host32(x) (holy_target_to_host32_real (image_target, (x)))
#define holy_host_to_target32(x) (holy_host_to_target32_real (image_target, (x)))
#define holy_target_to_host64(x) (holy_target_to_host64_real (image_target, (x)))
#define holy_host_to_target64(x) (holy_host_to_target64_real (image_target, (x)))
#define holy_host_to_target_addr(x) (holy_host_to_target_addr_real (image_target, (x)))
#define holy_target_to_host16(x) (holy_target_to_host16_real (image_target, (x)))
#define holy_host_to_target16(x) (holy_host_to_target16_real (image_target, (x)))

static inline holy_uint32_t
holy_target_to_host32_real (const struct holy_install_image_target_desc *image_target,
			    holy_uint32_t in)
{
  if (image_target->bigendian)
    return holy_be_to_cpu32 (in);
  else
    return holy_le_to_cpu32 (in);
}

static inline holy_uint64_t
holy_target_to_host64_real (const struct holy_install_image_target_desc *image_target,
			    holy_uint64_t in)
{
  if (image_target->bigendian)
    return holy_be_to_cpu64 (in);
  else
    return holy_le_to_cpu64 (in);
}

static inline holy_uint64_t
holy_host_to_target64_real (const struct holy_install_image_target_desc *image_target,
			    holy_uint64_t in)
{
  if (image_target->bigendian)
    return holy_cpu_to_be64 (in);
  else
    return holy_cpu_to_le64 (in);
}

static inline holy_uint32_t
holy_host_to_target32_real (const struct holy_install_image_target_desc *image_target,
			    holy_uint32_t in)
{
  if (image_target->bigendian)
    return holy_cpu_to_be32 (in);
  else
    return holy_cpu_to_le32 (in);
}

static inline holy_uint16_t
holy_target_to_host16_real (const struct holy_install_image_target_desc *image_target,
			    holy_uint16_t in)
{
  if (image_target->bigendian)
    return holy_be_to_cpu16 (in);
  else
    return holy_le_to_cpu16 (in);
}

static inline holy_uint16_t
holy_host_to_target16_real (const struct holy_install_image_target_desc *image_target,
			    holy_uint16_t in)
{
  if (image_target->bigendian)
    return holy_cpu_to_be16 (in);
  else
    return holy_cpu_to_le16 (in);
}

static inline holy_uint64_t
holy_host_to_target_addr_real (const struct holy_install_image_target_desc *image_target, holy_uint64_t in)
{
  if (image_target->voidp_sizeof == 8)
    return holy_host_to_target64_real (image_target, in);
  else
    return holy_host_to_target32_real (image_target, in);
}

static inline holy_uint64_t
holy_target_to_host_real (const struct holy_install_image_target_desc *image_target, holy_uint64_t in)
{
  if (image_target->voidp_sizeof == 8)
    return holy_target_to_host64_real (image_target, in);
  else
    return holy_target_to_host32_real (image_target, in);
}

#define holy_target_to_host(val) holy_target_to_host_real(image_target, (val))

#endif
