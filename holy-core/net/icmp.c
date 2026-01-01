/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/net.h>
#include <holy/net/ip.h>
#include <holy/net/netbuff.h>

struct icmp_header
{
  holy_uint8_t type;
  holy_uint8_t code;
  holy_uint16_t checksum;
} holy_PACKED;

struct ping_header
{
  holy_uint16_t id;
  holy_uint16_t seq;
} holy_PACKED;

enum
  {
    ICMP_ECHO_REPLY = 0,
    ICMP_ECHO = 8,
  };

holy_err_t
holy_net_recv_icmp_packet (struct holy_net_buff *nb,
			   struct holy_net_network_level_interface *inf,
			   const holy_net_link_level_address_t *ll_src,
			   const holy_net_network_level_address_t *src)
{
  struct icmp_header *icmph;
  holy_err_t err;
  holy_uint16_t checksum;

  /* Ignore broadcast.  */
  if (!inf)
    {
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }

  icmph = (struct icmp_header *) nb->data;

  if (nb->tail - nb->data < (holy_ssize_t) sizeof (*icmph))
    {
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }

  checksum = icmph->checksum;
  icmph->checksum = 0;
  if (checksum != holy_net_ip_chksum (nb->data, nb->tail - nb->data))
    {
      icmph->checksum = checksum;
      return holy_ERR_NONE;
    }
  icmph->checksum = checksum;

  err = holy_netbuff_pull (nb, sizeof (*icmph));
  if (err)
    return err;

  switch (icmph->type)
    {
    case ICMP_ECHO:
      {
	struct holy_net_buff *nb_reply;
	struct icmp_header *icmphr;
	if (icmph->code)
	  break;
	nb_reply = holy_netbuff_make_pkt (nb->tail - nb->data + sizeof (*icmphr));
	if (!nb_reply)
	  {
	    holy_netbuff_free (nb);
	    return holy_errno;
	  }
	holy_memcpy (nb_reply->data + sizeof (*icmphr), nb->data, nb->tail - nb->data);
	icmphr = (struct icmp_header *) nb_reply->data;
	icmphr->type = ICMP_ECHO_REPLY;
	icmphr->code = 0;
	icmphr->checksum = 0;
	icmphr->checksum = holy_net_ip_chksum ((void *) nb_reply->data,
					       nb_reply->tail - nb_reply->data);
	err = holy_net_send_ip_packet (inf, src, ll_src,
				       nb_reply, holy_NET_IP_ICMP);

	holy_netbuff_free (nb);
	holy_netbuff_free (nb_reply);
	return err;
      }
    };

  holy_netbuff_free (nb);
  return holy_ERR_NONE;
}
