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

struct router_adv
{
  holy_uint8_t ttl;
  holy_uint8_t flags;
  holy_uint16_t router_lifetime;
  holy_uint32_t reachable_time;
  holy_uint32_t retrans_timer;
  holy_uint8_t options[0];
} holy_PACKED;

struct option_header
{
  holy_uint8_t type;
  holy_uint8_t len;
} holy_PACKED;

struct prefix_option
{
  struct option_header header;
  holy_uint8_t prefixlen;
  holy_uint8_t flags;
  holy_uint32_t valid_lifetime;
  holy_uint32_t preferred_lifetime;
  holy_uint32_t reserved;
  holy_uint64_t prefix[2];
} holy_PACKED;

struct neighbour_solicit
{
  holy_uint32_t reserved;
  holy_uint64_t target[2];
} holy_PACKED;

struct neighbour_advertise
{
  holy_uint32_t flags;
  holy_uint64_t target[2];
} holy_PACKED;

struct router_solicit
{
  holy_uint32_t reserved;
} holy_PACKED;

enum
  {
    FLAG_SLAAC = 0x40
  };

enum
  {
    ICMP6_ECHO = 128,
    ICMP6_ECHO_REPLY = 129,
    ICMP6_ROUTER_SOLICIT = 133,
    ICMP6_ROUTER_ADVERTISE = 134,
    ICMP6_NEIGHBOUR_SOLICIT = 135,
    ICMP6_NEIGHBOUR_ADVERTISE = 136,
  };

enum
  {
    OPTION_SOURCE_LINK_LAYER_ADDRESS = 1,
    OPTION_TARGET_LINK_LAYER_ADDRESS = 2,
    OPTION_PREFIX = 3
  };

enum
  {
    FLAG_SOLICITED = (1 << 30),
    FLAG_OVERRIDE = (1 << 29)
  };

holy_err_t
holy_net_recv_icmp6_packet (struct holy_net_buff *nb,
			    struct holy_net_card *card,
			    struct holy_net_network_level_interface *inf,
			    const holy_net_link_level_address_t *ll_src,
			    const holy_net_network_level_address_t *source,
			    const holy_net_network_level_address_t *dest,
			    holy_uint8_t ttl)
{
  struct icmp_header *icmph;
  struct holy_net_network_level_interface *orig_inf = inf;
  holy_err_t err;
  holy_uint16_t checksum;

  icmph = (struct icmp_header *) nb->data;

  if (nb->tail - nb->data < (holy_ssize_t) sizeof (*icmph))
    {
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }

  checksum = icmph->checksum;
  icmph->checksum = 0;
  if (checksum != holy_net_ip_transport_checksum (nb,
						  holy_NET_IP_ICMPV6,
						  source,
						  dest))
    {
      holy_dprintf ("net", "invalid ICMPv6 checksum: %04x instead of %04x\n",
		    checksum,
		    holy_net_ip_transport_checksum (nb,
						    holy_NET_IP_ICMPV6,
						    source,
						    dest));
      icmph->checksum = checksum;
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }
  icmph->checksum = checksum;

  err = holy_netbuff_pull (nb, sizeof (*icmph));
  if (err)
    {
      holy_netbuff_free (nb);
      return err;
    }

  holy_dprintf ("net", "ICMPv6 message: %02x, %02x\n",
		icmph->type, icmph->code);
  switch (icmph->type)
    {
    case ICMP6_ECHO:
      /* Don't accept multicast pings.  */
      if (!inf)
	break;
      {
	struct holy_net_buff *nb_reply;
	struct icmp_header *icmphr;
	if (icmph->code)
	  break;
	nb_reply = holy_netbuff_alloc (nb->tail - nb->data + 512);
	if (!nb_reply)
	  {
	    holy_netbuff_free (nb);
	    return holy_errno;
	  }
	err = holy_netbuff_reserve (nb_reply, nb->tail - nb->data + 512);
	if (err)
	  goto ping_fail;
	err = holy_netbuff_push (nb_reply, nb->tail - nb->data);
	if (err)
	  goto ping_fail;
	holy_memcpy (nb_reply->data, nb->data, nb->tail - nb->data);
	err = holy_netbuff_push (nb_reply, sizeof (*icmphr));
	if (err)
	  goto ping_fail;
	icmphr = (struct icmp_header *) nb_reply->data;
	icmphr->type = ICMP6_ECHO_REPLY;
	icmphr->code = 0;
	icmphr->checksum = 0;
	icmphr->checksum = holy_net_ip_transport_checksum (nb_reply,
							   holy_NET_IP_ICMPV6,
							   &inf->address,
							   source);
	err = holy_net_send_ip_packet (inf, source, ll_src, nb_reply,
				       holy_NET_IP_ICMPV6);

      ping_fail:
	holy_netbuff_free (nb);
	holy_netbuff_free (nb_reply);
	return err;
      }
    case ICMP6_NEIGHBOUR_SOLICIT:
      {
	struct neighbour_solicit *nbh;
	struct holy_net_buff *nb_reply;
	struct option_header *ohdr;
	struct neighbour_advertise *adv;
	struct icmp_header *icmphr;
	holy_uint8_t *ptr;

	if (icmph->code)
	  break;
	if (ttl != 0xff)
	  break;
	nbh = (struct neighbour_solicit *) nb->data;
	err = holy_netbuff_pull (nb, sizeof (*nbh));
	if (err)
	  {
	    holy_netbuff_free (nb);
	    return err;
	  }
	for (ptr = (holy_uint8_t *) nb->data; ptr < nb->tail;
	     ptr += ohdr->len * 8)
	  {
	    ohdr = (struct option_header *) ptr;
	    if (ohdr->len == 0 || ptr + 8 * ohdr->len > nb->tail)
	      {
		holy_netbuff_free (nb);
		return holy_ERR_NONE;
	      }
	    if (ohdr->type == OPTION_SOURCE_LINK_LAYER_ADDRESS
		&& ohdr->len == 1)
	      {
		holy_net_link_level_address_t ll_address;
		ll_address.type = holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
		holy_memcpy (ll_address.mac, ohdr + 1, sizeof (ll_address.mac));
		holy_net_link_layer_add_address (card, source, &ll_address, 0);
	      }
	  }
	FOR_NET_NETWORK_LEVEL_INTERFACES (inf)
	{
	  if (inf->card == card
	      && inf->address.type == holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6
	      && holy_memcmp (&inf->address.ipv6, &nbh->target, 16) == 0)
	    break;
	}
	if (!inf)
	  break;

	nb_reply = holy_netbuff_alloc (sizeof (struct neighbour_advertise)
				       + sizeof (struct option_header)
				       + 6
				       + sizeof (struct icmp_header)
				       + holy_NET_OUR_IPV6_HEADER_SIZE
				       + holy_NET_MAX_LINK_HEADER_SIZE);
	if (!nb_reply)
	  {
	    holy_netbuff_free (nb);
	    return holy_errno;
	  }
	err = holy_netbuff_reserve (nb_reply,
				    sizeof (struct neighbour_advertise)
				    + sizeof (struct option_header)
				    + 6
				    + sizeof (struct icmp_header)
				    + holy_NET_OUR_IPV6_HEADER_SIZE
				    + holy_NET_MAX_LINK_HEADER_SIZE);
	if (err)
	  goto ndp_fail;

	err = holy_netbuff_push (nb_reply, 6);
	if (err)
	  goto ndp_fail;
	holy_memcpy (nb_reply->data, inf->hwaddress.mac, 6);
	err = holy_netbuff_push (nb_reply, sizeof (*ohdr));
	if (err)
	  goto ndp_fail;
	ohdr = (struct option_header *) nb_reply->data;
	ohdr->type = OPTION_TARGET_LINK_LAYER_ADDRESS;
	ohdr->len = 1;
	err = holy_netbuff_push (nb_reply, sizeof (*adv));
	if (err)
	  goto ndp_fail;
	adv = (struct neighbour_advertise *) nb_reply->data;
	adv->flags = holy_cpu_to_be32_compile_time (FLAG_SOLICITED
						    | FLAG_OVERRIDE);
	holy_memcpy (&adv->target, &nbh->target, 16);

	err = holy_netbuff_push (nb_reply, sizeof (*icmphr));
	if (err)
	  goto ndp_fail;
	icmphr = (struct icmp_header *) nb_reply->data;
	icmphr->type = ICMP6_NEIGHBOUR_ADVERTISE;
	icmphr->code = 0;
	icmphr->checksum = 0;
	icmphr->checksum = holy_net_ip_transport_checksum (nb_reply,
							   holy_NET_IP_ICMPV6,
							   &inf->address,
							   source);
	err = holy_net_send_ip_packet (inf, source, ll_src, nb_reply,
				       holy_NET_IP_ICMPV6);

      ndp_fail:
	holy_netbuff_free (nb);
	holy_netbuff_free (nb_reply);
	return err;
      }
    case ICMP6_NEIGHBOUR_ADVERTISE:
      {
	struct neighbour_advertise *nbh;
	holy_uint8_t *ptr;
	struct option_header *ohdr;

	if (icmph->code)
	  break;
	if (ttl != 0xff)
	  break;
	nbh = (struct neighbour_advertise *) nb->data;
	err = holy_netbuff_pull (nb, sizeof (*nbh));
	if (err)
	  {
	    holy_netbuff_free (nb);
	    return err;
	  }

	for (ptr = (holy_uint8_t *) nb->data; ptr < nb->tail;
	     ptr += ohdr->len * 8)
	  {
	    ohdr = (struct option_header *) ptr;
	    if (ohdr->len == 0 || ptr + 8 * ohdr->len > nb->tail)
	      {
		holy_netbuff_free (nb);
		return holy_ERR_NONE;
	      }
	    if (ohdr->type == OPTION_TARGET_LINK_LAYER_ADDRESS
		&& ohdr->len == 1)
	      {
		holy_net_link_level_address_t ll_address;
		ll_address.type = holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
		holy_memcpy (ll_address.mac, ohdr + 1, sizeof (ll_address.mac));
		holy_net_link_layer_add_address (card, source, &ll_address, 0);
	      }
	  }
	break;
      }
    case ICMP6_ROUTER_ADVERTISE:
      {
	holy_uint8_t *ptr;
	struct option_header *ohdr;
	struct router_adv *radv;
	struct holy_net_network_level_interface *route_inf = NULL;
	int default_route = 0;
	if (icmph->code)
	  break;
	radv = (struct router_adv *)nb->data;
	err = holy_netbuff_pull (nb, sizeof (struct router_adv));
	if (err)
	  {
	    holy_netbuff_free (nb);
	    return err;
	  }
	if (holy_be_to_cpu16 (radv->router_lifetime) > 0)
	  {
	    struct holy_net_route *route;

	    FOR_NET_ROUTES (route)
	    {
	      if (!holy_memcmp (&route->gw, source, sizeof (route->gw)))
		break;
	    }
	    if (route == NULL)
	      default_route = 1;
	  }

	for (ptr = (holy_uint8_t *) nb->data; ptr < nb->tail;
	     ptr += ohdr->len * 8)
	  {
	    ohdr = (struct option_header *) ptr;
	    if (ohdr->len == 0 || ptr + 8 * ohdr->len > nb->tail)
	      {
		holy_netbuff_free (nb);
		return holy_ERR_NONE;
	      }
	    if (ohdr->type == OPTION_SOURCE_LINK_LAYER_ADDRESS
		&& ohdr->len == 1)
	      {
		holy_net_link_level_address_t ll_address;
		ll_address.type = holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
		holy_memcpy (ll_address.mac, ohdr + 1, sizeof (ll_address.mac));
		holy_net_link_layer_add_address (card, source, &ll_address, 0);
	      }
	    if (ohdr->type == OPTION_PREFIX && ohdr->len == 4)
	      {
		struct prefix_option *opt = (struct prefix_option *) ptr;
		struct holy_net_slaac_mac_list *slaac;
		if (!(opt->flags & FLAG_SLAAC)
		    || (holy_be_to_cpu64 (opt->prefix[0]) >> 48) == 0xfe80
		    || (holy_be_to_cpu32 (opt->preferred_lifetime)
			> holy_be_to_cpu32 (opt->valid_lifetime))
		    || opt->prefixlen != 64)
		  {
		    holy_dprintf ("net", "discarded prefix: %d, %d, %d, %d\n",
				  !(opt->flags & FLAG_SLAAC),
				  (holy_be_to_cpu64 (opt->prefix[0]) >> 48) == 0xfe80,
				  (holy_be_to_cpu32 (opt->preferred_lifetime)
				   > holy_be_to_cpu32 (opt->valid_lifetime)),
				  opt->prefixlen != 64);
		    continue;
		  }
		for (slaac = card->slaac_list; slaac; slaac = slaac->next)
		  {
		    holy_net_network_level_address_t addr;
		    holy_net_network_level_netaddress_t netaddr;

		    if (slaac->address.type
			!= holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET)
		      continue;
		    addr.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6;
		    addr.ipv6[0] = opt->prefix[0];
		    addr.ipv6[1] = holy_net_ipv6_get_id (&slaac->address);
		    netaddr.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6;
		    netaddr.ipv6.base[0] = opt->prefix[0];
		    netaddr.ipv6.base[1] = 0;
		    netaddr.ipv6.masksize = 64;

		    FOR_NET_NETWORK_LEVEL_INTERFACES (inf)
		    {
		      if (inf->card == card
			  && holy_net_addr_cmp (&inf->address, &addr) == 0)
			break;
		    }
		    /* Update lease time if needed here once we have
		       lease times.  */
		    if (inf)
		      {
			if (!route_inf)
			  route_inf = inf;
			continue;
		      }

		    holy_dprintf ("net", "creating slaac\n");

		    {
		      char *name;
		      name = holy_xasprintf ("%s:%d",
					     slaac->name, slaac->slaac_counter++);
		      if (!name)
			{
			  holy_errno = holy_ERR_NONE;
			  continue;
			}
		      inf = holy_net_add_addr (name,
					       card, &addr,
					       &slaac->address, 0);
		      if (!route_inf)
			route_inf = inf;
		      holy_net_add_route (name, netaddr, inf);
		      holy_free (name);
		    }
		  }
	      }
	  }
	if (default_route)
	  {
	    char *name;
	    holy_net_network_level_netaddress_t netaddr;
	    name = holy_xasprintf ("%s:ra:default6", card->name);
	    if (!name)
	      {
		holy_errno = holy_ERR_NONE;
		goto next;
	      }
	    /* Default routes take alll of the traffic, so make the mask huge */
	    netaddr.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6;
	    netaddr.ipv6.masksize = 0;
	    netaddr.ipv6.base[0] = 0;
	    netaddr.ipv6.base[1] = 0;

	    /* May not have gotten slaac info, find a global address on this
	      card.  */
	    if (route_inf == NULL)
	      {
		FOR_NET_NETWORK_LEVEL_INTERFACES (inf)
		{
		  if (inf->card == card && inf != orig_inf
		      && inf->address.type == holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6
		      && holy_net_hwaddr_cmp(&inf->hwaddress,
					     &orig_inf->hwaddress) == 0)
		    {
		      route_inf = inf;
		      break;
		    }
		}
	      }
	    if (route_inf != NULL)
	      holy_net_add_route_gw (name, netaddr, *source, route_inf);
	    holy_free (name);
	  }
next:
	if (ptr != nb->tail)
	  break;
      }
    };

  holy_netbuff_free (nb);
  return holy_ERR_NONE;
}

holy_err_t
holy_net_icmp6_send_request (struct holy_net_network_level_interface *inf,
			     const holy_net_network_level_address_t *proto_addr)
{
  struct holy_net_buff *nb;
  holy_err_t err = holy_ERR_NONE;
  int i;
  struct option_header *ohdr;
  struct neighbour_solicit *sol;
  struct icmp_header *icmphr;
  holy_net_network_level_address_t multicast;
  holy_net_link_level_address_t ll_multicast;
  holy_uint8_t *nbd;
  multicast.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6;
  multicast.ipv6[0] = holy_be_to_cpu64_compile_time (0xff02ULL << 48);
  multicast.ipv6[1] = (holy_be_to_cpu64_compile_time (0x01ff000000ULL)
		       | (proto_addr->ipv6[1]
			  & holy_be_to_cpu64_compile_time (0xffffff)));
  
  err = holy_net_link_layer_resolve (inf, &multicast, &ll_multicast);
  if (err)
    return err;

  nb = holy_netbuff_alloc (sizeof (struct neighbour_solicit)
			   + sizeof (struct option_header)
			   + 6
			   + sizeof (struct icmp_header)
			   + holy_NET_OUR_IPV6_HEADER_SIZE
			   + holy_NET_MAX_LINK_HEADER_SIZE);
  if (!nb)
    return holy_errno;
  err = holy_netbuff_reserve (nb,
			      sizeof (struct neighbour_solicit)
			      + sizeof (struct option_header)
			      + 6
			      + sizeof (struct icmp_header)
			      + holy_NET_OUR_IPV6_HEADER_SIZE
			      + holy_NET_MAX_LINK_HEADER_SIZE);
  err = holy_netbuff_push (nb, 6);
  if (err)
    goto fail;

  holy_memcpy (nb->data, inf->hwaddress.mac, 6);
  err = holy_netbuff_push (nb, sizeof (*ohdr));
  if (err)
    goto fail;

  ohdr = (struct option_header *) nb->data;
  ohdr->type = OPTION_SOURCE_LINK_LAYER_ADDRESS;
  ohdr->len = 1;
  err = holy_netbuff_push (nb, sizeof (*sol));
  if (err)
    goto fail;

  sol = (struct neighbour_solicit *) nb->data;
  sol->reserved = 0;
  holy_memcpy (&sol->target, &proto_addr->ipv6, 16);

  err = holy_netbuff_push (nb, sizeof (*icmphr));
  if (err)
    goto fail;

  icmphr = (struct icmp_header *) nb->data;
  icmphr->type = ICMP6_NEIGHBOUR_SOLICIT;
  icmphr->code = 0;
  icmphr->checksum = 0;
  icmphr->checksum = holy_net_ip_transport_checksum (nb,
						     holy_NET_IP_ICMPV6,
						     &inf->address,
						     &multicast);
  nbd = nb->data;
  err = holy_net_send_ip_packet (inf, &multicast, &ll_multicast, nb,
				 holy_NET_IP_ICMPV6);
  if (err)
    goto fail;

  for (i = 0; i < holy_NET_TRIES; i++)
    {
      if (holy_net_link_layer_resolve_check (inf, proto_addr))
	break;
      holy_net_poll_cards (holy_NET_INTERVAL + (i * holy_NET_INTERVAL_ADDITION),
                           0);
      if (holy_net_link_layer_resolve_check (inf, proto_addr))
	break;
      nb->data = nbd;
      err = holy_net_send_ip_packet (inf, &multicast, &ll_multicast, nb,
				     holy_NET_IP_ICMPV6);
      if (err)
	break;
    }

 fail:
  holy_netbuff_free (nb);
  return err;
}

holy_err_t
holy_net_icmp6_send_router_solicit (struct holy_net_network_level_interface *inf)
{
  struct holy_net_buff *nb;
  holy_err_t err = holy_ERR_NONE;
  holy_net_network_level_address_t multicast;
  holy_net_link_level_address_t ll_multicast;
  struct option_header *ohdr;
  struct router_solicit *sol;
  struct icmp_header *icmphr;

  multicast.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6;
  multicast.ipv6[0] = holy_cpu_to_be64_compile_time (0xff02ULL << 48);
  multicast.ipv6[1] = holy_cpu_to_be64_compile_time (0x02ULL);

  err = holy_net_link_layer_resolve (inf, &multicast, &ll_multicast);
  if (err)
    return err;

  nb = holy_netbuff_alloc (sizeof (struct router_solicit)
			   + sizeof (struct option_header)
			   + 6
			   + sizeof (struct icmp_header)
			   + holy_NET_OUR_IPV6_HEADER_SIZE
			   + holy_NET_MAX_LINK_HEADER_SIZE);
  if (!nb)
    return holy_errno;
  err = holy_netbuff_reserve (nb,
			      sizeof (struct router_solicit)
			      + sizeof (struct option_header)
			      + 6
			      + sizeof (struct icmp_header)
			      + holy_NET_OUR_IPV6_HEADER_SIZE
			      + holy_NET_MAX_LINK_HEADER_SIZE);
  if (err)
    goto fail;

  err = holy_netbuff_push (nb, 6);
  if (err)
    goto fail;

  holy_memcpy (nb->data, inf->hwaddress.mac, 6);

  err = holy_netbuff_push (nb, sizeof (*ohdr));
  if (err)
    goto fail;

  ohdr = (struct option_header *) nb->data;
  ohdr->type = OPTION_SOURCE_LINK_LAYER_ADDRESS;
  ohdr->len = 1;

  err = holy_netbuff_push (nb, sizeof (*sol));
  if (err)
    goto fail;

  sol = (struct router_solicit *) nb->data;
  sol->reserved = 0;

  err = holy_netbuff_push (nb, sizeof (*icmphr));
  if (err)
    goto fail;

  icmphr = (struct icmp_header *) nb->data;
  icmphr->type = ICMP6_ROUTER_SOLICIT;
  icmphr->code = 0;
  icmphr->checksum = 0;
  icmphr->checksum = holy_net_ip_transport_checksum (nb,
						     holy_NET_IP_ICMPV6,
						     &inf->address,
						     &multicast);
  err = holy_net_send_ip_packet (inf, &multicast, &ll_multicast, nb,
				 holy_NET_IP_ICMPV6);
 fail:
  holy_netbuff_free (nb);
  return err;
}
