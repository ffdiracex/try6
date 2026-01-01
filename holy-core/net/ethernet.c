/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/net/ethernet.h>
#include <holy/net/ip.h>
#include <holy/net/arp.h>
#include <holy/net/netbuff.h>
#include <holy/net.h>
#include <holy/time.h>
#include <holy/net/arp.h>

#define LLCADDRMASK 0x7f

struct etherhdr
{
  holy_uint8_t dst[6];
  holy_uint8_t src[6];
  holy_uint16_t type;
} holy_PACKED;

struct llchdr
{
  holy_uint8_t dsap;
  holy_uint8_t ssap;
  holy_uint8_t ctrl;
} holy_PACKED;

struct snaphdr
{
  holy_uint8_t oui[3];
  holy_uint16_t type;
} holy_PACKED;

holy_err_t
send_ethernet_packet (struct holy_net_network_level_interface *inf,
		      struct holy_net_buff *nb,
		      holy_net_link_level_address_t target_addr,
		      holy_net_ethertype_t ethertype)
{
  struct etherhdr *eth;
  holy_err_t err;

  COMPILE_TIME_ASSERT (sizeof (*eth) < holy_NET_MAX_LINK_HEADER_SIZE);

  err = holy_netbuff_push (nb, sizeof (*eth));
  if (err)
    return err;
  eth = (struct etherhdr *) nb->data;
  holy_memcpy (eth->dst, target_addr.mac, 6);
  holy_memcpy (eth->src, inf->hwaddress.mac, 6);

  eth->type = holy_cpu_to_be16 (ethertype);
  if (!inf->card->opened)
    {
      err = holy_ERR_NONE;
      if (inf->card->driver->open)
	err = inf->card->driver->open (inf->card);
      if (err)
	return err;
      inf->card->opened = 1;
    }
  return inf->card->driver->send (inf->card, nb);
}

holy_err_t
holy_net_recv_ethernet_packet (struct holy_net_buff *nb,
			       struct holy_net_card *card)
{
  struct etherhdr *eth;
  struct llchdr *llch;
  struct snaphdr *snaph;
  holy_net_ethertype_t type;
  holy_net_link_level_address_t hwaddress;
  holy_net_link_level_address_t src_hwaddress;
  holy_err_t err;

  eth = (struct etherhdr *) nb->data;
  type = holy_be_to_cpu16 (eth->type);
  err = holy_netbuff_pull (nb, sizeof (*eth));
  if (err)
    return err;

  if (type <= 1500)
    {
      llch = (struct llchdr *) nb->data;
      type = llch->dsap & LLCADDRMASK;

      if (llch->dsap == 0xaa && llch->ssap == 0xaa && llch->ctrl == 0x3)
	{
	  err = holy_netbuff_pull (nb, sizeof (*llch));
	  if (err)
	    return err;
	  snaph = (struct snaphdr *) nb->data;
	  type = snaph->type;
	}
    }

  hwaddress.type = holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
  holy_memcpy (hwaddress.mac, eth->dst, sizeof (hwaddress.mac));
  src_hwaddress.type = holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
  holy_memcpy (src_hwaddress.mac, eth->src, sizeof (src_hwaddress.mac));

  switch (type)
    {
      /* ARP packet. */
    case holy_NET_ETHERTYPE_ARP:
      holy_net_arp_receive (nb, card);
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
      /* IP packet.  */
    case holy_NET_ETHERTYPE_IP:
    case holy_NET_ETHERTYPE_IP6:
      return holy_net_recv_ip_packets (nb, card, &hwaddress, &src_hwaddress);
    }
  holy_netbuff_free (nb);
  return holy_ERR_NONE;
}
