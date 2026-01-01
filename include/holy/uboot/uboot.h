/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_UBOOT_UBOOT_HEADER
#define holy_UBOOT_UBOOT_HEADER	1

#include <holy/types.h>
#include <holy/dl.h>

/* Functions.  */
void holy_uboot_mm_init (void);
void holy_uboot_init (void);
void holy_uboot_fini (void);

void holy_uboot_return (int) __attribute__ ((noreturn));

holy_addr_t holy_uboot_get_real_bss_start (void);

holy_err_t holy_uboot_probe_hardware (void);

extern holy_addr_t EXPORT_VAR (start_of_ram);

holy_uint32_t EXPORT_FUNC (holy_uboot_get_machine_type) (void);
holy_addr_t EXPORT_FUNC (holy_uboot_get_boot_data) (void);


/*
 * The U-Boot API operates through a "syscall" interface, consisting of an
 * entry point address and a set of syscall numbers. The location of this
 * entry point is described in a structure allocated on the U-Boot heap.
 * We scan through a defined region around the hint address passed to us
 * from U-Boot.
 */

#define UBOOT_API_SEARCH_LEN (3 * 1024 * 1024)
int holy_uboot_api_init (void);

/*
 * All functions below are wrappers around the uboot_syscall() function,
 * implemented in holy-core/kern/uboot/uboot.c
*/

int  holy_uboot_getc (void);
int  holy_uboot_tstc (void);
void holy_uboot_putc (int c);
void holy_uboot_puts (const char *s);

void EXPORT_FUNC (holy_uboot_reset) (void);

struct sys_info *holy_uboot_get_sys_info (void);

void holy_uboot_udelay (holy_uint32_t usec);
holy_uint32_t holy_uboot_get_timer (holy_uint32_t base);

int EXPORT_FUNC (holy_uboot_dev_enum) (void);
struct device_info * EXPORT_FUNC (holy_uboot_dev_get) (int index);
int EXPORT_FUNC (holy_uboot_dev_open) (struct device_info *dev);
int EXPORT_FUNC (holy_uboot_dev_close) (struct device_info *dev);
int holy_uboot_dev_write (struct device_info *dev, void *buf, int *len);
int holy_uboot_dev_read (struct device_info *dev, void *buf, holy_size_t blocks,
			 holy_uint32_t start, holy_size_t * real_blocks);
int EXPORT_FUNC (holy_uboot_dev_recv) (struct device_info *dev, void *buf,
				       int size, int *real_size);
int EXPORT_FUNC (holy_uboot_dev_send) (struct device_info *dev, void *buf,
				       int size);

char *holy_uboot_env_get (const char *name);
void holy_uboot_env_set (const char *name, const char *value);

#endif /* ! holy_UBOOT_UBOOT_HEADER */
