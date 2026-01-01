/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_NET_IP_HEADER
#define holy_NET_IP_HEADER	1
#include <holy/misc.h>
#include <holy/net.h>

typedef enum holy_net_ip_protocol
  {
    holy_NET_IP_ICMP = 1,
    holy_NET_IP_TCP = 6,
    holy_NET_IP_UDP = 17,
    holy_NET_IP_ICMPV6 = 58
  } holy_net_ip_protocol_t;
#define holy_NET_IP_BROADCAST    0xFFFFFFFF

static inline holy_uint64_t
holy_net_ipv6_get_id (const holy_net_link_level_address_t *addr)
{
  return holy_cpu_to_be64 (((holy_uint64_t) (addr->mac[0] ^ 2) << 56)
			   | ((holy_uint64_t) addr->mac[1] << 48)
			   | ((holy_uint64_t) addr->mac[2] << 40)
			   | 0xfffe000000ULL
			   | ((holy_uint64_t) addr->mac[3] << 16)
			   | ((holy_uint64_t) addr->mac[4] << 8)
			   | ((holy_uint64_t) addr->mac[5]));
}

holy_uint16_t holy_net_ip_chksum(void *ipv, holy_size_t len);

holy_err_t
holy_net_recv_ip_packets (struct holy_net_buff *nb,
			  struct holy_net_card *card,
			  const holy_net_link_level_address_t *hwaddress,
			  const holy_net_link_level_address_t *src_hwaddress);

holy_err_t
holy_net_send_ip_packet (struct holy_net_network_level_interface *inf,
			 const holy_net_network_level_address_t *target,
			 const holy_net_link_level_address_t *ll_target_addr,
			 struct holy_net_buff *nb,
			 holy_net_ip_protocol_t proto);

holy_err_t
holy_net_recv_icmp_packet (struct holy_net_buff *nb,
			   struct holy_net_network_level_interface *inf,
			   const holy_net_link_level_address_t *ll_src,
			   const holy_net_network_level_address_t *src);
holy_err_t
holy_net_recv_icmp6_packet (struct holy_net_buff *nb,
			    struct holy_net_card *card,
			    struct holy_net_network_level_interface *inf,
			    const holy_net_link_level_address_t *ll_src,
			    const holy_net_network_level_address_t *source,
			    const holy_net_network_level_address_t *dest,
			    holy_uint8_t ttl);
holy_err_t
holy_net_recv_udp_packet (struct holy_net_buff *nb,
			  struct holy_net_network_level_interface *inf,
			  const holy_net_network_level_address_t *src);
holy_err_t
holy_net_recv_tcp_packet (struct holy_net_buff *nb,
			  struct holy_net_network_level_interface *inf,
			  const holy_net_network_level_address_t *source);

holy_uint16_t
holy_net_ip_transport_checksum (struct holy_net_buff *nb,
				holy_uint16_t proto,
				const holy_net_network_level_address_t *src,
				const holy_net_network_level_address_t *dst);

struct holy_net_network_level_interface *
holy_net_ipv6_get_link_local (struct holy_net_card *card,
			      const holy_net_link_level_address_t *hwaddr);
holy_err_t
holy_net_icmp6_send_request (struct holy_net_network_level_interface *inf,
			     const holy_net_network_level_address_t *proto_addr);

holy_err_t
holy_net_icmp6_send_router_solicit (struct holy_net_network_level_interface *inf);
#endif 
