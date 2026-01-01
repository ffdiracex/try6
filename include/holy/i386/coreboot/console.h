/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MACHINE_CONSOLE_HEADER
#define holy_MACHINE_CONSOLE_HEADER	1

void holy_vga_text_init (void);
void holy_vga_text_fini (void);

void holy_video_coreboot_fb_init (void);
void holy_video_coreboot_fb_early_init (void);
void holy_video_coreboot_fb_late_init (void);
void holy_video_coreboot_fb_fini (void);

extern struct holy_linuxbios_table_framebuffer *holy_video_coreboot_fbtable;

#endif /* ! holy_MACHINE_CONSOLE_HEADER */
