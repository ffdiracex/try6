/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_LINUX_CPU_HEADER
#define holy_LINUX_CPU_HEADER 1

#define LINUX_ZIMAGE_OFFSET 0x24
#define LINUX_ZIMAGE_MAGIC  0x016f2818

#include "system.h"

#if defined holy_MACHINE_UBOOT
# include <holy/uboot/uboot.h>
# define LINUX_ADDRESS        (start_of_ram + 0x8000)
# define LINUX_INITRD_ADDRESS (start_of_ram + 0x02000000)
# define LINUX_FDT_ADDRESS    (LINUX_INITRD_ADDRESS - 0x10000)
# define holy_arm_firmware_get_boot_data holy_uboot_get_boot_data
# define holy_arm_firmware_get_machine_type holy_uboot_get_machine_type
#elif defined holy_MACHINE_EFI
# include <holy/efi/efi.h>
# include <holy/machine/loader.h>
/* On UEFI platforms - load the images at the lowest available address not
   less than *_PHYS_OFFSET from the first available memory location. */
# define LINUX_PHYS_OFFSET        (0x00008000)
# define LINUX_INITRD_PHYS_OFFSET (LINUX_PHYS_OFFSET + 0x02000000)
# define LINUX_FDT_PHYS_OFFSET    (LINUX_INITRD_PHYS_OFFSET - 0x10000)
# define holy_arm_firmware_get_boot_data (holy_addr_t)holy_efi_get_firmware_fdt
static inline holy_uint32_t
holy_arm_firmware_get_machine_type (void)
{
  return holy_ARM_MACHINE_TYPE_FDT;
}
#endif

#define FDT_ADDITIONAL_ENTRIES_SIZE	0x300

#endif /* ! holy_LINUX_CPU_HEADER */
