/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_NET_UDP_HEADER
#define holy_NET_UDP_HEADER	1
#include <holy/types.h>
#include <holy/net.h>

struct udphdr
{
  holy_uint16_t src;
  holy_uint16_t dst;
  holy_uint16_t len;
  holy_uint16_t chksum;
} holy_PACKED;

struct holy_net_udp_socket;
typedef struct holy_net_udp_socket *holy_net_udp_socket_t;

holy_net_udp_socket_t
holy_net_udp_open (holy_net_network_level_address_t addr,
		   holy_uint16_t out_port,
		   holy_err_t (*recv_hook) (holy_net_udp_socket_t sock,
					    struct holy_net_buff *nb,
					    void *data),
		   void *recv_hook_data);

void
holy_net_udp_close (holy_net_udp_socket_t sock);

holy_err_t
holy_net_send_udp_packet (const holy_net_udp_socket_t socket,
			  struct holy_net_buff *nb);


#endif 
