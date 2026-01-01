/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/net.h>
#include <holy/net/netbuff.h>
#include <holy/time.h>
#include <holy/file.h>
#include <holy/i18n.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/dl.h>
#include <holy/command.h>
#include <holy/env.h>
#include <holy/net/ethernet.h>
#include <holy/net/arp.h>
#include <holy/net/ip.h>
#include <holy/loader.h>
#include <holy/bufio.h>
#include <holy/kernel.h>

holy_MOD_LICENSE ("GPLv2+");

char *holy_net_default_server;

struct holy_net_route *holy_net_routes = NULL;
struct holy_net_network_level_interface *holy_net_network_level_interfaces = NULL;
struct holy_net_card *holy_net_cards = NULL;
struct holy_net_network_level_protocol *holy_net_network_level_protocols = NULL;
static struct holy_fs holy_net_fs;

struct holy_net_link_layer_entry {
  int avail;
  holy_net_network_level_address_t nl_address;
  holy_net_link_level_address_t ll_address;
};

#define LINK_LAYER_CACHE_SIZE 256

static struct holy_net_link_layer_entry *
link_layer_find_entry (const holy_net_network_level_address_t *proto,
		       const struct holy_net_card *card)
{
  unsigned i;
  if (!card->link_layer_table)
    return NULL;
  for (i = 0; i < LINK_LAYER_CACHE_SIZE; i++)
    {
      if (card->link_layer_table[i].avail == 1 
	  && holy_net_addr_cmp (&card->link_layer_table[i].nl_address,
				proto) == 0)
	return &card->link_layer_table[i];
    }
  return NULL;
}

void
holy_net_link_layer_add_address (struct holy_net_card *card,
				 const holy_net_network_level_address_t *nl,
				 const holy_net_link_level_address_t *ll,
				 int override)
{
  struct holy_net_link_layer_entry *entry;

  /* Check if the sender is in the cache table.  */
  entry = link_layer_find_entry (nl, card);
  /* Update sender hardware address.  */
  if (entry && override)
    holy_memcpy (&entry->ll_address, ll, sizeof (entry->ll_address));
  if (entry)
    return;

  /* Add sender to cache table.  */
  if (card->link_layer_table == NULL)
    card->link_layer_table = holy_zalloc (LINK_LAYER_CACHE_SIZE
					  * sizeof (card->link_layer_table[0]));
  entry = &(card->link_layer_table[card->new_ll_entry]);
  entry->avail = 1;
  holy_memcpy (&entry->ll_address, ll, sizeof (entry->ll_address));
  holy_memcpy (&entry->nl_address, nl, sizeof (entry->nl_address));
  card->new_ll_entry++;
  if (card->new_ll_entry == LINK_LAYER_CACHE_SIZE)
    card->new_ll_entry = 0;
}

int
holy_net_link_layer_resolve_check (struct holy_net_network_level_interface *inf,
				   const holy_net_network_level_address_t *proto_addr)
{
  struct holy_net_link_layer_entry *entry;

  if (proto_addr->type == holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4
      && proto_addr->ipv4 == 0xffffffff)
    return 1;
  entry = link_layer_find_entry (proto_addr, inf->card);
  if (entry)
    return 1;
  return 0;
}

holy_err_t
holy_net_link_layer_resolve (struct holy_net_network_level_interface *inf,
			     const holy_net_network_level_address_t *proto_addr,
			     holy_net_link_level_address_t *hw_addr)
{
  struct holy_net_link_layer_entry *entry;
  holy_err_t err;

  if ((proto_addr->type == holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4
       && proto_addr->ipv4 == 0xffffffff)
      || proto_addr->type == holy_NET_NETWORK_LEVEL_PROTOCOL_DHCP_RECV
      || (proto_addr->type == holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6
	  && proto_addr->ipv6[0] == holy_be_to_cpu64_compile_time (0xff02ULL
								   << 48)
	  && proto_addr->ipv6[1] == (holy_be_to_cpu64_compile_time (1))))
    {
      hw_addr->type = holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
      holy_memset (hw_addr->mac, -1, 6);
      return holy_ERR_NONE;
    }

  if (proto_addr->type == holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6
      && ((holy_be_to_cpu64 (proto_addr->ipv6[0]) >> 56) == 0xff))
    {
      hw_addr->type = holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
      hw_addr->mac[0] = 0x33;
      hw_addr->mac[1] = 0x33;
      hw_addr->mac[2] = ((holy_be_to_cpu64 (proto_addr->ipv6[1]) >> 24) & 0xff);
      hw_addr->mac[3] = ((holy_be_to_cpu64 (proto_addr->ipv6[1]) >> 16) & 0xff);
      hw_addr->mac[4] = ((holy_be_to_cpu64 (proto_addr->ipv6[1]) >> 8) & 0xff);
      hw_addr->mac[5] = ((holy_be_to_cpu64 (proto_addr->ipv6[1]) >> 0) & 0xff);
      return holy_ERR_NONE;
    }

  /* Check cache table.  */
  entry = link_layer_find_entry (proto_addr, inf->card);
  if (entry)
    {
      *hw_addr = entry->ll_address;
      return holy_ERR_NONE;
    }
  switch (proto_addr->type)
    {
    case holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4:
      err = holy_net_arp_send_request (inf, proto_addr);
      break;
    case holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6:
      err = holy_net_icmp6_send_request (inf, proto_addr);
      break;
    case holy_NET_NETWORK_LEVEL_PROTOCOL_DHCP_RECV:
      return holy_error (holy_ERR_BUG, "shouldn't reach here");
    default:
      return holy_error (holy_ERR_BUG,
			 "unsupported address type %d", proto_addr->type);
    }
  if (err)
    return err;
  entry = link_layer_find_entry (proto_addr, inf->card);
  if (entry)
    {
      *hw_addr = entry->ll_address;
      return holy_ERR_NONE;
    }
  return holy_error (holy_ERR_TIMEOUT,
		     N_("timeout: could not resolve hardware address"));
}

void
holy_net_card_unregister (struct holy_net_card *card)
{
  struct holy_net_network_level_interface *inf, *next;
  FOR_NET_NETWORK_LEVEL_INTERFACES_SAFE(inf, next)
    if (inf->card == card)
      holy_net_network_level_interface_unregister (inf);
  if (card->opened)
    {
      if (card->driver->close)
	card->driver->close (card);
      card->opened = 0;
    }
  holy_list_remove (holy_AS_LIST (card));
}

static struct holy_net_slaac_mac_list *
holy_net_ipv6_get_slaac (struct holy_net_card *card,
			 const holy_net_link_level_address_t *hwaddr)
{
  struct holy_net_slaac_mac_list *slaac;
  char *ptr;

  for (slaac = card->slaac_list; slaac; slaac = slaac->next)
    if (holy_net_hwaddr_cmp (&slaac->address, hwaddr) == 0)
      return slaac;

  slaac = holy_zalloc (sizeof (*slaac));
  if (!slaac)
    return NULL;

  slaac->name = holy_malloc (holy_strlen (card->name)
			     + holy_NET_MAX_STR_HWADDR_LEN
			     + sizeof (":slaac"));
  ptr = holy_stpcpy (slaac->name, card->name);
  if (holy_net_hwaddr_cmp (&card->default_address, hwaddr) != 0)
    {
      ptr = holy_stpcpy (ptr, ":");
      holy_net_hwaddr_to_str (hwaddr, ptr);
      ptr += holy_strlen (ptr);
    }
  ptr = holy_stpcpy (ptr, ":slaac");

  holy_memcpy (&slaac->address, hwaddr, sizeof (slaac->address));
  slaac->next = card->slaac_list;
  card->slaac_list = slaac;
  return slaac;
}

static void
holy_net_network_level_interface_register (struct holy_net_network_level_interface *inter);

static struct holy_net_network_level_interface *
holy_net_add_addr_real (char *name,
			struct holy_net_card *card,
			const holy_net_network_level_address_t *addr,
			const holy_net_link_level_address_t *hwaddress,
			holy_net_interface_flags_t flags)
{
  struct holy_net_network_level_interface *inter;

  inter = holy_zalloc (sizeof (*inter));
  if (!inter)
    return NULL;

  inter->name = name;
  holy_memcpy (&(inter->address), addr, sizeof (inter->address));
  holy_memcpy (&(inter->hwaddress), hwaddress, sizeof (inter->hwaddress));
  inter->flags = flags;
  inter->card = card;
  inter->dhcp_ack = NULL;
  inter->dhcp_acklen = 0;

  holy_net_network_level_interface_register (inter);

  return inter;
}

struct holy_net_network_level_interface *
holy_net_add_addr (const char *name,
		   struct holy_net_card *card,
		   const holy_net_network_level_address_t *addr,
		   const holy_net_link_level_address_t *hwaddress,
		   holy_net_interface_flags_t flags)
{
  char *name_dup = holy_strdup (name);
  struct holy_net_network_level_interface *ret;
 
  if (!name_dup)
    return NULL;
  ret = holy_net_add_addr_real (name_dup, card, addr, hwaddress, flags);
  if (!ret)
    holy_free (name_dup);
  return ret;
}

struct holy_net_network_level_interface *
holy_net_ipv6_get_link_local (struct holy_net_card *card,
			      const holy_net_link_level_address_t *hwaddr)
{
  struct holy_net_network_level_interface *inf;
  char *name;
  char *ptr;
  holy_net_network_level_address_t addr;

  addr.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6;
  addr.ipv6[0] = holy_cpu_to_be64_compile_time (0xfe80ULL << 48);
  addr.ipv6[1] = holy_net_ipv6_get_id (hwaddr);

  FOR_NET_NETWORK_LEVEL_INTERFACES (inf)
  {
    if (inf->card == card
	&& holy_net_hwaddr_cmp (&inf->hwaddress, hwaddr) == 0
	&& holy_net_addr_cmp (&inf->address, &addr) == 0)
      return inf;
  }

  name = holy_malloc (holy_strlen (card->name)
		      + holy_NET_MAX_STR_HWADDR_LEN
		      + sizeof (":link"));
  if (!name)
    return NULL;

  ptr = holy_stpcpy (name, card->name);
  if (holy_net_hwaddr_cmp (&card->default_address, hwaddr) != 0)
    {
      ptr = holy_stpcpy (ptr, ":");
      holy_net_hwaddr_to_str (hwaddr, ptr);
      ptr += holy_strlen (ptr);
    }
  ptr = holy_stpcpy (ptr, ":link");
  return holy_net_add_addr_real (name, card, &addr, hwaddr, 0);
}

/* FIXME: allow to specify mac address.  */
static holy_err_t
holy_cmd_ipv6_autoconf (struct holy_command *cmd __attribute__ ((unused)),
			int argc, char **args)
{
  struct holy_net_card *card;
  struct holy_net_network_level_interface **ifaces;
  holy_size_t ncards = 0;
  unsigned j = 0;
  int interval;
  holy_err_t err;
  struct holy_net_slaac_mac_list **slaacs;

  FOR_NET_CARDS (card)
  {
    if (argc > 0 && holy_strcmp (card->name, args[0]) != 0)
      continue;
    ncards++;
  }

  ifaces = holy_zalloc (ncards * sizeof (ifaces[0]));
  slaacs = holy_zalloc (ncards * sizeof (slaacs[0]));
  if (!ifaces || !slaacs)
    {
      holy_free (ifaces);
      holy_free (slaacs);
      return holy_errno;
    }

  FOR_NET_CARDS (card)
  {
    if (argc > 0 && holy_strcmp (card->name, args[0]) != 0)
      continue;
    ifaces[j] = holy_net_ipv6_get_link_local (card, &card->default_address);
    if (!ifaces[j])
      {
	holy_free (ifaces);
	holy_free (slaacs);
	return holy_errno;
      }
    slaacs[j] = holy_net_ipv6_get_slaac (card, &card->default_address);
    if (!slaacs[j])
      {
	holy_free (ifaces);
	holy_free (slaacs);
	return holy_errno;
      }
    j++;
  }

  for (interval = 200; interval < 10000; interval *= 2)
    {
      int done = 1;
      for (j = 0; j < ncards; j++)
	{
	  if (slaacs[j]->slaac_counter)
	    continue;
	  err = holy_net_icmp6_send_router_solicit (ifaces[j]);
	  if (err)
	    err = holy_ERR_NONE;
	  done = 0;
	}
      if (done)
	break;
      holy_net_poll_cards (interval, 0);
    }

  err = holy_ERR_NONE;
  for (j = 0; j < ncards; j++)
    {
      if (slaacs[j]->slaac_counter)
	continue;
      err = holy_error (holy_ERR_FILE_NOT_FOUND,
			N_("couldn't autoconfigure %s"),
			ifaces[j]->card->name);
    }

  holy_free (ifaces);
  holy_free (slaacs);
  return err;
}


static int
parse_ip (const char *val, holy_uint32_t *ip, const char **rest)
{
  holy_uint32_t newip = 0;
  int i;
  const char *ptr = val;

  for (i = 0; i < 4; i++)
    {
      unsigned long t;
      t = holy_strtoul (ptr, (char **) &ptr, 0);
      if (holy_errno)
	{
	  holy_errno = holy_ERR_NONE;
	  return 0;
	}
      if (*ptr != '.' && i == 0)
	{
	  newip = t;
	  break;
	}
      if (t & ~0xff)
	return 0;
      newip >>= 8;
      newip |= (t << 24);
      if (i != 3 && *ptr != '.')
	return 0;
      ptr++;
    }
  *ip = holy_cpu_to_le32 (newip);
  if (rest)
    *rest = (ptr - 1);
  return 1;
}

static int
parse_ip6 (const char *val, holy_uint64_t *ip, const char **rest)
{
  holy_uint16_t newip[8];
  const char *ptr = val;
  int word, quaddot = -1;

  if (ptr[0] == ':' && ptr[1] != ':')
    return 0;
  if (ptr[0] == ':')
    ptr++;

  for (word = 0; word < 8; word++)
    {
      unsigned long t;
      if (*ptr == ':')
	{
	  quaddot = word;
	  word--;
	  ptr++;
	  continue;
	}
      t = holy_strtoul (ptr, (char **) &ptr, 16);
      if (holy_errno)
	{
	  holy_errno = holy_ERR_NONE;
	  break;
	}
      if (t & ~0xffff)
	return 0;
      newip[word] = holy_cpu_to_be16 (t);
      if (*ptr != ':')
	break;
      ptr++;
    }
  if (quaddot == -1 && word < 7)
    return 0;
  if (quaddot != -1)
    {
      holy_memmove (&newip[quaddot + 7 - word], &newip[quaddot],
		    (word - quaddot + 1) * sizeof (newip[0]));
      holy_memset (&newip[quaddot], 0, (7 - word) * sizeof (newip[0]));
    }
  holy_memcpy (ip, newip, 16);
  if (rest)
    *rest = ptr;
  return 1;
}

static int
match_net (const holy_net_network_level_netaddress_t *net,
	   const holy_net_network_level_address_t *addr)
{
  if (net->type != addr->type)
    return 0;
  switch (net->type)
    {
    case holy_NET_NETWORK_LEVEL_PROTOCOL_DHCP_RECV:
      return 0;
    case holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4:
      {
	holy_uint32_t mask = (0xffffffffU << (32 - net->ipv4.masksize));
	if (net->ipv4.masksize == 0)
	  mask = 0;
	return ((holy_be_to_cpu32 (net->ipv4.base) & mask)
		== (holy_be_to_cpu32 (addr->ipv4) & mask));
      }
    case holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6:
      {
	holy_uint64_t mask[2];
	if (net->ipv6.masksize == 0)
	  return 1;
	if (net->ipv6.masksize <= 64)
	  {
	    mask[0] = 0xffffffffffffffffULL << (64 - net->ipv6.masksize);
	    mask[1] = 0;
	  }
	else 
	  {
	    mask[0] = 0xffffffffffffffffULL;
	    mask[1] = 0xffffffffffffffffULL << (128 - net->ipv6.masksize);
	  }
	return (((holy_be_to_cpu64 (net->ipv6.base[0]) & mask[0])
		== (holy_be_to_cpu64 (addr->ipv6[0]) & mask[0]))
		&& ((holy_be_to_cpu64 (net->ipv6.base[1]) & mask[1])
		    == (holy_be_to_cpu64 (addr->ipv6[1]) & mask[1])));
      }
    }
  return 0;
}

holy_err_t
holy_net_resolve_address (const char *name,
			  holy_net_network_level_address_t *addr)
{
  const char *rest;
  holy_err_t err;
  holy_size_t naddresses;
  struct holy_net_network_level_address *addresses = 0;

  if (parse_ip (name, &addr->ipv4, &rest) && *rest == 0)
    {
      addr->type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
      return holy_ERR_NONE;
    }
  if (parse_ip6 (name, addr->ipv6, &rest) && *rest == 0)
    {
      addr->type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6;
      return holy_ERR_NONE;
    }
  err = holy_net_dns_lookup (name, 0, 0, &naddresses, &addresses, 1);
  if (err)
    return err;
  if (!naddresses)
    holy_error (holy_ERR_NET_BAD_ADDRESS, N_("unresolvable address %s"),
		name);
  /* FIXME: use other results as well.  */
  *addr = addresses[0];
  holy_free (addresses);
  return holy_ERR_NONE;
}

holy_err_t
holy_net_resolve_net_address (const char *name,
			      holy_net_network_level_netaddress_t *addr)
{
  const char *rest;
  if (parse_ip (name, &addr->ipv4.base, &rest))
    {
      addr->type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
      if (*rest == '/')
	{
	  addr->ipv4.masksize = holy_strtoul (rest + 1, (char **) &rest, 0);
	  if (!holy_errno && *rest == 0)
	    return holy_ERR_NONE;
	  holy_errno = holy_ERR_NONE;
	}
      else if (*rest == 0)
	{
	  addr->ipv4.masksize = 32;
	  return holy_ERR_NONE;
	}
    }
  if (parse_ip6 (name, addr->ipv6.base, &rest))
    {
      addr->type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6;
      if (*rest == '/')
	{
	  addr->ipv6.masksize = holy_strtoul (rest + 1, (char **) &rest, 0);
	  if (!holy_errno && *rest == 0)
	    return holy_ERR_NONE;
	  holy_errno = holy_ERR_NONE;
	}
      else if (*rest == 0)
	{
	  addr->ipv6.masksize = 128;
	  return holy_ERR_NONE;
	}
    }
  return holy_error (holy_ERR_NET_BAD_ADDRESS,
		     N_("unrecognised network address `%s'"),
		     name);
}

static int
route_cmp (const struct holy_net_route *a, const struct holy_net_route *b)
{
  if (a == NULL && b == NULL)
    return 0;
  if (b == NULL)
    return +1;
  if (a == NULL)
    return -1;
  if (a->target.type < b->target.type)
    return -1;
  if (a->target.type > b->target.type)
    return +1;
  switch (a->target.type)
    {
    case holy_NET_NETWORK_LEVEL_PROTOCOL_DHCP_RECV:
      break;
    case holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6:
      if (a->target.ipv6.masksize > b->target.ipv6.masksize)
	return +1;
      if (a->target.ipv6.masksize < b->target.ipv6.masksize)
	return -1;
      break;
    case holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4:
      if (a->target.ipv4.masksize > b->target.ipv4.masksize)
	return +1;
      if (a->target.ipv4.masksize < b->target.ipv4.masksize)
	return -1;
      break;
    }
  return 0;
}

holy_err_t
holy_net_route_address (holy_net_network_level_address_t addr,
			holy_net_network_level_address_t *gateway,
			struct holy_net_network_level_interface **interf)
{
  struct holy_net_route *route;
  unsigned int depth = 0;
  unsigned int routecnt = 0;
  struct holy_net_network_level_protocol *prot = NULL;
  holy_net_network_level_address_t curtarget = addr;

  *gateway = addr;

  FOR_NET_ROUTES(route)
    routecnt++;

  for (depth = 0; depth < routecnt + 2 && depth < holy_UINT_MAX; depth++)
    {
      struct holy_net_route *bestroute = NULL;
      FOR_NET_ROUTES(route)
      {
	if (depth && prot != route->prot)
	  continue;
	if (!match_net (&route->target, &curtarget))
	  continue;
	if (route_cmp (route, bestroute) > 0)
	  bestroute = route;
      }
      if (bestroute == NULL)
	return holy_error (holy_ERR_NET_NO_ROUTE,
			   N_("destination unreachable"));

      if (!bestroute->is_gateway)
	{
	  *interf = bestroute->interface;
	  return holy_ERR_NONE;
	}
      if (depth == 0)
	{
	  *gateway = bestroute->gw;
	  if (bestroute->interface != NULL)
	    {
	      *interf = bestroute->interface;
	      return holy_ERR_NONE;
	    }
	}
      curtarget = bestroute->gw;
    }

  return holy_error (holy_ERR_NET_ROUTE_LOOP,
		     /* TRANSLATORS: route loop is a condition when e.g.
			to contact server A you need to go through B
			and to contact B you need to go through A.  */
		     N_("route loop detected"));
}

static holy_err_t
holy_cmd_deladdr (struct holy_command *cmd __attribute__ ((unused)),
		  int argc, char **args)
{
  struct holy_net_network_level_interface *inter;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));

  FOR_NET_NETWORK_LEVEL_INTERFACES (inter)
    if (holy_strcmp (inter->name, args[0]) == 0)
      break;
  if (inter == NULL)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("address not found"));

  if (inter->flags & holy_NET_INTERFACE_PERMANENT)
    return holy_error (holy_ERR_IO,
		       N_("you can't delete this address"));

  holy_net_network_level_interface_unregister (inter);
  holy_free (inter->name);
  holy_free (inter);

  return holy_ERR_NONE;
}

void
holy_net_addr_to_str (const holy_net_network_level_address_t *target, char *buf)
{
  switch (target->type)
    {
    case holy_NET_NETWORK_LEVEL_PROTOCOL_DHCP_RECV:
      COMPILE_TIME_ASSERT (sizeof ("temporary") < holy_NET_MAX_STR_ADDR_LEN);
      holy_strcpy (buf, "temporary");
      return;
    case holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6:
      {
	char *ptr = buf;
	holy_uint64_t n = holy_be_to_cpu64 (target->ipv6[0]);
	int i;
	for (i = 0; i < 4; i++)
	  {
	    holy_snprintf (ptr, 6, "%" PRIxholy_UINT64_T ":",
			   (n >> (48 - 16 * i)) & 0xffff);
	    ptr += holy_strlen (ptr);
	  }
	n  = holy_be_to_cpu64 (target->ipv6[1]);
	for (i = 0; i < 3; i++)
	  {
	    holy_snprintf (ptr, 6, "%" PRIxholy_UINT64_T ":",
			   (n >> (48 - 16 * i)) & 0xffff);
	    ptr += holy_strlen (ptr);
	  }
	holy_snprintf (ptr, 5, "%" PRIxholy_UINT64_T, n & 0xffff);
	return;
      }
    case holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4:
      {
	holy_uint32_t n = holy_be_to_cpu32 (target->ipv4);
	holy_snprintf (buf, holy_NET_MAX_STR_ADDR_LEN, "%d.%d.%d.%d",
		       ((n >> 24) & 0xff), ((n >> 16) & 0xff),
		       ((n >> 8) & 0xff), ((n >> 0) & 0xff));
      }
      return;
    }
  holy_snprintf (buf, holy_NET_MAX_STR_ADDR_LEN,
		 "Unknown address type %d", target->type);
}


void
holy_net_hwaddr_to_str (const holy_net_link_level_address_t *addr, char *str)
{
  str[0] = 0;
  switch (addr->type)
    {
    case holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET:
      {
	char *ptr;
	unsigned i;
	for (ptr = str, i = 0; i < ARRAY_SIZE (addr->mac); i++)
	  {
	    holy_snprintf (ptr, holy_NET_MAX_STR_HWADDR_LEN - (ptr - str),
			   "%02x:", addr->mac[i] & 0xff);
	    ptr += (sizeof ("XX:") - 1);
	  }
      return;
      }
    }
  holy_printf (_("Unsupported hw address type %d\n"), addr->type);
}

int
holy_net_hwaddr_cmp (const holy_net_link_level_address_t *a,
		     const holy_net_link_level_address_t *b)
{
  if (a->type < b->type)
    return -1;
  if (a->type > b->type)
    return +1;
  switch (a->type)
    {
    case holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET:
      return holy_memcmp (a->mac, b->mac, sizeof (a->mac));
    }
  holy_printf (_("Unsupported hw address type %d\n"), a->type);
  return 1;
}

int
holy_net_addr_cmp (const holy_net_network_level_address_t *a,
		   const holy_net_network_level_address_t *b)
{
  if (a->type < b->type)
    return -1;
  if (a->type > b->type)
    return +1;
  switch (a->type)
    {
    case holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4:
      return holy_memcmp (&a->ipv4, &b->ipv4, sizeof (a->ipv4));
    case holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6:
      return holy_memcmp (&a->ipv6, &b->ipv6, sizeof (a->ipv6));
    case holy_NET_NETWORK_LEVEL_PROTOCOL_DHCP_RECV:
      return 0;
    }
  holy_printf (_("Unsupported address type %d\n"), a->type);
  return 1;
}

/* FIXME: implement this. */
static char *
hwaddr_set_env (struct holy_env_var *var __attribute__ ((unused)),
		const char *val __attribute__ ((unused)))
{
  return NULL;
}

/* FIXME: implement this. */
static char *
addr_set_env (struct holy_env_var *var __attribute__ ((unused)),
	      const char *val __attribute__ ((unused)))
{
  return NULL;
}

static char *
defserver_set_env (struct holy_env_var *var __attribute__ ((unused)),
		   const char *val)
{
  holy_free (holy_net_default_server);
  holy_net_default_server = holy_strdup (val);
  return holy_strdup (val);
}

static const char *
defserver_get_env (struct holy_env_var *var __attribute__ ((unused)),
		   const char *val __attribute__ ((unused)))
{
  return holy_net_default_server ? : "";
}

static const char *
defip_get_env (struct holy_env_var *var __attribute__ ((unused)),
	       const char *val __attribute__ ((unused)))
{
  const char *intf = holy_env_get ("net_default_interface");
  const char *ret = NULL;
  if (intf)
    {
      char *buf = holy_xasprintf ("net_%s_ip", intf);
      if (buf)
	ret = holy_env_get (buf);
      holy_free (buf);
    }
  return ret;
}

static char *
defip_set_env (struct holy_env_var *var __attribute__ ((unused)),
	       const char *val)
{
  const char *intf = holy_env_get ("net_default_interface");
  if (intf)
    {
      char *buf = holy_xasprintf ("net_%s_ip", intf);
      if (buf)
	holy_env_set (buf, val);
      holy_free (buf);
    }
  return NULL;
}


static const char *
defmac_get_env (struct holy_env_var *var __attribute__ ((unused)),
	       const char *val __attribute__ ((unused)))
{
  const char *intf = holy_env_get ("net_default_interface");
  const char *ret = NULL;
  if (intf)
    {
      char *buf = holy_xasprintf ("net_%s_mac", intf);
      if (buf)
	ret = holy_env_get (buf);
      holy_free (buf);
    }
  return ret;
}

static char *
defmac_set_env (struct holy_env_var *var __attribute__ ((unused)),
	       const char *val)
{
  const char *intf = holy_env_get ("net_default_interface");
  if (intf)
    {
      char *buf = holy_xasprintf ("net_%s_mac", intf);
      if (buf)
	holy_env_set (buf, val);
      holy_free (buf);
    }
  return NULL;
}


static void
holy_net_network_level_interface_register (struct holy_net_network_level_interface *inter)
{
  {
    char buf[holy_NET_MAX_STR_HWADDR_LEN];
    char *name;
    char *ptr;
    holy_net_hwaddr_to_str (&inter->hwaddress, buf);
    name = holy_xasprintf ("net_%s_mac", inter->name);
    if (!name)
      return;
    for (ptr = name; *ptr; ptr++)
      if (*ptr == ':')
	*ptr = '_';    
    holy_env_set (name, buf);
    holy_register_variable_hook (name, 0, hwaddr_set_env);
    holy_env_export (name);
    holy_free (name);
  }

  {
    char buf[holy_NET_MAX_STR_ADDR_LEN];
    char *name;
    char *ptr;
    holy_net_addr_to_str (&inter->address, buf);
    name = holy_xasprintf ("net_%s_ip", inter->name);
    if (!name)
      return;
    for (ptr = name; *ptr; ptr++)
      if (*ptr == ':')
	*ptr = '_';    
    holy_env_set (name, buf);
    holy_register_variable_hook (name, 0, addr_set_env);
    holy_env_export (name);
    holy_free (name);
  }

  inter->card->num_ifaces++;
  inter->prev = &holy_net_network_level_interfaces;
  inter->next = holy_net_network_level_interfaces;
  if (inter->next)
    inter->next->prev = &inter->next;
  holy_net_network_level_interfaces = inter;
}


holy_err_t
holy_net_add_ipv4_local (struct holy_net_network_level_interface *inter,
			 int mask)
{
  holy_uint32_t ip_cpu;
  struct holy_net_route *route;

  if (inter->address.type != holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4)
    return 0;

  ip_cpu = holy_be_to_cpu32 (inter->address.ipv4);

  if (mask == -1)
    {
      if (!(ip_cpu & 0x80000000))
	mask = 8;
      else if (!(ip_cpu & 0x40000000))
	mask = 16;
      else if (!(ip_cpu & 0x20000000))
	mask = 24;
    }
  if (mask == -1)
    return 0;

  route = holy_zalloc (sizeof (*route));
  if (!route)
    return holy_errno;

  route->name = holy_xasprintf ("%s:local", inter->name);
  if (!route->name)
    {
      holy_free (route);
      return holy_errno;
    }

  route->target.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
  route->target.ipv4.base = holy_cpu_to_be32 (ip_cpu & (0xffffffff << (32 - mask)));
  route->target.ipv4.masksize = mask;
  route->is_gateway = 0;
  route->interface = inter;

  holy_net_route_register (route);

  return 0;
}

/* FIXME: support MAC specifying.  */
static holy_err_t
holy_cmd_addaddr (struct holy_command *cmd __attribute__ ((unused)),
		  int argc, char **args)
{
  struct holy_net_card *card;
  holy_net_network_level_address_t addr;
  holy_err_t err;
  holy_net_interface_flags_t flags = 0;
  struct holy_net_network_level_interface *inf;

  if (argc != 3)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("three arguments expected"));
  
  FOR_NET_CARDS (card)
    if (holy_strcmp (card->name, args[1]) == 0)
      break;
  if (card == NULL)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("card not found"));

  err = holy_net_resolve_address (args[2], &addr);
  if (err)
    return err;

  if (card->flags & holy_NET_CARD_NO_MANUAL_INTERFACES)
    return holy_error (holy_ERR_IO,
		       "this card doesn't support address addition");

  if (card->flags & holy_NET_CARD_HWADDRESS_IMMUTABLE)
    flags |= holy_NET_INTERFACE_HWADDRESS_IMMUTABLE;

  inf = holy_net_add_addr (args[0], card, &addr, &card->default_address,
			   flags);
  if (inf)
    holy_net_add_ipv4_local (inf, -1);

  return holy_errno;
}

static holy_err_t
holy_cmd_delroute (struct holy_command *cmd __attribute__ ((unused)),
		   int argc, char **args)
{
  struct holy_net_route *route;
  struct holy_net_route **prev;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));
  
  for (prev = &holy_net_routes, route = *prev; route; prev = &((*prev)->next),
	 route = *prev)
    if (holy_strcmp (route->name, args[0]) == 0)
      {
	*prev = route->next;
	holy_free (route->name);
	holy_free (route);
	if (!*prev)
	  break;
      }

  return holy_ERR_NONE;
}

holy_err_t
holy_net_add_route (const char *name,
		    holy_net_network_level_netaddress_t target,
		    struct holy_net_network_level_interface *inter)
{
  struct holy_net_route *route;

  route = holy_zalloc (sizeof (*route));
  if (!route)
    return holy_errno;

  route->name = holy_strdup (name);
  if (!route->name)
    {
      holy_free (route);
      return holy_errno;
    }

  route->target = target;
  route->is_gateway = 0;
  route->interface = inter;

  holy_net_route_register (route);

  return holy_ERR_NONE;
}

holy_err_t
holy_net_add_route_gw (const char *name,
		       holy_net_network_level_netaddress_t target,
		       holy_net_network_level_address_t gw,
		       struct holy_net_network_level_interface *inter)
{
  struct holy_net_route *route;

  route = holy_zalloc (sizeof (*route));
  if (!route)
    return holy_errno;

  route->name = holy_strdup (name);
  if (!route->name)
    {
      holy_free (route);
      return holy_errno;
    }

  route->target = target;
  route->is_gateway = 1;
  route->gw = gw;
  route->interface = inter;

  holy_net_route_register (route);

  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_addroute (struct holy_command *cmd __attribute__ ((unused)),
		  int argc, char **args)
{
  holy_net_network_level_netaddress_t target;
  if (argc < 3)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("three arguments expected"));

  holy_net_resolve_net_address  (args[1], &target);
  
  if (holy_strcmp (args[2], "gw") == 0 && argc >= 4)
    {
      holy_err_t err;
      holy_net_network_level_address_t gw;

      err = holy_net_resolve_address (args[3], &gw);
      if (err)
	return err;
      return holy_net_add_route_gw (args[0], target, gw, NULL);
    }
  else
    {
      struct holy_net_network_level_interface *inter;

      FOR_NET_NETWORK_LEVEL_INTERFACES (inter)
	if (holy_strcmp (inter->name, args[2]) == 0)
	  break;

      if (!inter)
	return holy_error (holy_ERR_BAD_ARGUMENT,
			   N_("unrecognised network interface `%s'"), args[2]);
      return holy_net_add_route (args[0], target, inter);
    }
}

static void
print_net_address (const holy_net_network_level_netaddress_t *target)
{
  switch (target->type)
    {
    case holy_NET_NETWORK_LEVEL_PROTOCOL_DHCP_RECV:
      /* TRANSLATORS: it refers to the network address.  */
      holy_printf ("%s\n", _("temporary"));
      return;
    case holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4:
      {
	holy_uint32_t n = holy_be_to_cpu32 (target->ipv4.base);
	holy_printf ("%d.%d.%d.%d/%d ", ((n >> 24) & 0xff),
		     ((n >> 16) & 0xff),
		     ((n >> 8) & 0xff),
		     ((n >> 0) & 0xff),
		     target->ipv4.masksize);
      }
      return;
    case holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6:
      {
	char buf[holy_NET_MAX_STR_ADDR_LEN];
	struct holy_net_network_level_address base;
	base.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6;
	holy_memcpy (&base.ipv6, &target->ipv6, 16);
	holy_net_addr_to_str (&base, buf);
	holy_printf ("%s/%d ", buf, target->ipv6.masksize);
      }
      return;
    }
  holy_printf (_("Unknown address type %d\n"), target->type);
}

static void
print_address (const holy_net_network_level_address_t *target)
{
  char buf[holy_NET_MAX_STR_ADDR_LEN];
  holy_net_addr_to_str (target, buf);
  holy_xputs (buf);
}

static holy_err_t
holy_cmd_listroutes (struct holy_command *cmd __attribute__ ((unused)),
		     int argc __attribute__ ((unused)),
		     char **args __attribute__ ((unused)))
{
  struct holy_net_route *route;
  FOR_NET_ROUTES(route)
  {
    holy_printf ("%s ", route->name);
    print_net_address (&route->target);
    if (route->is_gateway)
      {
	holy_printf ("gw ");
	print_address (&route->gw);	
      }
    else
      holy_printf ("%s", route->interface->name);
    holy_printf ("\n");
  }
  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_listcards (struct holy_command *cmd __attribute__ ((unused)),
		    int argc __attribute__ ((unused)),
		    char **args __attribute__ ((unused)))
{
  struct holy_net_card *card;
  FOR_NET_CARDS(card)
  {
    char buf[holy_NET_MAX_STR_HWADDR_LEN];
    holy_net_hwaddr_to_str (&card->default_address, buf);
    holy_printf ("%s %s\n", card->name, buf);
  }
  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_listaddrs (struct holy_command *cmd __attribute__ ((unused)),
		    int argc __attribute__ ((unused)),
		    char **args __attribute__ ((unused)))
{
  struct holy_net_network_level_interface *inf;
  FOR_NET_NETWORK_LEVEL_INTERFACES (inf)
  {
    char bufh[holy_NET_MAX_STR_HWADDR_LEN];
    char bufn[holy_NET_MAX_STR_ADDR_LEN];
    holy_net_hwaddr_to_str (&inf->hwaddress, bufh);
    holy_net_addr_to_str (&inf->address, bufn);
    holy_printf ("%s %s %s\n", inf->name, bufh, bufn);
  }
  return holy_ERR_NONE;
}

holy_net_app_level_t holy_net_app_level_list;
struct holy_net_socket *holy_net_sockets;

static holy_net_t
holy_net_open_real (const char *name)
{
  holy_net_app_level_t proto;
  const char *protname, *server;
  holy_size_t protnamelen;
  int try, port = 0;

  if (holy_strncmp (name, "pxe:", sizeof ("pxe:") - 1) == 0)
    {
      protname = "tftp";
      protnamelen = sizeof ("tftp") - 1;
      server = name + sizeof ("pxe:") - 1;
    }
  else if (holy_strcmp (name, "pxe") == 0)
    {
      protname = "tftp";
      protnamelen = sizeof ("tftp") - 1;
      server = holy_net_default_server;
    }
  else
    {
      const char *comma;
      char *colon;
      comma = holy_strchr (name, ',');
      if (!comma)
	{
	  comma = holy_strchr (name, ';');
	}
      colon = holy_strchr (name, ':');
      if (colon)
	{
	  port = (int) holy_strtol(colon+1, NULL, 10);
	  *colon = '\0';
	}
      if (comma)
	{
	  protnamelen = comma - name;
	  server = comma + 1;
	  protname = name;
	}
      else
	{
	  protnamelen = holy_strlen (name);
	  server = holy_net_default_server;
	  protname = name;
	}
    }
  if (!server)
    {
      holy_error (holy_ERR_NET_BAD_ADDRESS,
		  N_("no server is specified"));
      return NULL;
    }  

  for (try = 0; try < 2; try++)
    {
      FOR_NET_APP_LEVEL (proto)
      {
	if (holy_memcmp (proto->name, protname, protnamelen) == 0
	    && proto->name[protnamelen] == 0)
	  {
	    holy_net_t ret = holy_zalloc (sizeof (*ret));
	    if (!ret)
	      return NULL;
	    ret->protocol = proto;
	    ret->server = holy_strdup (server);
	    if (!ret->server)
	      {
		holy_free (ret);
		return NULL;
	      }
	    ret->port = port;
	    ret->fs = &holy_net_fs;
	    return ret;
	  }
      }
      if (try == 0)
	{
	  const char *prefix, *root;
	  char *prefdev, *comma;
	  int skip = 0;
	  holy_size_t devlen;

	  /* Do not attempt to load module if it requires protocol provided
	     by this module - it results in infinite recursion. Just continue,
	     fail and cleanup on next iteration.
	   */
	  prefix = holy_env_get ("prefix");
	  if (!prefix)
	    continue;

	  prefdev = holy_file_get_device_name (prefix);
	  if (!prefdev)
	    {
	      root = holy_env_get ("root");
	      if (!root)
		continue;
	      prefdev = holy_strdup (root);
	      if (!prefdev)
		continue;
	    }

	  if (holy_strncmp (prefdev, "pxe", sizeof ("pxe") - 1) == 0 &&
	      (!prefdev[sizeof ("pxe") - 1] || (prefdev[sizeof("pxe") - 1] == ':')))
	    {
	      holy_free (prefdev);
	      prefdev = holy_strdup ("tftp");
	      if (!prefdev)
		continue;
	    }

	  comma = holy_strchr (prefdev, ',');
	  if (comma)
	    *comma = '\0';
	  devlen = holy_strlen (prefdev);

	  if (protnamelen == devlen && holy_memcmp (prefdev, protname, devlen) == 0)
	    skip = 1;

	  holy_free (prefdev);

	  if (skip)
	    continue;

	  if (sizeof ("http") - 1 == protnamelen
	      && holy_memcmp ("http", protname, protnamelen) == 0)
	    {
	      holy_dl_load ("http");
	      holy_errno = holy_ERR_NONE;
	      continue;
	    }
	  if (sizeof ("tftp") - 1 == protnamelen
	      && holy_memcmp ("tftp", protname, protnamelen) == 0)
	    {
	      holy_dl_load ("tftp");
	      holy_errno = holy_ERR_NONE;
	      continue;
	    }
	}
      break;
    }

  /* Restore original error.  */
  holy_error (holy_ERR_UNKNOWN_DEVICE, N_("disk `%s' not found"),
	      name);

  return NULL;
}

static holy_err_t
holy_net_fs_dir (holy_device_t device, const char *path __attribute__ ((unused)),
		 holy_fs_dir_hook_t hook __attribute__ ((unused)),
		 void *hook_data __attribute__ ((unused)))
{
  if (!device->net)
    return holy_error (holy_ERR_BUG, "invalid net device");
  return holy_ERR_NONE;
}

static holy_err_t
holy_net_fs_open (struct holy_file *file_out, const char *name)
{
  holy_err_t err;
  struct holy_file *file, *bufio;

  file = holy_malloc (sizeof (*file));
  if (!file)
    return holy_errno;

  holy_memcpy (file, file_out, sizeof (struct holy_file));
  file->device->net->packs.first = NULL;
  file->device->net->packs.last = NULL;
  file->device->net->name = holy_strdup (name);
  if (!file->device->net->name)
    {
      holy_free (file);
      return holy_errno;
    }

  err = file->device->net->protocol->open (file, name);
  if (err)
    {
      while (file->device->net->packs.first)
	{
	  holy_netbuff_free (file->device->net->packs.first->nb);
	  holy_net_remove_packet (file->device->net->packs.first);
	}
      holy_free (file->device->net->name);
      holy_free (file);
      return err;
    }
  bufio = holy_bufio_open (file, 32768);
  if (! bufio)
    {
      while (file->device->net->packs.first)
	{
	  holy_netbuff_free (file->device->net->packs.first->nb);
	  holy_net_remove_packet (file->device->net->packs.first);
	}
      file->device->net->protocol->close (file);
      holy_free (file->device->net->name);
      holy_free (file);
      return holy_errno;
    }

  holy_memcpy (file_out, bufio, sizeof (struct holy_file));
  holy_free (bufio);
  return holy_ERR_NONE;
}

static holy_err_t
holy_net_fs_close (holy_file_t file)
{
  while (file->device->net->packs.first)
    {
      holy_netbuff_free (file->device->net->packs.first->nb);
      holy_net_remove_packet (file->device->net->packs.first);
    }
  file->device->net->protocol->close (file);
  holy_free (file->device->net->name);
  return holy_ERR_NONE;
}

static void
receive_packets (struct holy_net_card *card, int *stop_condition)
{
  int received = 0;
  if (card->num_ifaces == 0)
    return;
  if (!card->opened)
    {
      holy_err_t err = holy_ERR_NONE;
      if (card->driver->open)
	err = card->driver->open (card);
      if (err)
	{
	  holy_errno = holy_ERR_NONE;
	  return;
	}
      card->opened = 1;
    }
  while (received < 100)
    {
      /* Maybe should be better have a fixed number of packets for each card
	 and just mark them as used and not used.  */ 
      struct holy_net_buff *nb;

      if (received > 10 && stop_condition && *stop_condition)
	break;

      nb = card->driver->recv (card);
      if (!nb)
	{
	  card->last_poll = holy_get_time_ms ();
	  break;
	}
      received++;
      holy_net_recv_ethernet_packet (nb, card);
      if (holy_errno)
	{
	  holy_dprintf ("net", "error receiving: %d: %s\n", holy_errno,
			holy_errmsg);
	  holy_errno = holy_ERR_NONE;
	}
    }
  holy_print_error ();
}

static char *
holy_env_write_readonly (struct holy_env_var *var __attribute__ ((unused)),
			 const char *val __attribute__ ((unused)))
{
  return NULL;
}

holy_err_t
holy_env_set_net_property (const char *intername, const char *suffix,
                           const char *value, holy_size_t len)
{
  char *varname, *varvalue;
  char *ptr;

  varname = holy_xasprintf ("net_%s_%s", intername, suffix);
  if (!varname)
    return holy_errno;
  for (ptr = varname; *ptr; ptr++)
    if (*ptr == ':')
      *ptr = '_';
  varvalue = holy_malloc (len + 1);
  if (!varvalue)
    {
      holy_free (varname);
      return holy_errno;
    }

  holy_memcpy (varvalue, value, len);
  varvalue[len] = 0;
  holy_err_t ret = holy_env_set (varname, varvalue);
  holy_register_variable_hook (varname, 0, holy_env_write_readonly);
  holy_env_export (varname);
  holy_free (varname);
  holy_free (varvalue);

  return ret;
}

void
holy_net_poll_cards (unsigned time, int *stop_condition)
{
  struct holy_net_card *card;
  holy_uint64_t start_time;
  start_time = holy_get_time_ms ();
  while ((holy_get_time_ms () - start_time) < time
	 && (!stop_condition || !*stop_condition))
    FOR_NET_CARDS (card)
      receive_packets (card, stop_condition);
  holy_net_tcp_retransmit ();
}

static void
holy_net_poll_cards_idle_real (void)
{
  struct holy_net_card *card;
  FOR_NET_CARDS (card)
  {
    holy_uint64_t ctime = holy_get_time_ms ();

    if (ctime < card->last_poll
	|| ctime >= card->last_poll + card->idle_poll_delay_ms)
      receive_packets (card, 0);
  }
  holy_net_tcp_retransmit ();
}

/*  Read from the packets list*/
static holy_ssize_t
holy_net_fs_read_real (holy_file_t file, char *buf, holy_size_t len)
{
  holy_net_t net = file->device->net;
  struct holy_net_buff *nb;
  char *ptr = buf;
  holy_size_t amount, total = 0;
  int try = 0;

  while (try <= holy_NET_TRIES)
    {
      while (net->packs.first)
	{
	  try = 0;
	  nb = net->packs.first->nb;
	  amount = nb->tail - nb->data;
	  if (amount > len)
	    amount = len;
	  len -= amount;
	  total += amount;
	  file->device->net->offset += amount;
	  if (holy_file_progress_hook)
	    holy_file_progress_hook (0, 0, amount, file);
	  if (buf)
	    {
	      holy_memcpy (ptr, nb->data, amount);
	      ptr += amount;
	    }
	  if (amount == (holy_size_t) (nb->tail - nb->data))
	    {
	      holy_netbuff_free (nb);
	      holy_net_remove_packet (net->packs.first);
	    }
	  else
	    nb->data += amount;

	  if (!len)
	    {
	      if (net->protocol->packets_pulled)
		net->protocol->packets_pulled (file);
	      return total;
	    }
	}
      if (net->protocol->packets_pulled)
	net->protocol->packets_pulled (file);

      if (!net->eof)
	{
	  try++;
	  holy_net_poll_cards (holy_NET_INTERVAL +
                               (try * holy_NET_INTERVAL_ADDITION), &net->stall);
        }
      else
	return total;
    }
  holy_error (holy_ERR_TIMEOUT, N_("timeout reading `%s'"), net->name);
  return -1;
}

static holy_off_t
have_ahead (struct holy_file *file)
{
  holy_net_t net = file->device->net;
  holy_off_t ret = net->offset;
  struct holy_net_packet *pack;
  for (pack = net->packs.first; pack; pack = pack->next)
    ret += pack->nb->tail - pack->nb->data;
  return ret;
}

static holy_err_t
holy_net_seek_real (struct holy_file *file, holy_off_t offset)
{
  if (offset == file->device->net->offset)
    return holy_ERR_NONE;

  if (offset > file->device->net->offset)
    {
      if (!file->device->net->protocol->seek || have_ahead (file) >= offset)
	{
	  holy_net_fs_read_real (file, NULL,
				 offset - file->device->net->offset);
	  return holy_errno;
	}
      return file->device->net->protocol->seek (file, offset);
    }

  {
    holy_err_t err;
    if (file->device->net->protocol->seek)
      return file->device->net->protocol->seek (file, offset);
    while (file->device->net->packs.first)
      {
	holy_netbuff_free (file->device->net->packs.first->nb);
	holy_net_remove_packet (file->device->net->packs.first);
      }
    file->device->net->protocol->close (file);

    file->device->net->packs.first = NULL;
    file->device->net->packs.last = NULL;
    file->device->net->offset = 0;
    file->device->net->eof = 0;
    file->device->net->stall = 0;
    err = file->device->net->protocol->open (file, file->device->net->name);
    if (err)
      return err;
    holy_net_fs_read_real (file, NULL, offset);
    return holy_errno;
  }
}

static holy_ssize_t
holy_net_fs_read (holy_file_t file, char *buf, holy_size_t len)
{
  if (file->offset != file->device->net->offset)
    {
      holy_err_t err;
      err = holy_net_seek_real (file, file->offset);
      if (err)
	return err;
    }
  return holy_net_fs_read_real (file, buf, len);
}

static struct holy_fs holy_net_fs =
  {
    .name = "netfs",
    .dir = holy_net_fs_dir,
    .open = holy_net_fs_open,
    .read = holy_net_fs_read,
    .close = holy_net_fs_close,
    .label = NULL,
    .uuid = NULL,
    .mtime = NULL,
  };

static holy_err_t
holy_net_fini_hw (int noreturn __attribute__ ((unused)))
{
  struct holy_net_card *card;
  FOR_NET_CARDS (card) 
    if (card->opened)
      {
	if (card->driver->close)
	  card->driver->close (card);
	card->opened = 0;
      }
  return holy_ERR_NONE;
}

static holy_err_t
holy_net_restore_hw (void)
{
  return holy_ERR_NONE;
}

static struct holy_preboot *fini_hnd;

static holy_command_t cmd_addaddr, cmd_deladdr, cmd_addroute, cmd_delroute;
static holy_command_t cmd_lsroutes, cmd_lscards;
static holy_command_t cmd_lsaddr, cmd_slaac;

holy_MOD_INIT(net)
{
  holy_register_variable_hook ("net_default_server", defserver_get_env,
			       defserver_set_env);
  holy_env_export ("net_default_server");
  holy_register_variable_hook ("pxe_default_server", defserver_get_env,
			       defserver_set_env);
  holy_env_export ("pxe_default_server");
  holy_register_variable_hook ("net_default_ip", defip_get_env,
			       defip_set_env);
  holy_env_export ("net_default_ip");
  holy_register_variable_hook ("net_default_mac", defmac_get_env,
			       defmac_set_env);
  holy_env_export ("net_default_mac");

  cmd_addaddr = holy_register_command ("net_add_addr", holy_cmd_addaddr,
					/* TRANSLATORS: HWADDRESS stands for
					   "hardware address".  */
				       N_("SHORTNAME CARD ADDRESS [HWADDRESS]"),
				       N_("Add a network address."));
  cmd_slaac = holy_register_command ("net_ipv6_autoconf",
				     holy_cmd_ipv6_autoconf,
				     N_("[CARD [HWADDRESS]]"),
				     N_("Perform an IPV6 autoconfiguration"));

  cmd_deladdr = holy_register_command ("net_del_addr", holy_cmd_deladdr,
				       N_("SHORTNAME"),
				       N_("Delete a network address."));
  cmd_addroute = holy_register_command ("net_add_route", holy_cmd_addroute,
					/* TRANSLATORS: "gw" is a keyword.  */
					N_("SHORTNAME NET [INTERFACE| gw GATEWAY]"),
					N_("Add a network route."));
  cmd_delroute = holy_register_command ("net_del_route", holy_cmd_delroute,
					N_("SHORTNAME"),
					N_("Delete a network route."));
  cmd_lsroutes = holy_register_command ("net_ls_routes", holy_cmd_listroutes,
					"", N_("list network routes"));
  cmd_lscards = holy_register_command ("net_ls_cards", holy_cmd_listcards,
				       "", N_("list network cards"));
  cmd_lsaddr = holy_register_command ("net_ls_addr", holy_cmd_listaddrs,
				       "", N_("list network addresses"));
  holy_bootp_init ();
  holy_dns_init ();

  holy_net_open = holy_net_open_real;
  fini_hnd = holy_loader_register_preboot_hook (holy_net_fini_hw,
						holy_net_restore_hw,
						holy_LOADER_PREBOOT_HOOK_PRIO_DISK);
  holy_net_poll_cards_idle = holy_net_poll_cards_idle_real;
}

holy_MOD_FINI(net)
{
  holy_register_variable_hook ("net_default_server", 0, 0);
  holy_register_variable_hook ("pxe_default_server", 0, 0);

  holy_bootp_fini ();
  holy_dns_fini ();
  holy_unregister_command (cmd_addaddr);
  holy_unregister_command (cmd_deladdr);
  holy_unregister_command (cmd_addroute);
  holy_unregister_command (cmd_delroute);
  holy_unregister_command (cmd_lsroutes);
  holy_unregister_command (cmd_lscards);
  holy_unregister_command (cmd_lsaddr);
  holy_unregister_command (cmd_slaac);
  holy_fs_unregister (&holy_net_fs);
  holy_net_open = NULL;
  holy_net_fini_hw (0);
  holy_loader_unregister_preboot_hook (fini_hnd);
  holy_net_poll_cards_idle = holy_net_poll_cards_idle_real;
}
