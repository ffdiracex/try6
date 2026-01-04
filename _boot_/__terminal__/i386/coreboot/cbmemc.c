/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/term.h>
#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/time.h>
#include <holy/terminfo.h>
#include <holy/dl.h>
#include <holy/i386/coreboot/lbio.h>
#include <holy/command.h>
#include <holy/normal.h>

holy_MOD_LICENSE ("GPLv2+");

struct holy_linuxbios_cbmemc
{
  holy_uint32_t size;
  holy_uint32_t pointer;
  char data[0];
};

static struct holy_linuxbios_cbmemc *cbmemc;

static void
put (struct holy_term_output *term __attribute__ ((unused)), const int c)
{
  if (!cbmemc)
    return;
  if (cbmemc->pointer < cbmemc->size)
    cbmemc->data[cbmemc->pointer] = c;
  cbmemc->pointer++;
}

struct holy_terminfo_output_state holy_cbmemc_terminfo_output =
  {
    .put = put,
    .size = { 80, 24 }
  };

static struct holy_term_output holy_cbmemc_term_output =
  {
    .name = "cbmemc",
    .init = holy_terminfo_output_init,
    .fini = 0,
    .putchar = holy_terminfo_putchar,
    .getxy = holy_terminfo_getxy,
    .getwh = holy_terminfo_getwh,
    .gotoxy = holy_terminfo_gotoxy,
    .cls = holy_terminfo_cls,
    .setcolorstate = holy_terminfo_setcolorstate,
    .setcursor = holy_terminfo_setcursor,
    .flags = holy_TERM_CODE_TYPE_ASCII,
    .data = &holy_cbmemc_terminfo_output,
    .progress_update_divisor = holy_PROGRESS_NO_UPDATE
  };

static int
iterate_linuxbios_table (holy_linuxbios_table_item_t table_item,
			 void *data __attribute__ ((unused)))
{
  if (table_item->tag != holy_LINUXBIOS_MEMBER_CBMEMC)
    return 0;
  cbmemc = (struct holy_linuxbios_cbmemc *) (holy_addr_t)
    *(holy_uint64_t *) (table_item + 1);
  return 1;
}

static holy_err_t
holy_cmd_cbmemc (struct holy_command *cmd __attribute__ ((unused)),
		 int argc __attribute__ ((unused)),
		 char *argv[] __attribute__ ((unused)))
{
  holy_size_t len;
  char *str;
  struct holy_linuxbios_cbmemc *cbmemc_saved;

  if (!cbmemc)
    return holy_error (holy_ERR_IO, "no CBMEM console found");

  len = cbmemc->pointer;
  if (len > cbmemc->size)
    len = cbmemc->size;
  str = cbmemc->data;
  cbmemc_saved = cbmemc;
  cbmemc = 0;
  holy_xnputs (str, len);
  cbmemc = cbmemc_saved;
  return 0;
}

static holy_command_t cmd;

holy_MOD_INIT (cbmemc)
{
  holy_linuxbios_table_iterate (iterate_linuxbios_table, 0);

  if (cbmemc)
    holy_term_register_output ("cbmemc", &holy_cbmemc_term_output);

  cmd =
    holy_register_command ("cbmemc", holy_cmd_cbmemc,
			   0, N_("Show CBMEM console content."));
}


holy_MOD_FINI (cbmemc)
{
  holy_term_unregister_output (&holy_cbmemc_term_output);
  holy_unregister_command (cmd);
}
