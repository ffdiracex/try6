/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <config-util.h>

#include <holy/i18n.h>
#include <holy/emu/net.h>

holy_ssize_t
holy_emunet_send (const void *packet __attribute__ ((unused)),
		  holy_size_t sz __attribute__ ((unused)))
{
  return -1;
}

holy_ssize_t
holy_emunet_receive (void *packet __attribute__ ((unused)),
		     holy_size_t sz __attribute__ ((unused)))
{
  return -1;
}

int
holy_emunet_create (holy_size_t *mtu)
{
  *mtu = 1500;
  return -1;
}

void
holy_emunet_close (void)
{
  return;
}
