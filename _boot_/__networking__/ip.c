/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/net/ip.h>
#include <holy/misc.h>
#include <holy/net/arp.h>
#include <holy/net/udp.h>
#include <holy/net/ethernet.h>
#include <holy/net.h>
#include <holy/net/netbuff.h>
#include <holy/mm.h>
#include <holy/priority_queue.h>
#include <holy/time.h>

struct iphdr {
  holy_uint8_t verhdrlen;
  holy_uint8_t service;
  holy_uint16_t len;
  holy_uint16_t ident;
  holy_uint16_t frags;
  holy_uint8_t ttl;
  holy_uint8_t protocol;
  holy_uint16_t chksum;
  holy_uint32_t src;
  holy_uint32_t dest;
} holy_PACKED ;

enum
{
  DONT_FRAGMENT =  0x4000,
  MORE_FRAGMENTS = 0x2000,
  OFFSET_MASK =    0x1fff
};

typedef holy_uint64_t ip6addr[2];

struct ip6hdr {
  holy_uint32_t version_class_flow;
  holy_uint16_t len;
  holy_uint8_t protocol;
  holy_uint8_t ttl;
  ip6addr src;
  ip6addr dest;
} holy_PACKED ;

static int
cmp (const void *a__, const void *b__)
{
  struct holy_net_buff *a_ = *(struct holy_net_buff **) a__;
  struct holy_net_buff *b_ = *(struct holy_net_buff **) b__;
  struct iphdr *a = (struct iphdr *) a_->data;
  struct iphdr *b = (struct iphdr *) b_->data;
  /* We want the first elements to be on top.  */
  if ((holy_be_to_cpu16 (a->frags) & OFFSET_MASK)
      < (holy_be_to_cpu16 (b->frags) & OFFSET_MASK))
    return +1;
  if ((holy_be_to_cpu16 (a->frags) & OFFSET_MASK)
      > (holy_be_to_cpu16 (b->frags) & OFFSET_MASK))
    return -1;
  return 0;
}

struct reassemble
{
  struct reassemble *next;
  holy_uint32_t source;
  holy_uint32_t dest;
  holy_uint16_t id;
  holy_uint8_t proto;
  holy_uint64_t last_time;
  holy_priority_queue_t pq;
  struct holy_net_buff *asm_netbuff;
  holy_size_t total_len;
  holy_size_t cur_ptr;
  holy_uint8_t ttl;
};

static struct reassemble *reassembles;

holy_uint16_t
holy_net_ip_chksum (void *ipv, holy_size_t len)
{
  holy_uint16_t *ip = (holy_uint16_t *) ipv;
  holy_uint32_t sum = 0;

  for (; len >= 2; len -= 2)
    {
      sum += holy_be_to_cpu16 (holy_get_unaligned16 (ip++));
      if (sum > 0xFFFF)
	sum -= 0xFFFF;
    }
  if (len)
    {
      sum += *((holy_uint8_t *) ip) << 8;
      if (sum > 0xFFFF)
	sum -= 0xFFFF;
    }

  if (sum >= 0xFFFF)
    sum -= 0xFFFF;

  return holy_cpu_to_be16 ((~sum) & 0x0000FFFF);
}

static int id = 0x2400;

static holy_err_t
send_fragmented (struct holy_net_network_level_interface * inf,
		 const holy_net_network_level_address_t * target,
		 struct holy_net_buff * nb,
		 holy_net_ip_protocol_t proto,
		 holy_net_link_level_address_t ll_target_addr)
{
  holy_size_t off = 0;
  holy_size_t fraglen;
  holy_err_t err;

  fraglen = (inf->card->mtu - sizeof (struct iphdr)) & ~7;
  id++;

  while (nb->tail - nb->data)
    {
      holy_size_t len = fraglen;
      struct holy_net_buff *nb2;
      struct iphdr *iph;

      if ((holy_ssize_t) len > nb->tail - nb->data)
	len = nb->tail - nb->data;
      nb2 = holy_netbuff_alloc (fraglen + sizeof (struct iphdr)
				+ holy_NET_MAX_LINK_HEADER_SIZE);
      if (!nb2)
	return holy_errno;
      err = holy_netbuff_reserve (nb2, holy_NET_MAX_LINK_HEADER_SIZE);
      if (err)
	return err;
      err = holy_netbuff_put (nb2, sizeof (struct iphdr));
      if (err)
	return err;

      iph = (struct iphdr *) nb2->data;
      iph->verhdrlen = ((4 << 4) | 5);
      iph->service = 0;
      iph->len = holy_cpu_to_be16 (len + sizeof (struct iphdr));
      iph->ident = holy_cpu_to_be16 (id);
      iph->frags = holy_cpu_to_be16 (off | (((holy_ssize_t) len
					     == nb->tail - nb->data)
					    ? 0 : MORE_FRAGMENTS));
      iph->ttl = 0xff;
      iph->protocol = proto;
      iph->src = inf->address.ipv4;
      iph->dest = target->ipv4;
      off += len / 8;

      iph->chksum = 0;
      iph->chksum = holy_net_ip_chksum ((void *) nb2->data, sizeof (*iph));
      err = holy_netbuff_put (nb2, len);
      if (err)
	return err;
      holy_memcpy (iph + 1, nb->data, len);
      err = holy_netbuff_pull (nb, len);
      if (err)
	return err;
      err = send_ethernet_packet (inf, nb2, ll_target_addr,
				  holy_NET_ETHERTYPE_IP);
      if (err)
	return err;
    }
  return holy_ERR_NONE;
}

static holy_err_t
holy_net_send_ip4_packet (struct holy_net_network_level_interface *inf,
			  const holy_net_network_level_address_t *target,
			  const holy_net_link_level_address_t *ll_target_addr,
			  struct holy_net_buff *nb,
			  holy_net_ip_protocol_t proto)
{
  struct iphdr *iph;
  holy_err_t err;

  COMPILE_TIME_ASSERT (holy_NET_OUR_IPV4_HEADER_SIZE == sizeof (*iph));

  if (nb->tail - nb->data + sizeof (struct iphdr) > inf->card->mtu)
    return send_fragmented (inf, target, nb, proto, *ll_target_addr);

  err = holy_netbuff_push (nb, sizeof (*iph));
  if (err)
    return err;

  iph = (struct iphdr *) nb->data;
  iph->verhdrlen = ((4 << 4) | 5);
  iph->service = 0;
  iph->len = holy_cpu_to_be16 (nb->tail - nb->data);
  iph->ident = holy_cpu_to_be16 (++id);
  iph->frags = 0;
  iph->ttl = 0xff;
  iph->protocol = proto;
  iph->src = inf->address.ipv4;
  iph->dest = target->ipv4;

  iph->chksum = 0;
  iph->chksum = holy_net_ip_chksum ((void *) nb->data, sizeof (*iph));

  return send_ethernet_packet (inf, nb, *ll_target_addr,
			       holy_NET_ETHERTYPE_IP);
}

static holy_err_t
handle_dgram (struct holy_net_buff *nb,
	      struct holy_net_card *card,
	      const holy_net_link_level_address_t *source_hwaddress,
	      const holy_net_link_level_address_t *hwaddress,
	      holy_net_ip_protocol_t proto,
	      const holy_net_network_level_address_t *source,
	      const holy_net_network_level_address_t *dest,
	      holy_uint8_t ttl)
{
  struct holy_net_network_level_interface *inf = NULL;
  holy_err_t err;
  int multicast = 0;
  
  /* DHCP needs special treatment since we don't know IP yet.  */
  {
    struct udphdr *udph;
    udph = (struct udphdr *) nb->data;
    if (proto == holy_NET_IP_UDP && holy_be_to_cpu16 (udph->dst) == 68)
      {
	const struct holy_net_bootp_packet *bootp;
	if (udph->chksum)
	  {
	    holy_uint16_t chk, expected;
	    chk = udph->chksum;
	    udph->chksum = 0;
	    expected = holy_net_ip_transport_checksum (nb,
						       holy_NET_IP_UDP,
						       source,
						       dest);
	    if (expected != chk)
	      {
		holy_dprintf ("net", "Invalid UDP checksum. "
			      "Expected %x, got %x\n", 
			      holy_be_to_cpu16 (expected),
			      holy_be_to_cpu16 (chk));
		holy_netbuff_free (nb);
		return holy_ERR_NONE;
	      }
	    udph->chksum = chk;
	  }

	err = holy_netbuff_pull (nb, sizeof (*udph));
	if (err)
	  {
	    holy_netbuff_free (nb);
	    return err;
	  }

	bootp = (const struct holy_net_bootp_packet *) nb->data;
	
	FOR_NET_NETWORK_LEVEL_INTERFACES (inf)
	  if (inf->card == card
	      && inf->address.type == holy_NET_NETWORK_LEVEL_PROTOCOL_DHCP_RECV
	      && inf->hwaddress.type == holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET
	      && holy_memcmp (inf->hwaddress.mac, &bootp->mac_addr,
			      sizeof (inf->hwaddress.mac)) == 0)
	    {
	      holy_net_process_dhcp (nb, inf->card);
	      holy_netbuff_free (nb);
	      return holy_ERR_NONE;
	    }
	holy_netbuff_free (nb);
	return holy_ERR_NONE;
      }
  }

  FOR_NET_NETWORK_LEVEL_INTERFACES (inf)
  {
    if (inf->card == card
	&& holy_net_addr_cmp (&inf->address, dest) == 0
	&& holy_net_hwaddr_cmp (&inf->hwaddress, hwaddress) == 0)
      break;
    /* Solicited node multicast.  */
    if (inf->card == card
	&& inf->address.type == holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6
	&& dest->type == holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6
	&& dest->ipv6[0] == holy_be_to_cpu64_compile_time (0xff02ULL << 48)
	&& dest->ipv6[1] == (holy_be_to_cpu64_compile_time (0x01ff000000ULL)
			     | (inf->address.ipv6[1]
				& holy_be_to_cpu64_compile_time (0xffffff)))
	&& hwaddress->type == holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET
	&& hwaddress->mac[0] == 0x33 && hwaddress->mac[1] == 0x33
	&& hwaddress->mac[2] == 0xff
	&& hwaddress->mac[3] == ((holy_be_to_cpu64 (inf->address.ipv6[1])
				  >> 16) & 0xff)
	&& hwaddress->mac[4] == ((holy_be_to_cpu64 (inf->address.ipv6[1])
				  >> 8) & 0xff)
	&& hwaddress->mac[5] == ((holy_be_to_cpu64 (inf->address.ipv6[1])
				  >> 0) & 0xff))
      {
	multicast = 1;
	break;
      }
  }
 
  if (!inf && !(dest->type == holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6
		&& dest->ipv6[0] == holy_be_to_cpu64_compile_time (0xff02ULL
								   << 48)
		&& dest->ipv6[1] == holy_be_to_cpu64_compile_time (1)))
    {
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }
  if (multicast)
    inf = NULL;

  switch (proto)
    {
    case holy_NET_IP_UDP:
      return holy_net_recv_udp_packet (nb, inf, source);
    case holy_NET_IP_TCP:
      return holy_net_recv_tcp_packet (nb, inf, source);
    case holy_NET_IP_ICMP:
      return holy_net_recv_icmp_packet (nb, inf, source_hwaddress, source);
    case holy_NET_IP_ICMPV6:
      return holy_net_recv_icmp6_packet (nb, card, inf, source_hwaddress,
					 source, dest, ttl);
    default:
      holy_netbuff_free (nb);
      break;
    }
  return holy_ERR_NONE;
}

static void
free_rsm (struct reassemble *rsm)
{
  struct holy_net_buff **nb;
  while ((nb = holy_priority_queue_top (rsm->pq)))
    {
      holy_netbuff_free (*nb);
      holy_priority_queue_pop (rsm->pq);
    }
  holy_netbuff_free (rsm->asm_netbuff);
  holy_priority_queue_destroy (rsm->pq);
  holy_free (rsm);
}

static void
free_old_fragments (void)
{
  struct reassemble *rsm, **prev;
  holy_uint64_t limit_time = holy_get_time_ms ();

  limit_time = (limit_time > 90000) ? limit_time - 90000 : 0;

  for (prev = &reassembles, rsm = *prev; rsm; rsm = *prev)
    if (rsm->last_time < limit_time)
      {
	*prev = rsm->next;
	free_rsm (rsm);
      }
    else
      {
	prev = &rsm->next;
      }
}

static holy_err_t
holy_net_recv_ip4_packets (struct holy_net_buff *nb,
			   struct holy_net_card *card,
			   const holy_net_link_level_address_t *hwaddress,
			   const holy_net_link_level_address_t *src_hwaddress)
{
  struct iphdr *iph = (struct iphdr *) nb->data;
  holy_err_t err;
  struct reassemble *rsm, **prev;

  if ((iph->verhdrlen >> 4) != 4)
    {
      holy_dprintf ("net", "Bad IP version: %d\n", (iph->verhdrlen >> 4));
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }

  if ((iph->verhdrlen & 0xf) < 5)
    {
      holy_dprintf ("net", "IP header too short: %d\n",
		    (iph->verhdrlen & 0xf));
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }

  if (nb->tail - nb->data < (holy_ssize_t) ((iph->verhdrlen & 0xf)
					    * sizeof (holy_uint32_t)))
    {
      holy_dprintf ("net", "IP packet too short: %" PRIdholy_SSIZE "\n",
		    (holy_ssize_t) (nb->tail - nb->data));
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }

  /* Check size.  */
  {
    holy_size_t expected_size = holy_be_to_cpu16 (iph->len);
    holy_size_t actual_size = (nb->tail - nb->data);
    if (actual_size > expected_size)
      {
	err = holy_netbuff_unput (nb, actual_size - expected_size);
	if (err)
	  {
	    holy_netbuff_free (nb);
	    return err;
	  }
      }
    if (actual_size < expected_size)
      {
	holy_dprintf ("net", "Cut IP packet actual: %" PRIuholy_SIZE
		      ", expected %" PRIuholy_SIZE "\n", actual_size,
		      expected_size);
	holy_netbuff_free (nb);
	return holy_ERR_NONE;
      }
  }

  /* Unfragmented packet. Good.  */
  if (((holy_be_to_cpu16 (iph->frags) & MORE_FRAGMENTS) == 0)
      && (holy_be_to_cpu16 (iph->frags) & OFFSET_MASK) == 0)
    {
      holy_net_network_level_address_t source;
      holy_net_network_level_address_t dest;

      err = holy_netbuff_pull (nb, ((iph->verhdrlen & 0xf)
				    * sizeof (holy_uint32_t)));
      if (err)
	{
	  holy_netbuff_free (nb);
	  return err;
	}

      source.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
      source.ipv4 = iph->src;

      dest.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
      dest.ipv4 = iph->dest;

      return handle_dgram (nb, card, src_hwaddress, hwaddress, iph->protocol,
			   &source, &dest, iph->ttl);
    }

  for (prev = &reassembles, rsm = *prev; rsm; prev = &rsm->next, rsm = *prev)
    if (rsm->source == iph->src && rsm->dest == iph->dest
	&& rsm->id == iph->ident && rsm->proto == iph->protocol)
      break;
  if (!rsm)
    {
      rsm = holy_malloc (sizeof (*rsm));
      if (!rsm)
	return holy_errno;
      rsm->source = iph->src;
      rsm->dest = iph->dest;
      rsm->id = iph->ident;
      rsm->proto = iph->protocol;
      rsm->next = reassembles;
      reassembles = rsm;
      prev = &reassembles;
      rsm->pq = holy_priority_queue_new (sizeof (struct holy_net_buff **), cmp);
      if (!rsm->pq)
	{
	  holy_free (rsm);
	  return holy_errno;
	}
      rsm->asm_netbuff = 0;
      rsm->total_len = 0;
      rsm->cur_ptr = 0;
      rsm->ttl = 0xff;
    }
  if (rsm->ttl > iph->ttl)
    rsm->ttl = iph->ttl;
  rsm->last_time = holy_get_time_ms ();
  free_old_fragments ();

  err = holy_priority_queue_push (rsm->pq, &nb);
  if (err)
    return err;

  if (!(holy_be_to_cpu16 (iph->frags) & MORE_FRAGMENTS))
    {
      rsm->total_len = (8 * (holy_be_to_cpu16 (iph->frags) & OFFSET_MASK)
			+ (nb->tail - nb->data));
      rsm->total_len -= ((iph->verhdrlen & 0xf) * sizeof (holy_uint32_t));
      rsm->asm_netbuff = holy_netbuff_alloc (rsm->total_len);
      if (!rsm->asm_netbuff)
	{
	  *prev = rsm->next;
	  free_rsm (rsm);
	  return holy_errno;
	}
    }
  if (!rsm->asm_netbuff)
    return holy_ERR_NONE;

  while (1)
    {
      struct holy_net_buff **nb_top_p, *nb_top;
      holy_size_t copy;
      holy_size_t res_len;
      struct holy_net_buff *ret;
      holy_net_ip_protocol_t proto;
      holy_uint32_t src;
      holy_uint32_t dst;
      holy_net_network_level_address_t source;
      holy_net_network_level_address_t dest;
      holy_uint8_t ttl;

      nb_top_p = holy_priority_queue_top (rsm->pq);
      if (!nb_top_p)
	return holy_ERR_NONE;
      nb_top = *nb_top_p;
      holy_priority_queue_pop (rsm->pq);
      iph = (struct iphdr *) nb_top->data;
      err = holy_netbuff_pull (nb_top, ((iph->verhdrlen & 0xf)
					* sizeof (holy_uint32_t)));
      if (err)
	{
	  holy_netbuff_free (nb_top);
	  return err;
	}
      if (rsm->cur_ptr < (holy_size_t) 8 * (holy_be_to_cpu16 (iph->frags)
					    & OFFSET_MASK))
	{
	  holy_netbuff_free (nb_top);
	  return holy_ERR_NONE;
	}

      rsm->cur_ptr = (8 * (holy_be_to_cpu16 (iph->frags) & OFFSET_MASK)
		      + (nb_top->tail - nb_top->head));
      if ((holy_size_t) 8 * (holy_be_to_cpu16 (iph->frags) & OFFSET_MASK)
	  >= rsm->total_len)
	{
	  holy_netbuff_free (nb_top);
	  continue;
	}
      copy = nb_top->tail - nb_top->data;
      if (rsm->total_len - 8 * (holy_be_to_cpu16 (iph->frags) & OFFSET_MASK)
	  < copy)
	copy = rsm->total_len - 8 * (holy_be_to_cpu16 (iph->frags)
				     & OFFSET_MASK);
      holy_memcpy (&rsm->asm_netbuff->data[8 * (holy_be_to_cpu16 (iph->frags)
						& OFFSET_MASK)],
		   nb_top->data, copy);

      if ((holy_be_to_cpu16 (iph->frags) & MORE_FRAGMENTS))
	{
	  holy_netbuff_free (nb_top);
	  continue;
	}
      holy_netbuff_free (nb_top);

      ret = rsm->asm_netbuff;
      proto = rsm->proto;
      src = rsm->source;
      dst = rsm->dest;
      ttl = rsm->ttl;

      rsm->asm_netbuff = 0;
      res_len = rsm->total_len;
      *prev = rsm->next;
      free_rsm (rsm);

      if (holy_netbuff_put (ret, res_len))
	{
	  holy_netbuff_free (ret);
	  return holy_ERR_NONE;
	}

      source.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
      source.ipv4 = src;

      dest.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
      dest.ipv4 = dst;

      return handle_dgram (ret, card, src_hwaddress,
			   hwaddress, proto, &source, &dest,
			   ttl);
    }
}

static holy_err_t
holy_net_send_ip6_packet (struct holy_net_network_level_interface *inf,
			  const holy_net_network_level_address_t *target,
			  const holy_net_link_level_address_t *ll_target_addr,
			  struct holy_net_buff *nb,
			  holy_net_ip_protocol_t proto)
{
  struct ip6hdr *iph;
  holy_err_t err;

  COMPILE_TIME_ASSERT (holy_NET_OUR_IPV6_HEADER_SIZE == sizeof (*iph));

  if (nb->tail - nb->data + sizeof (struct iphdr) > inf->card->mtu)
    return holy_error (holy_ERR_NET_PACKET_TOO_BIG, "packet too big");

  err = holy_netbuff_push (nb, sizeof (*iph));
  if (err)
    return err;

  iph = (struct ip6hdr *) nb->data;
  iph->version_class_flow = holy_cpu_to_be32_compile_time ((6 << 28));
  iph->len = holy_cpu_to_be16 (nb->tail - nb->data - sizeof (*iph));
  iph->protocol = proto;
  iph->ttl = 0xff;
  holy_memcpy (&iph->src, inf->address.ipv6, sizeof (iph->src));
  holy_memcpy (&iph->dest, target->ipv6, sizeof (iph->dest));

  return send_ethernet_packet (inf, nb, *ll_target_addr,
			       holy_NET_ETHERTYPE_IP6);
}

holy_err_t
holy_net_send_ip_packet (struct holy_net_network_level_interface *inf,
			 const holy_net_network_level_address_t *target,
			 const holy_net_link_level_address_t *ll_target_addr,
			 struct holy_net_buff *nb,
			 holy_net_ip_protocol_t proto)
{
  switch (target->type)
    {
    case holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4:
      return holy_net_send_ip4_packet (inf, target, ll_target_addr, nb, proto);
    case holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6:
      return holy_net_send_ip6_packet (inf, target, ll_target_addr, nb, proto);
    default:
      return holy_error (holy_ERR_BUG, "not an IP");
    }
}

static holy_err_t
holy_net_recv_ip6_packets (struct holy_net_buff *nb,
			   struct holy_net_card *card,
			   const holy_net_link_level_address_t *hwaddress,
			   const holy_net_link_level_address_t *src_hwaddress)
{
  struct ip6hdr *iph = (struct ip6hdr *) nb->data;
  holy_err_t err;
  holy_net_network_level_address_t source;
  holy_net_network_level_address_t dest;

  if (nb->tail - nb->data < (holy_ssize_t) sizeof (*iph))
    {
      holy_dprintf ("net", "IP packet too short: %" PRIdholy_SSIZE "\n",
		    (holy_ssize_t) (nb->tail - nb->data));
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }

  err = holy_netbuff_pull (nb, sizeof (*iph));
  if (err)
    {
      holy_netbuff_free (nb);
      return err;
    }

  /* Check size.  */
  {
    holy_size_t expected_size = holy_be_to_cpu16 (iph->len);
    holy_size_t actual_size = (nb->tail - nb->data);
    if (actual_size > expected_size)
      {
	err = holy_netbuff_unput (nb, actual_size - expected_size);
	if (err)
	  {
	    holy_netbuff_free (nb);
	    return err;
	  }
      }
    if (actual_size < expected_size)
      {
	holy_dprintf ("net", "Cut IP packet actual: %" PRIuholy_SIZE
		      ", expected %" PRIuholy_SIZE "\n", actual_size,
		      expected_size);
	holy_netbuff_free (nb);
	return holy_ERR_NONE;
      }
  }

  source.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6;
  dest.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6;
  holy_memcpy (source.ipv6, &iph->src, sizeof (source.ipv6));
  holy_memcpy (dest.ipv6, &iph->dest, sizeof (dest.ipv6));

  return handle_dgram (nb, card, src_hwaddress, hwaddress, iph->protocol,
		       &source, &dest, iph->ttl);
}

holy_err_t
holy_net_recv_ip_packets (struct holy_net_buff *nb,
			  struct holy_net_card *card,
			  const holy_net_link_level_address_t *hwaddress,
			  const holy_net_link_level_address_t *src_hwaddress)
{
  struct iphdr *iph = (struct iphdr *) nb->data;

  if ((iph->verhdrlen >> 4) == 4)
    return holy_net_recv_ip4_packets (nb, card, hwaddress, src_hwaddress);
  if ((iph->verhdrlen >> 4) == 6)
    return holy_net_recv_ip6_packets (nb, card, hwaddress, src_hwaddress);
  holy_dprintf ("net", "Bad IP version: %d\n", (iph->verhdrlen >> 4));
  holy_netbuff_free (nb);
  return holy_ERR_NONE;
}
