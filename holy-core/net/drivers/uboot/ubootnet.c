/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/net/netbuff.h>
#include <holy/uboot/disk.h>
#include <holy/uboot/uboot.h>
#include <holy/uboot/api_public.h>
#include <holy/dl.h>
#include <holy/net.h>
#include <holy/time.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t
card_open (struct holy_net_card *dev)
{
  int status;

  status = holy_uboot_dev_open (dev->data);
  if (status)
    return holy_error (holy_ERR_IO, "Couldn't open network card.");

  return holy_ERR_NONE;
}

static void
card_close (struct holy_net_card *dev)
{
  holy_uboot_dev_close (dev->data);
}

static holy_err_t
send_card_buffer (struct holy_net_card *dev, struct holy_net_buff *pack)
{
  int status;
  holy_size_t len;

  len = (pack->tail - pack->data);
  if (len > dev->mtu)
    len = dev->mtu;

  holy_memcpy (dev->txbuf, pack->data, len);
  status = holy_uboot_dev_send (dev->data, dev->txbuf, len);

  if (status)
    return holy_error (holy_ERR_IO, N_("couldn't send network packet"));
  return holy_ERR_NONE;
}

static struct holy_net_buff *
get_card_packet (struct holy_net_card *dev)
{
  int rc;
  holy_uint64_t start_time;
  struct holy_net_buff *nb;
  int actual;

  nb = holy_netbuff_alloc (dev->mtu + 64 + 2);
  if (!nb)
    return NULL;
  /* Reserve 2 bytes so that 2 + 14/18 bytes of ethernet header is divisible
     by 4. So that IP header is aligned on 4 bytes. */
  holy_netbuff_reserve (nb, 2);

  start_time = holy_get_time_ms ();
  do
    {
      rc = holy_uboot_dev_recv (dev->data, nb->data, dev->mtu + 64, &actual);
      holy_dprintf ("net", "rc=%d, actual=%d, time=%lld\n", rc, actual,
		    holy_get_time_ms () - start_time);
    }
  while ((actual <= 0 || rc < 0) && (holy_get_time_ms () - start_time < 200));
  if (actual > 0)
    {
      holy_netbuff_put (nb, actual);
      return nb;
    }
  holy_netbuff_free (nb);
  return NULL;
}

static struct holy_net_card_driver ubootnet =
  {
    .name = "ubnet",
    .open = card_open,
    .close = card_close,
    .send = send_card_buffer,
    .recv = get_card_packet
  };

holy_MOD_INIT (ubootnet)
{
  int devcount, i;
  int nfound = 0;

  devcount = holy_uboot_dev_enum ();

  for (i = 0; i < devcount; i++)
    {
      struct device_info *devinfo = holy_uboot_dev_get (i);
      struct holy_net_card *card;

      if (!(devinfo->type & DEV_TYP_NET))
	continue;

      card = holy_zalloc (sizeof (struct holy_net_card));
      if (!card)
	{
	  holy_print_error ();
	  return;
	}

      /* FIXME: Any way to check this?  */
      card->mtu = 1500;

      holy_memcpy (&(card->default_address.mac), &devinfo->di_net.hwaddr, 6);
      card->default_address.type = holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET;

      card->txbufsize = ALIGN_UP (card->mtu, 64) + 256;
      card->txbuf = holy_zalloc (card->txbufsize);
      if (!card->txbuf)
	{
	  holy_free (card);
	  holy_print_error ();
	  continue;
	}

      card->data = devinfo;
      card->flags = 0;
      card->name = holy_xasprintf ("ubnet_%d", ++nfound);
      card->idle_poll_delay_ms = 10;

      card->driver = &ubootnet;
      holy_net_card_register (card);
    }
}

holy_MOD_FINI (ubootnet)
{
  struct holy_net_card *card, *next;

  FOR_NET_CARDS_SAFE (card, next) 
    if (card->driver && holy_strcmp (card->driver->name, "ubnet") == 0)
      holy_net_card_unregister (card);
}
