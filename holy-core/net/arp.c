/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/net/arp.h>
#include <holy/net/netbuff.h>
#include <holy/mm.h>
#include <holy/net.h>
#include <holy/net/ethernet.h>
#include <holy/net/ip.h>
#include <holy/time.h>

/* ARP header operation codes */
enum
  {
    ARP_REQUEST = 1,
    ARP_REPLY = 2
  };

enum
  {
    /* IANA ARP constant to define hardware type as ethernet. */
    holy_NET_ARPHRD_ETHERNET = 1
  };

struct arppkt {
  holy_uint16_t hrd;
  holy_uint16_t pro;
  holy_uint8_t hln;
  holy_uint8_t pln;
  holy_uint16_t op;
  holy_uint8_t sender_mac[6];
  holy_uint32_t sender_ip;
  holy_uint8_t recv_mac[6];
  holy_uint32_t recv_ip;
} holy_PACKED;

static int have_pending;
static holy_uint32_t pending_req;

holy_err_t
holy_net_arp_send_request (struct holy_net_network_level_interface *inf,
			   const holy_net_network_level_address_t *proto_addr)
{
  struct holy_net_buff nb;
  struct arppkt *arp_packet;
  holy_net_link_level_address_t target_mac_addr;
  holy_err_t err;
  int i;
  holy_uint8_t *nbd;
  holy_uint8_t arp_data[128];

  if (proto_addr->type != holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4)
    return holy_error (holy_ERR_BUG, "unsupported address family");

  /* Build a request packet.  */
  nb.head = arp_data;
  nb.end = arp_data + sizeof (arp_data);
  holy_netbuff_clear (&nb);
  holy_netbuff_reserve (&nb, 128);

  err = holy_netbuff_push (&nb, sizeof (*arp_packet));
  if (err)
    return err;

  arp_packet = (struct arppkt *) nb.data;
  arp_packet->hrd = holy_cpu_to_be16_compile_time (holy_NET_ARPHRD_ETHERNET);
  arp_packet->hln = 6;
  arp_packet->pro = holy_cpu_to_be16_compile_time (holy_NET_ETHERTYPE_IP);
  arp_packet->pln = 4;
  arp_packet->op = holy_cpu_to_be16_compile_time (ARP_REQUEST);
  /* Sender hardware address.  */
  holy_memcpy (arp_packet->sender_mac, &inf->hwaddress.mac, 6);
  arp_packet->sender_ip = inf->address.ipv4;
  holy_memset (arp_packet->recv_mac, 0, 6);
  arp_packet->recv_ip = proto_addr->ipv4;
  /* Target protocol address */
  holy_memset (&target_mac_addr.mac, 0xff, 6);

  nbd = nb.data;
  send_ethernet_packet (inf, &nb, target_mac_addr, holy_NET_ETHERTYPE_ARP);
  for (i = 0; i < holy_NET_TRIES; i++)
    {
      if (holy_net_link_layer_resolve_check (inf, proto_addr))
	return holy_ERR_NONE;
      pending_req = proto_addr->ipv4;
      have_pending = 0;
      holy_net_poll_cards (holy_NET_INTERVAL + (i * holy_NET_INTERVAL_ADDITION),
                           &have_pending);
      if (holy_net_link_layer_resolve_check (inf, proto_addr))
	return holy_ERR_NONE;
      nb.data = nbd;
      send_ethernet_packet (inf, &nb, target_mac_addr, holy_NET_ETHERTYPE_ARP);
    }

  return holy_ERR_NONE;
}

holy_err_t
holy_net_arp_receive (struct holy_net_buff *nb,
		      struct holy_net_card *card)
{
  struct arppkt *arp_packet = (struct arppkt *) nb->data;
  holy_net_network_level_address_t sender_addr, target_addr;
  holy_net_link_level_address_t sender_mac_addr;
  struct holy_net_network_level_interface *inf;

  if (arp_packet->pro != holy_cpu_to_be16_compile_time (holy_NET_ETHERTYPE_IP)
      || arp_packet->pln != 4 || arp_packet->hln != 6
      || nb->tail - nb->data < (int) sizeof (*arp_packet))
    return holy_ERR_NONE;

  sender_addr.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
  target_addr.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
  sender_addr.ipv4 = arp_packet->sender_ip;
  target_addr.ipv4 = arp_packet->recv_ip;
  if (arp_packet->sender_ip == pending_req)
    have_pending = 1;

  sender_mac_addr.type = holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
  holy_memcpy (sender_mac_addr.mac, arp_packet->sender_mac,
	       sizeof (sender_mac_addr.mac));
  holy_net_link_layer_add_address (card, &sender_addr, &sender_mac_addr, 1);

  FOR_NET_NETWORK_LEVEL_INTERFACES (inf)
  {
    /* Am I the protocol address target? */
    if (holy_net_addr_cmp (&inf->address, &target_addr) == 0
	&& arp_packet->op == holy_cpu_to_be16_compile_time (ARP_REQUEST))
      {
	holy_net_link_level_address_t target;
	struct holy_net_buff nb_reply;
	struct arppkt *arp_reply;
	holy_uint8_t arp_data[128];
	holy_err_t err;

	nb_reply.head = arp_data;
	nb_reply.end = arp_data + sizeof (arp_data);
	holy_netbuff_clear (&nb_reply);
	holy_netbuff_reserve (&nb_reply, 128);

	err = holy_netbuff_push (&nb_reply, sizeof (*arp_packet));
	if (err)
	  return err;

	arp_reply = (struct arppkt *) nb_reply.data;

	arp_reply->hrd = holy_cpu_to_be16_compile_time (holy_NET_ARPHRD_ETHERNET);
	arp_reply->pro = holy_cpu_to_be16_compile_time (holy_NET_ETHERTYPE_IP);
	arp_reply->pln = 4;
	arp_reply->hln = 6;
	arp_reply->op = holy_cpu_to_be16_compile_time (ARP_REPLY);
	arp_reply->sender_ip = arp_packet->recv_ip;
	arp_reply->recv_ip = arp_packet->sender_ip;
	arp_reply->hln = 6;

	target.type = holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
	holy_memcpy (target.mac, arp_packet->sender_mac, 6);
	holy_memcpy (arp_reply->sender_mac, inf->hwaddress.mac, 6);
	holy_memcpy (arp_reply->recv_mac, arp_packet->sender_mac, 6);

	/* Change operation to REPLY and send packet */
	send_ethernet_packet (inf, &nb_reply, target, holy_NET_ETHERTYPE_ARP);
      }
  }
  return holy_ERR_NONE;
}
