/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/i386/coreboot/lbio.h>
#include <holy/i386/tsc.h>

holy_MOD_LICENSE ("GPLv2+");

static const char *console_descs[] = {
  "8250 UART",
  "VGA",
  "BTEXT",
  "log buffer console",
  "SROM",
  "EHCI debug",
  "memory-mapped 8250 UART"
};

static const char *descs[] = {
  [holy_LINUXBIOS_MEMBER_MEMORY] = "memory map (`lsmmap' to list)",
  [holy_LINUXBIOS_MEMBER_MAINBOARD] = "mainboard",
  [4] = "version",
  [5] = "extra version",
  [6] = "build",
  [7] = "compile time",
  [8] = "compile by",
  [9] = "compile host",
  [0xa] = "compile domain",
  [0xb] = "compiler",
  [0xc] = "linker",
  [0xd] = "assembler",
  [0xf] = "serial",
  [holy_LINUXBIOS_MEMBER_CONSOLE] = "console",
  [holy_LINUXBIOS_MEMBER_FRAMEBUFFER] = "framebuffer",
  [0x13] = "GPIO",
  [0x15] = "VDAT",
  [holy_LINUXBIOS_MEMBER_TIMESTAMPS] = "timestamps (`coreboot_boottime' to list)",
  [holy_LINUXBIOS_MEMBER_CBMEMC] = "CBMEM console (`cbmemc' to list)",
  [0x18] = "MRC cache",
  [0x19] = "VBNV",
  [0xc8] = "CMOS option table",
  [0xc9] = "CMOS option",
  [0xca] = "CMOS option enum",
  [0xcb] = "CMOS option defaults",
  [0xcc] = "CMOS checksum",
};

static int
iterate_linuxbios_table (holy_linuxbios_table_item_t table_item,
			 void *data __attribute__ ((unused)))
{
  if (table_item->tag < ARRAY_SIZE (descs) && descs[table_item->tag])
    holy_printf ("tag=%02x size=%02x %s",
		 table_item->tag, table_item->size, descs[table_item->tag]);
  else
    holy_printf ("tag=%02x size=%02x",
		 table_item->tag, table_item->size);

  switch (table_item->tag)
    {
    case holy_LINUXBIOS_MEMBER_FRAMEBUFFER:
      {
	struct holy_linuxbios_table_framebuffer *fb;
	fb = (struct holy_linuxbios_table_framebuffer *) (table_item + 1);

	holy_printf (": %dx%dx%d pitch=%d lfb=0x%llx %d/%d/%d/%d %d/%d/%d/%d",
		     fb->width, fb->height,
		     fb->bpp, fb->pitch, 
		     (unsigned long long) fb->lfb,
		     fb->red_mask_size, fb->green_mask_size,
		     fb->blue_mask_size, fb->reserved_mask_size,
		     fb->red_field_pos, fb->green_field_pos,
		     fb->blue_field_pos, fb->reserved_field_pos);
	break;
      }
    case holy_LINUXBIOS_MEMBER_MAINBOARD:
      {
	struct holy_linuxbios_mainboard *mb;
	mb = (struct holy_linuxbios_mainboard *) (table_item + 1);
	holy_printf (": vendor=`%s' part_number=`%s'",
		     mb->strings + mb->vendor,
		     mb->strings + mb->part_number);
	break;
      }
    case 0x04 ... 0x0d:
      holy_printf (": `%s'", (char *) (table_item + 1));
      break;
    case holy_LINUXBIOS_MEMBER_CONSOLE:
      {
	holy_uint16_t *val = (holy_uint16_t *) (table_item + 1);
	holy_printf (": id=%d", *val);
	if (*val < ARRAY_SIZE (console_descs)
	    && console_descs[*val])
	  holy_printf (" %s", console_descs[*val]);
      }
    }
  holy_printf ("\n");

  return 0;
}


static holy_err_t
holy_cmd_lscoreboot (struct holy_command *cmd __attribute__ ((unused)),
			    int argc __attribute__ ((unused)),
			    char *argv[] __attribute__ ((unused)))
{
  holy_linuxbios_table_iterate (iterate_linuxbios_table, 0);
  return 0;
}

static holy_command_t cmd;

holy_MOD_INIT(cbls)
{
  cmd =
    holy_register_command ("lscoreboot", holy_cmd_lscoreboot,
			   0, N_("List coreboot tables."));
}

holy_MOD_FINI(cbls)
{
  holy_unregister_command (cmd);
}
