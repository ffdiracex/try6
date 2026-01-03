/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <holy/types.h>
#include <holy/elf.h>
#include <holy/aout.h>
#include <holy/i18n.h>
#include <holy/kernel.h>
#include <holy/disk.h>
#include <holy/emu/misc.h>
#include <holy/util/misc.h>
#include <holy/util/resolve.h>
#include <holy/misc.h>
#include <holy/offsets.h>
#include <holy/crypto.h>
#include <holy/dl.h>
#include <time.h>
#include <multiboot.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <holy/efi/pe32.h>
#include <holy/uboot/image.h>
#include <holy/arm/reloc.h>
#include <holy/arm64/reloc.h>
#include <holy/ia64/reloc.h>
#include <holy/osdep/hostfile.h>
#include <holy/util/install.h>
#include <holy/util/mkimage.h>

#define ALIGN_ADDR(x) (ALIGN_UP((x), image_target->voidp_sizeof))

#ifdef USE_LIBLZMA
#include <lzma.h>
#endif

#pragma GCC diagnostic ignored "-Wcast-align"

#define TARGET_NO_FIELD 0xffffffff

/* use 2015-01-01T00:00:00+0000 as a stock timestamp */
#define STABLE_EMBEDDING_TIMESTAMP 1420070400

#define EFI32_HEADER_SIZE ALIGN_UP (holy_PE32_MSDOS_STUB_SIZE		\
				    + holy_PE32_SIGNATURE_SIZE		\
				    + sizeof (struct holy_pe32_coff_header) \
				    + sizeof (struct holy_pe32_optional_header) \
				    + 4 * sizeof (struct holy_pe32_section_table), \
				    holy_PE32_SECTION_ALIGNMENT)

#define EFI64_HEADER_SIZE ALIGN_UP (holy_PE32_MSDOS_STUB_SIZE		\
				    + holy_PE32_SIGNATURE_SIZE		\
				    + sizeof (struct holy_pe32_coff_header) \
				    + sizeof (struct holy_pe64_optional_header) \
				    + 4 * sizeof (struct holy_pe32_section_table), \
				    holy_PE32_SECTION_ALIGNMENT)

static const struct holy_install_image_target_desc image_targets[] =
  {
    {
      .dirname = "i386-coreboot",
      .names = { "i386-coreboot", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_COREBOOT,
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .reloc_table_offset = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_I386_COREBOOT_LINK_ADDR,
      .elf_target = EM_386,
      .link_align = 4,
      .mod_gap = holy_KERNEL_I386_COREBOOT_MOD_GAP,
      .mod_align = holy_KERNEL_I386_COREBOOT_MOD_ALIGN
    },
    {
      .dirname = "i386-multiboot",
      .names = { "i386-multiboot", NULL},
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_COREBOOT,
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_I386_COREBOOT_LINK_ADDR,
      .elf_target = EM_386,
      .link_align = 4,
      .mod_gap = holy_KERNEL_I386_COREBOOT_MOD_GAP,
      .mod_align = holy_KERNEL_I386_COREBOOT_MOD_ALIGN
    },
    {
      .dirname = "i386-pc",
      .names = { "i386-pc", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_I386_PC, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = holy_DECOMPRESSOR_I386_PC_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = holy_DECOMPRESSOR_I386_PC_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_I386_PC_LINK_ADDR,
      .default_compression = holy_COMPRESSION_LZMA
    },
    {
      .dirname = "i386-pc",
      .names = { "i386-pc-pxe", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_I386_PC_PXE, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = holy_DECOMPRESSOR_I386_PC_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = holy_DECOMPRESSOR_I386_PC_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_I386_PC_LINK_ADDR,
      .default_compression = holy_COMPRESSION_LZMA
    },
    {
      .dirname = "i386-pc",
      .names = { "i386-pc-eltorito", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_I386_PC_ELTORITO, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = holy_DECOMPRESSOR_I386_PC_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = holy_DECOMPRESSOR_I386_PC_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_I386_PC_LINK_ADDR,
      .default_compression = holy_COMPRESSION_LZMA
    },
    {
      .dirname = "i386-efi",
      .names = { "i386-efi", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_EFI,
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = holy_PE32_SECTION_ALIGNMENT,
      .vaddr_offset = EFI32_HEADER_SIZE,
      .pe_target = holy_PE32_MACHINE_I386,
      .elf_target = EM_386,
    },
    {
      .dirname = "i386-ieee1275",
      .names = { "i386-ieee1275", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_I386_IEEE1275, 
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_I386_IEEE1275_LINK_ADDR,
      .elf_target = EM_386,
      .mod_gap = holy_KERNEL_I386_IEEE1275_MOD_GAP,
      .mod_align = holy_KERNEL_I386_IEEE1275_MOD_ALIGN,
      .link_align = 4,
    },
    {
      .dirname = "i386-qemu",
      .names = { "i386-qemu", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_QEMU, 
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_I386_QEMU_LINK_ADDR
    },
    {
      .dirname = "x86_64-efi",
      .names = { "x86_64-efi", NULL },
      .voidp_sizeof = 8,
      .bigendian = 0, 
      .id = IMAGE_EFI, 
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = holy_PE32_SECTION_ALIGNMENT,
      .vaddr_offset = EFI64_HEADER_SIZE,
      .pe_target = holy_PE32_MACHINE_X86_64,
      .elf_target = EM_X86_64,
    },
    {
      .dirname = "i386-xen",
      .names = { "i386-xen", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_XEN, 
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = 0,
      .elf_target = EM_386,
      .mod_gap = holy_KERNEL_I386_XEN_MOD_GAP,
      .mod_align = holy_KERNEL_I386_XEN_MOD_ALIGN,
      .link_align = 4
    },
    {
      .dirname = "x86_64-xen",
      .names = { "x86_64-xen", NULL },
      .voidp_sizeof = 8,
      .bigendian = 0,
      .id = IMAGE_XEN, 
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = 0,
      .elf_target = EM_X86_64,
      .mod_gap = holy_KERNEL_X86_64_XEN_MOD_GAP,
      .mod_align = holy_KERNEL_X86_64_XEN_MOD_ALIGN,
      .link_align = 8
    },
    {
      .dirname = "mipsel-loongson",
      .names = { "mipsel-yeeloong-flash", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_YEELOONG_FLASH, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = holy_KERNEL_MIPS_LOONGSON_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = holy_DECOMPRESSOR_MIPS_LOONGSON_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = holy_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = holy_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_ADDR,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_MIPS_LOONGSON_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = holy_KERNEL_MIPS_LOONGSON_LINK_ALIGN,
      .default_compression = holy_COMPRESSION_NONE
    },
    {
      .dirname = "mipsel-loongson",
      .names = { "mipsel-fuloong2f-flash", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_FULOONG2F_FLASH, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = holy_KERNEL_MIPS_LOONGSON_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = holy_DECOMPRESSOR_MIPS_LOONGSON_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = holy_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = holy_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_ADDR,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_MIPS_LOONGSON_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = holy_KERNEL_MIPS_LOONGSON_LINK_ALIGN,
      .default_compression = holy_COMPRESSION_NONE
    },
    {
      .dirname = "mipsel-loongson",
      .names = { "mipsel-loongson-elf", "mipsel-yeeloong-elf",
		 "mipsel-fuloong2f-elf", "mipsel-fuloong2e-elf",
		 "mipsel-fuloong-elf", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_LOONGSON_ELF, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = holy_KERNEL_MIPS_LOONGSON_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = holy_DECOMPRESSOR_MIPS_LOONGSON_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = holy_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = holy_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_ADDR,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_MIPS_LOONGSON_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = holy_KERNEL_MIPS_LOONGSON_LINK_ALIGN,
      .default_compression = holy_COMPRESSION_NONE
    },
    {
      .dirname = "powerpc-ieee1275",
      .names = { "powerpc-ieee1275", NULL },
      .voidp_sizeof = 4,
      .bigendian = 1,
      .id = IMAGE_PPC, 
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_POWERPC_IEEE1275_LINK_ADDR,
      .elf_target = EM_PPC,
      .mod_gap = holy_KERNEL_POWERPC_IEEE1275_MOD_GAP,
      .mod_align = holy_KERNEL_POWERPC_IEEE1275_MOD_ALIGN,
      .link_align = 4
    },
    {
      .dirname = "sparc64-ieee1275",
      .names = { "sparc64-ieee1275-raw", NULL },
      .voidp_sizeof = 8,
      .bigendian = 1, 
      .id = IMAGE_SPARC64_RAW,
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = holy_KERNEL_SPARC64_IEEE1275_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_SPARC64_IEEE1275_LINK_ADDR,
      .mod_align = holy_KERNEL_SPARC64_IEEE1275_MOD_ALIGN,
    },
    {
      .dirname = "sparc64-ieee1275",
      .names = { "sparc64-ieee1275-cdcore", NULL },
      .voidp_sizeof = 8,
      .bigendian = 1, 
      .id = IMAGE_SPARC64_CDCORE,
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = holy_KERNEL_SPARC64_IEEE1275_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_SPARC64_IEEE1275_LINK_ADDR,
      .mod_align = holy_KERNEL_SPARC64_IEEE1275_MOD_ALIGN,
    },
    {
      .dirname = "sparc64-ieee1275",
      .names = { "sparc64-ieee1275-aout", NULL },
      .voidp_sizeof = 8,
      .bigendian = 1,
      .id = IMAGE_SPARC64_AOUT,
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = holy_KERNEL_SPARC64_IEEE1275_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_SPARC64_IEEE1275_LINK_ADDR,
      .mod_align = holy_KERNEL_SPARC64_IEEE1275_MOD_ALIGN,
    },
    {
      .dirname = "ia64-efi",
      .names = {"ia64-efi", NULL},
      .voidp_sizeof = 8,
      .bigendian = 0, 
      .id = IMAGE_EFI, 
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = holy_PE32_SECTION_ALIGNMENT,
      .vaddr_offset = EFI64_HEADER_SIZE,
      .pe_target = holy_PE32_MACHINE_IA64,
      .elf_target = EM_IA_64,
    },
    {
      .dirname = "mips-arc",
      .names = {"mips-arc", NULL},
      .voidp_sizeof = 4,
      .bigendian = 1,
      .id = IMAGE_MIPS_ARC, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = holy_KERNEL_MIPS_ARC_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = holy_DECOMPRESSOR_MIPS_LOONGSON_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = holy_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = holy_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_ADDR,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_MIPS_ARC_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = holy_KERNEL_MIPS_ARC_LINK_ALIGN,
      .default_compression = holy_COMPRESSION_NONE
    },
    {
      .dirname = "mipsel-arc",
      .names = {"mipsel-arc", NULL},
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_MIPS_ARC, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = holy_KERNEL_MIPS_ARC_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = holy_DECOMPRESSOR_MIPS_LOONGSON_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = holy_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = holy_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_ADDR,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_MIPSEL_ARC_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = holy_KERNEL_MIPS_ARC_LINK_ALIGN,
      .default_compression = holy_COMPRESSION_NONE
    },
    {
      .dirname = "mipsel-qemu_mips",
      .names = { "mipsel-qemu_mips-elf", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_LOONGSON_ELF, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = holy_KERNEL_MIPS_QEMU_MIPS_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = holy_DECOMPRESSOR_MIPS_LOONGSON_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = holy_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = holy_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_ADDR,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_MIPS_QEMU_MIPS_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = holy_KERNEL_MIPS_QEMU_MIPS_LINK_ALIGN,
      .default_compression = holy_COMPRESSION_NONE
    },
    {
      .dirname = "mips-qemu_mips",
      .names = { "mips-qemu_mips-flash", NULL },
      .voidp_sizeof = 4,
      .bigendian = 1,
      .id = IMAGE_QEMU_MIPS_FLASH, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = holy_KERNEL_MIPS_QEMU_MIPS_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = holy_DECOMPRESSOR_MIPS_LOONGSON_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = holy_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = holy_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_ADDR,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_MIPS_QEMU_MIPS_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = holy_KERNEL_MIPS_QEMU_MIPS_LINK_ALIGN,
      .default_compression = holy_COMPRESSION_NONE
    },
    {
      .dirname = "mipsel-qemu_mips",
      .names = { "mipsel-qemu_mips-flash", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_QEMU_MIPS_FLASH, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = holy_KERNEL_MIPS_QEMU_MIPS_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = holy_DECOMPRESSOR_MIPS_LOONGSON_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = holy_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = holy_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_ADDR,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_MIPS_QEMU_MIPS_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = holy_KERNEL_MIPS_QEMU_MIPS_LINK_ALIGN,
      .default_compression = holy_COMPRESSION_NONE
    },
    {
      .dirname = "mips-qemu_mips",
      .names = { "mips-qemu_mips-elf", NULL },
      .voidp_sizeof = 4,
      .bigendian = 1,
      .id = IMAGE_LOONGSON_ELF, 
      .flags = PLATFORM_FLAGS_DECOMPRESSORS,
      .total_module_size = holy_KERNEL_MIPS_QEMU_MIPS_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = holy_DECOMPRESSOR_MIPS_LOONGSON_COMPRESSED_SIZE,
      .decompressor_uncompressed_size = holy_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_SIZE,
      .decompressor_uncompressed_addr = holy_DECOMPRESSOR_MIPS_LOONGSON_UNCOMPRESSED_ADDR,
      .section_align = 1,
      .vaddr_offset = 0,
      .link_addr = holy_KERNEL_MIPS_QEMU_MIPS_LINK_ADDR,
      .elf_target = EM_MIPS,
      .link_align = holy_KERNEL_MIPS_QEMU_MIPS_LINK_ALIGN,
      .default_compression = holy_COMPRESSION_NONE
    },
    {
      .dirname = "arm-uboot",
      .names = { "arm-uboot", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0,
      .id = IMAGE_UBOOT, 
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = holy_KERNEL_ARM_UBOOT_TOTAL_MODULE_SIZE,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = holy_KERNEL_ARM_UBOOT_MOD_ALIGN,
      .vaddr_offset = 0,
      .elf_target = EM_ARM,
      .mod_gap = holy_KERNEL_ARM_UBOOT_MOD_GAP,
      .mod_align = holy_KERNEL_ARM_UBOOT_MOD_ALIGN,
      .link_align = 4
    },
    {
      .dirname = "arm-efi",
      .names = { "arm-efi", NULL },
      .voidp_sizeof = 4,
      .bigendian = 0, 
      .id = IMAGE_EFI, 
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = holy_PE32_SECTION_ALIGNMENT,
      .vaddr_offset = ALIGN_UP (holy_PE32_MSDOS_STUB_SIZE
                                + holy_PE32_SIGNATURE_SIZE
                                + sizeof (struct holy_pe32_coff_header)
                                + sizeof (struct holy_pe32_optional_header)
                                + 4 * sizeof (struct holy_pe32_section_table),
                                holy_PE32_SECTION_ALIGNMENT),
      .pe_target = holy_PE32_MACHINE_ARMTHUMB_MIXED,
      .elf_target = EM_ARM,
    },
    {
      .dirname = "arm64-efi",
      .names = { "arm64-efi", NULL },
      .voidp_sizeof = 8,
      .bigendian = 0,
      .id = IMAGE_EFI,
      .flags = PLATFORM_FLAGS_NONE,
      .total_module_size = TARGET_NO_FIELD,
      .decompressor_compressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_size = TARGET_NO_FIELD,
      .decompressor_uncompressed_addr = TARGET_NO_FIELD,
      .section_align = holy_PE32_SECTION_ALIGNMENT,
      .vaddr_offset = EFI64_HEADER_SIZE,
      .pe_target = holy_PE32_MACHINE_ARM64,
      .elf_target = EM_AARCH64,
    },
  };

#include <holy/lib/LzmaEnc.h>

static void *SzAlloc(void *p __attribute__ ((unused)), size_t size) { return xmalloc(size); }
static void SzFree(void *p __attribute__ ((unused)), void *address) { free(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

static void
compress_kernel_lzma (char *kernel_img, size_t kernel_size,
		      char **core_img, size_t *core_size)
{
  CLzmaEncProps props;
  unsigned char out_props[5];
  size_t out_props_size = 5;

  LzmaEncProps_Init(&props);
  props.dictSize = 1 << 16;
  props.lc = 3;
  props.lp = 0;
  props.pb = 2;
  props.numThreads = 1;

  *core_img = xmalloc (kernel_size);

  *core_size = kernel_size;
  if (LzmaEncode ((unsigned char *) *core_img, core_size,
		  (unsigned char *) kernel_img,
		  kernel_size,
		  &props, out_props, &out_props_size,
		  0, NULL, &g_Alloc, &g_Alloc) != SZ_OK)
    holy_util_error ("%s", _("cannot compress the kernel image"));
}

#ifdef USE_LIBLZMA
static void
compress_kernel_xz (char *kernel_img, size_t kernel_size,
		    char **core_img, size_t *core_size)
{
  lzma_stream strm = LZMA_STREAM_INIT;
  lzma_ret xzret;
  lzma_options_lzma lzopts = {
    .dict_size = 1 << 16,
    .preset_dict = NULL,
    .preset_dict_size = 0,
    .lc = 3,
    .lp = 0,
    .pb = 2,
    .mode = LZMA_MODE_NORMAL,
    .nice_len = 64,
    .mf = LZMA_MF_BT4,
    .depth = 0,
  };
  lzma_filter fltrs[] = {
    { .id = LZMA_FILTER_LZMA2, .options = &lzopts},
    { .id = LZMA_VLI_UNKNOWN, .options = NULL}
  };

  xzret = lzma_stream_encoder (&strm, fltrs, LZMA_CHECK_NONE);
  if (xzret != LZMA_OK)
    holy_util_error ("%s", _("cannot compress the kernel image"));

  *core_img = xmalloc (kernel_size);

  *core_size = kernel_size;
  strm.next_in = (unsigned char *) kernel_img;
  strm.avail_in = kernel_size;
  strm.next_out = (unsigned char *) *core_img;
  strm.avail_out = *core_size;

  while (1)
    {
      xzret = lzma_code (&strm, LZMA_FINISH);
      if (xzret == LZMA_OK)
	continue;
      if (xzret == LZMA_STREAM_END)
	break;
      holy_util_error ("%s", _("cannot compress the kernel image"));
    }

  *core_size -= strm.avail_out;
}
#endif

static void
compress_kernel (const struct holy_install_image_target_desc *image_target, char *kernel_img,
		 size_t kernel_size, char **core_img, size_t *core_size,
		 holy_compression_t comp)
{
  if (image_target->flags & PLATFORM_FLAGS_DECOMPRESSORS
      && (comp == holy_COMPRESSION_LZMA))
    {
      compress_kernel_lzma (kernel_img, kernel_size, core_img,
			    core_size);
      return;
    }

#ifdef USE_LIBLZMA
 if (image_target->flags & PLATFORM_FLAGS_DECOMPRESSORS
     && (comp == holy_COMPRESSION_XZ))
   {
     compress_kernel_xz (kernel_img, kernel_size, core_img,
			 core_size);
     return;
   }
#endif

 if (image_target->flags & PLATFORM_FLAGS_DECOMPRESSORS
     && (comp != holy_COMPRESSION_NONE))
   holy_util_error (_("unknown compression %d"), comp);

  *core_img = xmalloc (kernel_size);
  memcpy (*core_img, kernel_img, kernel_size);
  *core_size = kernel_size;
}

const struct holy_install_image_target_desc *
holy_install_get_image_target (const char *arg)
{
  unsigned i, j;
  for (i = 0; i < ARRAY_SIZE (image_targets); i++)
    for (j = 0; j < ARRAY_SIZE (image_targets[i].names) &&
		    image_targets[i].names[j]; j++)
      if (strcmp (arg, image_targets[i].names[j]) == 0)
	return &image_targets[i];
  return NULL;
}

const char *
holy_util_get_target_dirname (const struct holy_install_image_target_desc *t)
{
  return t->dirname;
}

const char *
holy_util_get_target_name (const struct holy_install_image_target_desc *t)
{
  return t->names[0];
}

char *
holy_install_get_image_targets_string (void)
{
  int format_len = 0;
  char *formats;
  char *ptr;
  unsigned i;
  for (i = 0; i < ARRAY_SIZE (image_targets); i++)
    format_len += strlen (image_targets[i].names[0]) + 2;
  ptr = formats = xmalloc (format_len);
  for (i = 0; i < ARRAY_SIZE (image_targets); i++)
    {
      strcpy (ptr, image_targets[i].names[0]);
      ptr += strlen (image_targets[i].names[0]);
      *ptr++ = ',';
      *ptr++ = ' ';
    }
  ptr[-2] = 0;

  return formats;
}

void
holy_install_generate_image (const char *dir, const char *prefix,
			     FILE *out, const char *outname, char *mods[],
			     char *memdisk_path, char **pubkey_paths,
			     size_t npubkeys, char *config_path,
			     const struct holy_install_image_target_desc *image_target,
			     int note,
			     holy_compression_t comp)
{
  char *kernel_img, *core_img;
  size_t total_module_size, core_size;
  size_t memdisk_size = 0, config_size = 0;
  size_t prefix_size = 0;
  char *kernel_path;
  size_t offset;
  struct holy_util_path_list *path_list, *p;
  size_t decompress_size = 0;
  struct holy_mkimage_layout layout;

  if (comp == holy_COMPRESSION_AUTO)
    comp = image_target->default_compression;

  if (image_target->id == IMAGE_I386_PC
      || image_target->id == IMAGE_I386_PC_PXE
      || image_target->id == IMAGE_I386_PC_ELTORITO)
    comp = holy_COMPRESSION_LZMA;

  path_list = holy_util_resolve_dependencies (dir, "moddep.lst", mods);

  kernel_path = holy_util_get_path (dir, "kernel.img");

  if (image_target->voidp_sizeof == 8)
    total_module_size = sizeof (struct holy_module_info64);
  else
    total_module_size = sizeof (struct holy_module_info32);

  {
    size_t i;
    for (i = 0; i < npubkeys; i++)
      {
	size_t curs;
	curs = ALIGN_ADDR (holy_util_get_image_size (pubkey_paths[i]));
	holy_util_info ("the size of public key %u is 0x%"
			holy_HOST_PRIxLONG_LONG,
			(unsigned) i, (unsigned long long) curs);
	total_module_size += curs + sizeof (struct holy_module_header);
      }
  }

  if (memdisk_path)
    {
      memdisk_size = ALIGN_UP(holy_util_get_image_size (memdisk_path), 512);
      holy_util_info ("the size of memory disk is 0x%" holy_HOST_PRIxLONG_LONG,
		      (unsigned long long) memdisk_size);
      total_module_size += memdisk_size + sizeof (struct holy_module_header);
    }

  if (config_path)
    {
      config_size = ALIGN_ADDR (holy_util_get_image_size (config_path) + 1);
      holy_util_info ("the size of config file is 0x%" holy_HOST_PRIxLONG_LONG,
		      (unsigned long long) config_size);
      total_module_size += config_size + sizeof (struct holy_module_header);
    }

  if (prefix)
    {
      prefix_size = ALIGN_ADDR (strlen (prefix) + 1);
      total_module_size += prefix_size + sizeof (struct holy_module_header);
    }

  for (p = path_list; p; p = p->next)
    total_module_size += (ALIGN_ADDR (holy_util_get_image_size (p->name))
			  + sizeof (struct holy_module_header));

  holy_util_info ("the total module size is 0x%" holy_HOST_PRIxLONG_LONG,
		  (unsigned long long) total_module_size);

  if (image_target->voidp_sizeof == 4)
    kernel_img = holy_mkimage_load_image32 (kernel_path, total_module_size,
					    &layout, image_target);
  else
    kernel_img = holy_mkimage_load_image64 (kernel_path, total_module_size,
					    &layout, image_target);
  if (image_target->id == IMAGE_XEN && layout.align < 4096)
    layout.align = 4096;

  if ((image_target->flags & PLATFORM_FLAGS_DECOMPRESSORS)
      && (image_target->total_module_size != TARGET_NO_FIELD))
    *((holy_uint32_t *) (kernel_img + image_target->total_module_size))
      = holy_host_to_target32 (total_module_size);

  if (image_target->flags & PLATFORM_FLAGS_MODULES_BEFORE_KERNEL)
    {
      memmove (kernel_img + total_module_size, kernel_img, layout.kernel_size);
      memset (kernel_img, 0, total_module_size);
    }

  if (image_target->voidp_sizeof == 8)
    {
      /* Fill in the holy_module_info structure.  */
      struct holy_module_info64 *modinfo;
      if (image_target->flags & PLATFORM_FLAGS_MODULES_BEFORE_KERNEL)
	modinfo = (struct holy_module_info64 *) kernel_img;
      else
	modinfo = (struct holy_module_info64 *) (kernel_img + layout.kernel_size);
      modinfo->magic = holy_host_to_target32 (holy_MODULE_MAGIC);
      modinfo->offset = holy_host_to_target_addr (sizeof (struct holy_module_info64));
      modinfo->size = holy_host_to_target_addr (total_module_size);
      if (image_target->flags & PLATFORM_FLAGS_MODULES_BEFORE_KERNEL)
	offset = sizeof (struct holy_module_info64);
      else
	offset = layout.kernel_size + sizeof (struct holy_module_info64);
    }
  else
    {
      /* Fill in the holy_module_info structure.  */
      struct holy_module_info32 *modinfo;
      if (image_target->flags & PLATFORM_FLAGS_MODULES_BEFORE_KERNEL)
	modinfo = (struct holy_module_info32 *) kernel_img;
      else
	modinfo = (struct holy_module_info32 *) (kernel_img + layout.kernel_size);
      modinfo->magic = holy_host_to_target32 (holy_MODULE_MAGIC);
      modinfo->offset = holy_host_to_target_addr (sizeof (struct holy_module_info32));
      modinfo->size = holy_host_to_target_addr (total_module_size);
      if (image_target->flags & PLATFORM_FLAGS_MODULES_BEFORE_KERNEL)
	offset = sizeof (struct holy_module_info32);
      else
	offset = layout.kernel_size + sizeof (struct holy_module_info32);
    }

  for (p = path_list; p; p = p->next)
    {
      struct holy_module_header *header;
      size_t mod_size;

      mod_size = ALIGN_ADDR (holy_util_get_image_size (p->name));

      header = (struct holy_module_header *) (kernel_img + offset);
      header->type = holy_host_to_target32 (OBJ_TYPE_ELF);
      header->size = holy_host_to_target32 (mod_size + sizeof (*header));
      offset += sizeof (*header);

      holy_util_load_image (p->name, kernel_img + offset);
      offset += mod_size;
    }

  {
    size_t i;
    for (i = 0; i < npubkeys; i++)
      {
	size_t curs;
	struct holy_module_header *header;

	curs = holy_util_get_image_size (pubkey_paths[i]);

	header = (struct holy_module_header *) (kernel_img + offset);
	header->type = holy_host_to_target32 (OBJ_TYPE_PUBKEY);
	header->size = holy_host_to_target32 (curs + sizeof (*header));
	offset += sizeof (*header);

	holy_util_load_image (pubkey_paths[i], kernel_img + offset);
	offset += ALIGN_ADDR (curs);
      }
  }

  if (memdisk_path)
    {
      struct holy_module_header *header;

      header = (struct holy_module_header *) (kernel_img + offset);
      header->type = holy_host_to_target32 (OBJ_TYPE_MEMDISK);
      header->size = holy_host_to_target32 (memdisk_size + sizeof (*header));
      offset += sizeof (*header);

      holy_util_load_image (memdisk_path, kernel_img + offset);
      offset += memdisk_size;
    }

  if (config_path)
    {
      struct holy_module_header *header;

      header = (struct holy_module_header *) (kernel_img + offset);
      header->type = holy_host_to_target32 (OBJ_TYPE_CONFIG);
      header->size = holy_host_to_target32 (config_size + sizeof (*header));
      offset += sizeof (*header);

      holy_util_load_image (config_path, kernel_img + offset);
      offset += config_size;
    }

  if (prefix)
    {
      struct holy_module_header *header;

      header = (struct holy_module_header *) (kernel_img + offset);
      header->type = holy_host_to_target32 (OBJ_TYPE_PREFIX);
      header->size = holy_host_to_target32 (prefix_size + sizeof (*header));
      offset += sizeof (*header);

      holy_strcpy (kernel_img + offset, prefix);
      offset += prefix_size;
    }

  holy_util_info ("kernel_img=%p, kernel_size=0x%" holy_HOST_PRIxLONG_LONG,
		  kernel_img,
		  (unsigned long long) layout.kernel_size);
  compress_kernel (image_target, kernel_img, layout.kernel_size + total_module_size,
		   &core_img, &core_size, comp);
  free (kernel_img);

  holy_util_info ("the core size is 0x%" holy_HOST_PRIxLONG_LONG,
		  (unsigned long long) core_size);

  if (!(image_target->flags & PLATFORM_FLAGS_DECOMPRESSORS) 
      && image_target->total_module_size != TARGET_NO_FIELD)
    *((holy_uint32_t *) (core_img + image_target->total_module_size))
      = holy_host_to_target32 (total_module_size);

  if (image_target->flags & PLATFORM_FLAGS_DECOMPRESSORS)
    {
      char *full_img;
      size_t full_size;
      char *decompress_path, *decompress_img;
      const char *name;

      switch (comp)
	{
	case holy_COMPRESSION_XZ:
	  name = "xz_decompress.img";
	  break;
	case holy_COMPRESSION_LZMA:
	  name = "lzma_decompress.img";
	  break;
	case holy_COMPRESSION_NONE:
	  name = "none_decompress.img";
	  break;
	default:
	  holy_util_error (_("unknown compression %d"), comp);
	}
      
      decompress_path = holy_util_get_path (dir, name);
      decompress_size = holy_util_get_image_size (decompress_path);
      decompress_img = holy_util_read_image (decompress_path);

      if ((image_target->id == IMAGE_I386_PC
	   || image_target->id == IMAGE_I386_PC_PXE
	   || image_target->id == IMAGE_I386_PC_ELTORITO)
	  && decompress_size > holy_KERNEL_I386_PC_LINK_ADDR - 0x8200)
	holy_util_error ("%s", _("Decompressor is too big"));

      if (image_target->decompressor_compressed_size != TARGET_NO_FIELD)
	*((holy_uint32_t *) (decompress_img
			     + image_target->decompressor_compressed_size))
	  = holy_host_to_target32 (core_size);

      if (image_target->decompressor_uncompressed_size != TARGET_NO_FIELD)
	*((holy_uint32_t *) (decompress_img
			     + image_target->decompressor_uncompressed_size))
	  = holy_host_to_target32 (layout.kernel_size + total_module_size);

      if (image_target->decompressor_uncompressed_addr != TARGET_NO_FIELD)
	{
	  if (image_target->flags & PLATFORM_FLAGS_MODULES_BEFORE_KERNEL)
	    *((holy_uint32_t *) (decompress_img + image_target->decompressor_uncompressed_addr))
	      = holy_host_to_target_addr (image_target->link_addr - total_module_size);
	  else
	    *((holy_uint32_t *) (decompress_img + image_target->decompressor_uncompressed_addr))
	      = holy_host_to_target_addr (image_target->link_addr);
	}
      full_size = core_size + decompress_size;

      full_img = xmalloc (full_size);

      memcpy (full_img, decompress_img, decompress_size);

      memcpy (full_img + decompress_size, core_img, core_size);

      free (core_img);
      core_img = full_img;
      core_size = full_size;
      free (decompress_img);
      free (decompress_path);
    }

  switch (image_target->id)
    {
    case IMAGE_I386_PC:
    case IMAGE_I386_PC_PXE:
    case IMAGE_I386_PC_ELTORITO:
	if (holy_KERNEL_I386_PC_LINK_ADDR + core_size > 0x78000
	    || (core_size > (0xffff << holy_DISK_SECTOR_BITS))
	    || (layout.kernel_size + layout.bss_size
		+ holy_KERNEL_I386_PC_LINK_ADDR > 0x68000))
	  holy_util_error (_("core image is too big (0x%x > 0x%x)"),
			   holy_KERNEL_I386_PC_LINK_ADDR + (unsigned) core_size,
			   0x78000);
	/* fallthrough */
    case IMAGE_COREBOOT:
    case IMAGE_QEMU:
	if (layout.kernel_size + layout.bss_size + holy_KERNEL_I386_PC_LINK_ADDR > 0x68000)
	  holy_util_error (_("kernel image is too big (0x%x > 0x%x)"),
			   (unsigned) layout.kernel_size + (unsigned) layout.bss_size
			   + holy_KERNEL_I386_PC_LINK_ADDR,
			   0x68000);
	break;
    case IMAGE_LOONGSON_ELF:
    case IMAGE_YEELOONG_FLASH:
    case IMAGE_FULOONG2F_FLASH:
    case IMAGE_EFI:
    case IMAGE_MIPS_ARC:
    case IMAGE_QEMU_MIPS_FLASH:
    case IMAGE_XEN:
      break;
    case IMAGE_SPARC64_AOUT:
    case IMAGE_SPARC64_RAW:
    case IMAGE_SPARC64_CDCORE:
    case IMAGE_I386_IEEE1275:
    case IMAGE_PPC:
    case IMAGE_UBOOT:
      break;
    }

  switch (image_target->id)
    {
    case IMAGE_I386_PC:
    case IMAGE_I386_PC_PXE:
    case IMAGE_I386_PC_ELTORITO:
      {
	unsigned num;
	char *boot_path, *boot_img;
	size_t boot_size;

	num = ((core_size + holy_DISK_SECTOR_SIZE - 1) >> holy_DISK_SECTOR_BITS);
	if (image_target->id == IMAGE_I386_PC_PXE)
	  {
	    char *pxeboot_path, *pxeboot_img;
	    size_t pxeboot_size;
	    holy_uint32_t *ptr;
	    
	    pxeboot_path = holy_util_get_path (dir, "pxeboot.img");
	    pxeboot_size = holy_util_get_image_size (pxeboot_path);
	    pxeboot_img = holy_util_read_image (pxeboot_path);
	    
	    holy_util_write_image (pxeboot_img, pxeboot_size, out,
				   outname);
	    free (pxeboot_img);
	    free (pxeboot_path);

	    /* Remove Multiboot header to avoid confusing ipxe.  */
	    for (ptr = (holy_uint32_t *) core_img;
		 ptr < (holy_uint32_t *) (core_img + MULTIBOOT_SEARCH); ptr++)
	      if (*ptr == holy_host_to_target32 (MULTIBOOT_HEADER_MAGIC)
		  && holy_target_to_host32 (ptr[0])
		  + holy_target_to_host32 (ptr[1])
		  + holy_target_to_host32 (ptr[2]) == 0)
		{
		  *ptr = 0;
		  break;
		}
	  }

	if (image_target->id == IMAGE_I386_PC_ELTORITO)
	  {
	    char *eltorito_path, *eltorito_img;
	    size_t eltorito_size;
	    
	    eltorito_path = holy_util_get_path (dir, "cdboot.img");
	    eltorito_size = holy_util_get_image_size (eltorito_path);
	    eltorito_img = holy_util_read_image (eltorito_path);
	    
	    holy_util_write_image (eltorito_img, eltorito_size, out,
				   outname);
	    free (eltorito_img);
	    free (eltorito_path);
	  }

	boot_path = holy_util_get_path (dir, "diskboot.img");
	boot_size = holy_util_get_image_size (boot_path);
	if (boot_size != holy_DISK_SECTOR_SIZE)
	  holy_util_error (_("diskboot.img size must be %u bytes"),
			   holy_DISK_SECTOR_SIZE);

	boot_img = holy_util_read_image (boot_path);

	{
	  struct holy_pc_bios_boot_blocklist *block;
	  block = (struct holy_pc_bios_boot_blocklist *) (boot_img
							  + holy_DISK_SECTOR_SIZE
							  - sizeof (*block));
	  block->len = holy_host_to_target16 (num);

	  /* This is filled elsewhere.  Verify it just in case.  */
	  assert (block->segment
		  == holy_host_to_target16 (holy_BOOT_I386_PC_KERNEL_SEG
					    + (holy_DISK_SECTOR_SIZE >> 4)));
	}

	holy_util_write_image (boot_img, boot_size, out, outname);
	free (boot_img);
	free (boot_path);
      }
      break;
    case IMAGE_EFI:
      {
	void *pe_img;
	holy_uint8_t *header;
	void *sections;
	size_t pe_size;
	struct holy_pe32_coff_header *c;
	struct holy_pe32_section_table *text_section, *data_section;
	struct holy_pe32_section_table *mods_section, *reloc_section;
	static const holy_uint8_t stub[] = holy_PE32_MSDOS_STUB;
	int header_size;
	int reloc_addr;

	if (image_target->voidp_sizeof == 4)
	  header_size = EFI32_HEADER_SIZE;
	else
	  header_size = EFI64_HEADER_SIZE;

	reloc_addr = ALIGN_UP (header_size + core_size,
			       image_target->section_align);

	pe_size = ALIGN_UP (reloc_addr + layout.reloc_size,
			    image_target->section_align);
	pe_img = xmalloc (reloc_addr + layout.reloc_size);
	memset (pe_img, 0, header_size);
	memcpy ((char *) pe_img + header_size, core_img, core_size);
	memset ((char *) pe_img + header_size + core_size, 0, reloc_addr - (header_size + core_size));
	memcpy ((char *) pe_img + reloc_addr, layout.reloc_section, layout.reloc_size);
	header = pe_img;

	/* The magic.  */
	memcpy (header, stub, holy_PE32_MSDOS_STUB_SIZE);
	memcpy (header + holy_PE32_MSDOS_STUB_SIZE, "PE\0\0",
		holy_PE32_SIGNATURE_SIZE);

	/* The COFF file header.  */
	c = (struct holy_pe32_coff_header *) (header + holy_PE32_MSDOS_STUB_SIZE
					      + holy_PE32_SIGNATURE_SIZE);
	c->machine = holy_host_to_target16 (image_target->pe_target);

	c->num_sections = holy_host_to_target16 (4);
	c->time = holy_host_to_target32 (STABLE_EMBEDDING_TIMESTAMP);
	c->characteristics = holy_host_to_target16 (holy_PE32_EXECUTABLE_IMAGE
						    | holy_PE32_LINE_NUMS_STRIPPED
						    | ((image_target->voidp_sizeof == 4)
						       ? holy_PE32_32BIT_MACHINE
						       : 0)
						    | holy_PE32_LOCAL_SYMS_STRIPPED
						    | holy_PE32_DEBUG_STRIPPED);

	/* The PE Optional header.  */
	if (image_target->voidp_sizeof == 4)
	  {
	    struct holy_pe32_optional_header *o;

	    c->optional_header_size = holy_host_to_target16 (sizeof (struct holy_pe32_optional_header));

	    o = (struct holy_pe32_optional_header *)
	      (header + holy_PE32_MSDOS_STUB_SIZE + holy_PE32_SIGNATURE_SIZE
	       + sizeof (struct holy_pe32_coff_header));
	    o->magic = holy_host_to_target16 (holy_PE32_PE32_MAGIC);
	    o->code_size = holy_host_to_target32 (layout.exec_size);
	    o->data_size = holy_cpu_to_le32 (reloc_addr - layout.exec_size
					     - header_size);
	    o->bss_size = holy_cpu_to_le32 (layout.bss_size);
	    o->entry_addr = holy_cpu_to_le32 (layout.start_address);
	    o->code_base = holy_cpu_to_le32 (header_size);

	    o->data_base = holy_host_to_target32 (header_size + layout.exec_size);

	    o->image_base = 0;
	    o->section_alignment = holy_host_to_target32 (image_target->section_align);
	    o->file_alignment = holy_host_to_target32 (image_target->section_align);
	    o->image_size = holy_host_to_target32 (pe_size);
	    o->header_size = holy_host_to_target32 (header_size);
	    o->subsystem = holy_host_to_target16 (holy_PE32_SUBSYSTEM_EFI_APPLICATION);

	    /* Do these really matter? */
	    o->stack_reserve_size = holy_host_to_target32 (0x10000);
	    o->stack_commit_size = holy_host_to_target32 (0x10000);
	    o->heap_reserve_size = holy_host_to_target32 (0x10000);
	    o->heap_commit_size = holy_host_to_target32 (0x10000);
    
	    o->num_data_directories = holy_host_to_target32 (holy_PE32_NUM_DATA_DIRECTORIES);

	    o->base_relocation_table.rva = holy_host_to_target32 (reloc_addr);
	    o->base_relocation_table.size = holy_host_to_target32 (layout.reloc_size);
	    sections = o + 1;
	  }
	else
	  {
	    struct holy_pe64_optional_header *o;

	    c->optional_header_size = holy_host_to_target16 (sizeof (struct holy_pe64_optional_header));

	    o = (struct holy_pe64_optional_header *)
	      (header + holy_PE32_MSDOS_STUB_SIZE + holy_PE32_SIGNATURE_SIZE
	       + sizeof (struct holy_pe32_coff_header));
	    o->magic = holy_host_to_target16 (holy_PE32_PE64_MAGIC);
	    o->code_size = holy_host_to_target32 (layout.exec_size);
	    o->data_size = holy_cpu_to_le32 (reloc_addr - layout.exec_size
					     - header_size);
	    o->bss_size = holy_cpu_to_le32 (layout.bss_size);
	    o->entry_addr = holy_cpu_to_le32 (layout.start_address);
	    o->code_base = holy_cpu_to_le32 (header_size);
	    o->image_base = 0;
	    o->section_alignment = holy_host_to_target32 (image_target->section_align);
	    o->file_alignment = holy_host_to_target32 (image_target->section_align);
	    o->image_size = holy_host_to_target32 (pe_size);
	    o->header_size = holy_host_to_target32 (header_size);
	    o->subsystem = holy_host_to_target16 (holy_PE32_SUBSYSTEM_EFI_APPLICATION);

	    /* Do these really matter? */
	    o->stack_reserve_size = holy_host_to_target64 (0x10000);
	    o->stack_commit_size = holy_host_to_target64 (0x10000);
	    o->heap_reserve_size = holy_host_to_target64 (0x10000);
	    o->heap_commit_size = holy_host_to_target64 (0x10000);
    
	    o->num_data_directories
	      = holy_host_to_target32 (holy_PE32_NUM_DATA_DIRECTORIES);

	    o->base_relocation_table.rva = holy_host_to_target32 (reloc_addr);
	    o->base_relocation_table.size = holy_host_to_target32 (layout.reloc_size);
	    sections = o + 1;
	  }
	/* The sections.  */
	text_section = sections;
	strcpy (text_section->name, ".text");
	text_section->virtual_size = holy_cpu_to_le32 (layout.exec_size);
	text_section->virtual_address = holy_cpu_to_le32 (header_size);
	text_section->raw_data_size = holy_cpu_to_le32 (layout.exec_size);
	text_section->raw_data_offset = holy_cpu_to_le32 (header_size);
	text_section->characteristics = holy_cpu_to_le32_compile_time (
						  holy_PE32_SCN_CNT_CODE
						| holy_PE32_SCN_MEM_EXECUTE
						| holy_PE32_SCN_MEM_READ);

	data_section = text_section + 1;
	strcpy (data_section->name, ".data");
	data_section->virtual_size = holy_cpu_to_le32 (layout.kernel_size - layout.exec_size);
	data_section->virtual_address = holy_cpu_to_le32 (header_size + layout.exec_size);
	data_section->raw_data_size = holy_cpu_to_le32 (layout.kernel_size - layout.exec_size);
	data_section->raw_data_offset = holy_cpu_to_le32 (header_size + layout.exec_size);
	data_section->characteristics
	  = holy_cpu_to_le32_compile_time (holy_PE32_SCN_CNT_INITIALIZED_DATA
			      | holy_PE32_SCN_MEM_READ
			      | holy_PE32_SCN_MEM_WRITE);

#if 0
	bss_section = data_section + 1;
	strcpy (bss_section->name, ".bss");
	bss_section->virtual_size = holy_cpu_to_le32 (layout.bss_size);
	bss_section->virtual_address = holy_cpu_to_le32 (header_size + layout.kernel_size);
	bss_section->raw_data_size = 0;
	bss_section->raw_data_offset = 0;
	bss_section->characteristics
	  = holy_cpu_to_le32_compile_time (holy_PE32_SCN_MEM_READ
			      | holy_PE32_SCN_MEM_WRITE
			      | holy_PE32_SCN_ALIGN_64BYTES
			      | holy_PE32_SCN_CNT_INITIALIZED_DATA
			      | 0x80);
#endif
    
	mods_section = data_section + 1;
	strcpy (mods_section->name, "mods");
	mods_section->virtual_size = holy_cpu_to_le32 (reloc_addr - layout.kernel_size - header_size);
	mods_section->virtual_address = holy_cpu_to_le32 (header_size + layout.kernel_size + layout.bss_size);
	mods_section->raw_data_size = holy_cpu_to_le32 (reloc_addr - layout.kernel_size - header_size);
	mods_section->raw_data_offset = holy_cpu_to_le32 (header_size + layout.kernel_size);
	mods_section->characteristics
	  = holy_cpu_to_le32_compile_time (holy_PE32_SCN_CNT_INITIALIZED_DATA
			      | holy_PE32_SCN_MEM_READ
			      | holy_PE32_SCN_MEM_WRITE);

	reloc_section = mods_section + 1;
	strcpy (reloc_section->name, ".reloc");
	reloc_section->virtual_size = holy_cpu_to_le32 (layout.reloc_size);
	reloc_section->virtual_address = holy_cpu_to_le32 (reloc_addr + layout.bss_size);
	reloc_section->raw_data_size = holy_cpu_to_le32 (layout.reloc_size);
	reloc_section->raw_data_offset = holy_cpu_to_le32 (reloc_addr);
	reloc_section->characteristics
	  = holy_cpu_to_le32_compile_time (holy_PE32_SCN_CNT_INITIALIZED_DATA
			      | holy_PE32_SCN_MEM_DISCARDABLE
			      | holy_PE32_SCN_MEM_READ);
	free (core_img);
	core_img = pe_img;
	core_size = pe_size;
      }
      break;
    case IMAGE_QEMU:
      {
	char *rom_img;
	size_t rom_size;
	char *boot_path, *boot_img;
	size_t boot_size;

	boot_path = holy_util_get_path (dir, "boot.img");
	boot_size = holy_util_get_image_size (boot_path);
	boot_img = holy_util_read_image (boot_path);

	/* Rom sizes must be 64k-aligned.  */
	rom_size = ALIGN_UP (core_size + boot_size, 64 * 1024);

	rom_img = xmalloc (rom_size);
	memset (rom_img, 0, rom_size);

	*((holy_int32_t *) (core_img + holy_KERNEL_I386_QEMU_CORE_ENTRY_ADDR))
	  = holy_host_to_target32 ((holy_uint32_t) -rom_size);

	memcpy (rom_img, core_img, core_size);

	*((holy_int32_t *) (boot_img + holy_BOOT_I386_QEMU_CORE_ENTRY_ADDR))
	  = holy_host_to_target32 ((holy_uint32_t) -rom_size);

	memcpy (rom_img + rom_size - boot_size, boot_img, boot_size);

	free (core_img);
	core_img = rom_img;
	core_size = rom_size;

	free (boot_img);
	free (boot_path);
      }
      break;
    case IMAGE_SPARC64_AOUT:
      {
	void *aout_img;
	size_t aout_size;
	struct holy_aout32_header *aout_head;

	aout_size = core_size + sizeof (*aout_head);
	aout_img = xmalloc (aout_size);
	aout_head = aout_img;
	holy_memset (aout_head, 0, sizeof (*aout_head));
	aout_head->a_midmag = holy_host_to_target32 ((AOUT_MID_SUN << 16)
						     | AOUT32_OMAGIC);
	aout_head->a_text = holy_host_to_target32 (core_size);
	aout_head->a_entry
	  = holy_host_to_target32 (holy_BOOT_SPARC64_IEEE1275_IMAGE_ADDRESS);
	memcpy ((char *) aout_img + sizeof (*aout_head), core_img, core_size);

	free (core_img);
	core_img = aout_img;
	core_size = aout_size;
      }
      break;
    case IMAGE_SPARC64_RAW:
      {
	unsigned int num;
	char *boot_path, *boot_img;
	size_t boot_size;

	num = ((core_size + holy_DISK_SECTOR_SIZE - 1) >> holy_DISK_SECTOR_BITS);
	num <<= holy_DISK_SECTOR_BITS;

	boot_path = holy_util_get_path (dir, "diskboot.img");
	boot_size = holy_util_get_image_size (boot_path);
	if (boot_size != holy_DISK_SECTOR_SIZE)
	  holy_util_error (_("diskboot.img size must be %u bytes"),
			   holy_DISK_SECTOR_SIZE);

	boot_img = holy_util_read_image (boot_path);

	*((holy_uint32_t *) (boot_img + holy_DISK_SECTOR_SIZE
			     - holy_BOOT_SPARC64_IEEE1275_LIST_SIZE + 8))
	  = holy_host_to_target32 (num);

	holy_util_write_image (boot_img, boot_size, out, outname);
	free (boot_img);
	free (boot_path);
      }
      break;
    case IMAGE_SPARC64_CDCORE:
      break;
    case IMAGE_YEELOONG_FLASH:
    case IMAGE_FULOONG2F_FLASH:
    {
      char *rom_img;
      size_t rom_size;
      char *boot_path, *boot_img;
      size_t boot_size;
      /* fwstart.img is the only part which can't be tested by using *-elf
	 target. Check it against the checksum. */
      const holy_uint8_t yeeloong_fwstart_good_hash[512 / 8] =
	{
	  0x5f, 0x67, 0x46, 0x57, 0x31, 0x30, 0xc5, 0x0a,
	  0xe9, 0x98, 0x18, 0xc9, 0xf3, 0xca, 0x45, 0xa5,
	  0x75, 0x64, 0x6b, 0xbb, 0x24, 0xcd, 0xb4, 0xbc,
	  0xf2, 0x3e, 0x23, 0xf9, 0xc2, 0x6a, 0x8c, 0xde,
	  0x3b, 0x94, 0x9c, 0xcc, 0xa5, 0xa7, 0x58, 0xb1,
	  0xbe, 0x8b, 0x3d, 0x73, 0x98, 0x18, 0x7e, 0x68,
	  0x5e, 0x5f, 0x23, 0x7d, 0x7a, 0xe8, 0x51, 0xf7,
	  0x1a, 0xaf, 0x2f, 0x54, 0x11, 0x2e, 0x5c, 0x25
	};
      const holy_uint8_t fuloong2f_fwstart_good_hash[512 / 8] =
	{ 
	  0x76, 0x9b, 0xad, 0x6e, 0xa2, 0x39, 0x47, 0x62,
	  0x1f, 0xc9, 0x3a, 0x6d, 0x05, 0x5c, 0x43, 0x5c,
	  0x29, 0x4a, 0x7e, 0x08, 0x2a, 0x31, 0x8f, 0x5d,
	  0x02, 0x84, 0xa0, 0x85, 0xf2, 0xd1, 0xb9, 0x53,
	  0xa2, 0xbc, 0xf2, 0xe1, 0x39, 0x1e, 0x51, 0xb5,
	  0xaf, 0xec, 0x9e, 0xf2, 0xf1, 0xf3, 0x0a, 0x2f,
	  0xe6, 0xf1, 0x08, 0x89, 0xbe, 0xbc, 0x73, 0xab,
	  0x46, 0x50, 0xd6, 0x21, 0xce, 0x8e, 0x24, 0xa7
	};
      const holy_uint8_t *fwstart_good_hash;
      holy_uint8_t fwstart_hash[512 / 8];
            
      if (image_target->id == IMAGE_FULOONG2F_FLASH)
	{
	  fwstart_good_hash = fuloong2f_fwstart_good_hash;
	  boot_path = holy_util_get_path (dir, "fwstart_fuloong2f.img");
	}
      else
	{
	  fwstart_good_hash = yeeloong_fwstart_good_hash;
	  boot_path = holy_util_get_path (dir, "fwstart.img");
	}

      boot_size = holy_util_get_image_size (boot_path);
      boot_img = holy_util_read_image (boot_path);

      holy_crypto_hash (holy_MD_SHA512, fwstart_hash, boot_img, boot_size);

      if (holy_memcmp (fwstart_hash, fwstart_good_hash,
		       holy_MD_SHA512->mdlen) != 0)
	/* TRANSLATORS: fwstart.img may still be good, just it wasn't checked.  */
	holy_util_warn ("%s",
			_("fwstart.img doesn't match the known good version. "
			  "proceed at your own risk"));

      if (core_size + boot_size > 512 * 1024)
	holy_util_error ("%s", _("firmware image is too big"));
      rom_size = 512 * 1024;

      rom_img = xmalloc (rom_size);
      memset (rom_img, 0, rom_size); 

      memcpy (rom_img, boot_img, boot_size);

      memcpy (rom_img + boot_size, core_img, core_size);

      memset (rom_img + boot_size + core_size, 0,
	      rom_size - (boot_size + core_size));

      free (core_img);
      core_img = rom_img;
      core_size = rom_size;
      free (boot_img);
      free (boot_path);
    }
    break;
    case IMAGE_QEMU_MIPS_FLASH:
    {
      char *rom_img;
      size_t rom_size;

      if (core_size > 512 * 1024)
	holy_util_error ("%s", _("firmware image is too big"));
      rom_size = 512 * 1024;

      rom_img = xmalloc (rom_size);
      memset (rom_img, 0, rom_size); 

      memcpy (rom_img, core_img, core_size);

      memset (rom_img + core_size, 0,
	      rom_size - core_size);

      free (core_img);
      core_img = rom_img;
      core_size = rom_size;
    }
    break;

    case IMAGE_UBOOT:
    {
      struct holy_uboot_image_header *hdr;

      hdr = xmalloc (core_size + sizeof (struct holy_uboot_image_header));
      memcpy (hdr + 1, core_img, core_size);

      memset (hdr, 0, sizeof (*hdr));
      hdr->ih_magic = holy_cpu_to_be32_compile_time (holy_UBOOT_IH_MAGIC);
      hdr->ih_time = holy_cpu_to_be32 (STABLE_EMBEDDING_TIMESTAMP);
      hdr->ih_size = holy_cpu_to_be32 (core_size);
      hdr->ih_load = 0;
      hdr->ih_ep = 0;
      hdr->ih_type = holy_UBOOT_IH_TYPE_KERNEL_NOLOAD;
      hdr->ih_os = holy_UBOOT_IH_OS_LINUX;
      hdr->ih_arch = holy_UBOOT_IH_ARCH_ARM;
      hdr->ih_comp = holy_UBOOT_IH_COMP_NONE;

      holy_crypto_hash (holy_MD_CRC32, &hdr->ih_dcrc, hdr + 1, core_size);
      holy_crypto_hash (holy_MD_CRC32, &hdr->ih_hcrc, hdr, sizeof (*hdr));

      free (core_img);
      core_img = (char *) hdr;
      core_size += sizeof (struct holy_uboot_image_header);
    }
    break;

    case IMAGE_MIPS_ARC:
      {
	char *ecoff_img;
	struct ecoff_header {
	  holy_uint16_t magic;
	  holy_uint16_t nsec;
	  holy_uint32_t time;
	  holy_uint32_t syms;
	  holy_uint32_t nsyms;
	  holy_uint16_t opt;
	  holy_uint16_t flags;
	  holy_uint16_t magic2;
	  holy_uint16_t version;
	  holy_uint32_t textsize;
	  holy_uint32_t datasize;
	  holy_uint32_t bsssize;
	  holy_uint32_t entry;
	  holy_uint32_t text_start;
	  holy_uint32_t data_start;
	  holy_uint32_t bss_start;
	  holy_uint32_t gprmask;
	  holy_uint32_t cprmask[4];
	  holy_uint32_t gp_value;
	};
	struct ecoff_section
	{
	  char name[8];
	  holy_uint32_t paddr;
	  holy_uint32_t vaddr;
	  holy_uint32_t size;
	  holy_uint32_t file_offset;
	  holy_uint32_t reloc;
	  holy_uint32_t gp;
	  holy_uint16_t nreloc;
	  holy_uint16_t ngp;
	  holy_uint32_t flags;
	};
	struct ecoff_header *head;
	struct ecoff_section *section;
	holy_uint32_t target_addr;
	size_t program_size;

	program_size = ALIGN_ADDR (core_size);
	if (comp == holy_COMPRESSION_NONE)
	  target_addr = (image_target->link_addr - decompress_size);
	else
	  target_addr = ALIGN_UP (image_target->link_addr
				  + layout.kernel_size + total_module_size, 32);

	ecoff_img = xmalloc (program_size + sizeof (*head) + sizeof (*section));
	holy_memset (ecoff_img, 0, program_size + sizeof (*head) + sizeof (*section));
	head = (void *) ecoff_img;
	section = (void *) (head + 1);
	head->magic = image_target->bigendian ? holy_host_to_target16 (0x160)
	  : holy_host_to_target16 (0x166);
	head->nsec = holy_host_to_target16 (1);
	head->time = holy_host_to_target32 (0);
	head->opt = holy_host_to_target16 (0x38);
	head->flags = image_target->bigendian
	  ? holy_host_to_target16 (0x207)
	  : holy_host_to_target16 (0x103);
	head->magic2 = holy_host_to_target16 (0x107);
	head->textsize = holy_host_to_target32 (program_size);
	head->entry = holy_host_to_target32 (target_addr);
	head->text_start = holy_host_to_target32 (target_addr);
	head->data_start = holy_host_to_target32 (target_addr + program_size);
	holy_memcpy (section->name, ".text", sizeof (".text") - 1);
	section->vaddr = holy_host_to_target32 (target_addr);
	section->size = holy_host_to_target32 (program_size);
	section->file_offset = holy_host_to_target32 (sizeof (*head) + sizeof (*section));
	if (!image_target->bigendian)
	  {
	    section->paddr = holy_host_to_target32 (0xaa60);
	    section->flags = holy_host_to_target32 (0x20);
	  }
	memcpy (section + 1, core_img, core_size);
	free (core_img);
	core_img = ecoff_img;
	core_size = program_size + sizeof (*head) + sizeof (*section);
      }
      break;
    case IMAGE_LOONGSON_ELF:
    case IMAGE_PPC:
    case IMAGE_XEN:
    case IMAGE_COREBOOT:
    case IMAGE_I386_IEEE1275:
      {
	holy_uint64_t target_addr;
	if (image_target->id == IMAGE_LOONGSON_ELF)
	  {
	    if (comp == holy_COMPRESSION_NONE)
	      target_addr = (image_target->link_addr - decompress_size);
	    else
	      target_addr = ALIGN_UP (image_target->link_addr
				      + layout.kernel_size + total_module_size, 32);
	  }
	else
	  target_addr = image_target->link_addr;
	if (image_target->voidp_sizeof == 4)
	  holy_mkimage_generate_elf32 (image_target, note, &core_img, &core_size,
				       target_addr, layout.align, layout.kernel_size, layout.bss_size);
	else
	  holy_mkimage_generate_elf64 (image_target, note, &core_img, &core_size,
				       target_addr, layout.align, layout.kernel_size, layout.bss_size);
      }
      break;
    }

  holy_util_write_image (core_img, core_size, out, outname);
  free (core_img);
  free (kernel_path);
  free (layout.reloc_section);

  holy_util_free_path_list (path_list);
}
