/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/net/netbuff.h>
#include <holy/net.h>
#include <holy/term.h>
#include <holy/i18n.h>
#include <holy/emu/net.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t
send_card_buffer (struct holy_net_card *dev __attribute__ ((unused)),
		  struct holy_net_buff *pack);

static struct holy_net_buff *
get_card_packet (struct holy_net_card *dev __attribute__ ((unused)));

static struct holy_net_card_driver emudriver =
  {
    .name = "emu",
    .send = send_card_buffer,
    .recv = get_card_packet
  };

static struct holy_net_card emucard =
  {
    .name = "emu0",
    .driver = &emudriver,
    .mtu = 1500,
    .default_address = {
			 .type = holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET,
			 {.mac = {0, 1, 2, 3, 4, 5}}
		       },
    .flags = 0
  };

static holy_err_t
send_card_buffer (struct holy_net_card *dev __attribute__ ((unused)),
		  struct holy_net_buff *pack)
{
  holy_ssize_t actual;

  actual = holy_emunet_send (pack->data, pack->tail - pack->data);
  if (actual < 0)
    return holy_error (holy_ERR_IO, N_("couldn't send network packet"));

  return holy_ERR_NONE;
}

static struct holy_net_buff *
get_card_packet (struct holy_net_card *dev __attribute__ ((unused)))
{
  holy_ssize_t actual;
  struct holy_net_buff *nb;

  nb = holy_netbuff_alloc (emucard.mtu + 36 + 2);
  if (!nb)
    return NULL;

  /* Reserve 2 bytes so that 2 + 14/18 bytes of ethernet header is divisible
     by 4. So that IP header is aligned on 4 bytes. */
  holy_netbuff_reserve (nb, 2);
  if (!nb)
    {
      holy_netbuff_free (nb);
      return NULL;
    }

  actual = holy_emunet_receive (nb->data, emucard.mtu + 36);
  if (actual < 0)
    {
      holy_netbuff_free (nb);
      return NULL;
    }
  holy_netbuff_put (nb, actual);

  return nb;
}

static int registered = 0;

holy_MOD_INIT(emunet)
{
  if (!holy_emunet_create (&emucard.mtu))
    {
      holy_net_card_register (&emucard);
      registered = 1;
    }
}

holy_MOD_FINI(emunet)
{
  if (registered)
    {
      holy_emunet_close ();
      holy_net_card_unregister (&emucard);
      registered = 0;
    }
}
