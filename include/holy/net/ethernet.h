/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_NET_ETHERNET_HEADER
#define holy_NET_ETHERNET_HEADER	1
#include <holy/types.h>
#include <holy/net.h>

/* IANA Ethertype */
typedef enum
  {
    holy_NET_ETHERTYPE_IP = 0x0800,
    holy_NET_ETHERTYPE_ARP = 0x0806,
    holy_NET_ETHERTYPE_IP6 = 0x86DD,
  } holy_net_ethertype_t;

holy_err_t
send_ethernet_packet (struct holy_net_network_level_interface *inf,
		      struct holy_net_buff *nb,
		      holy_net_link_level_address_t target_addr,
		      holy_net_ethertype_t ethertype);
holy_err_t
holy_net_recv_ethernet_packet (struct holy_net_buff *nb,
			       struct holy_net_card *card);

#endif 
