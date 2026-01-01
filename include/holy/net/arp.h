/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_NET_ARP_HEADER
#define holy_NET_ARP_HEADER	1
#include <holy/misc.h>
#include <holy/net.h>

extern holy_err_t holy_net_arp_receive (struct holy_net_buff *nb,
					struct holy_net_card *card);

holy_err_t
holy_net_arp_send_request (struct holy_net_network_level_interface *inf,
			   const holy_net_network_level_address_t *proto_addr);

#endif 
