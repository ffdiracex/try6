/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/net.h>
#include <holy/env.h>
#include <holy/i18n.h>
#include <holy/command.h>
#include <holy/net/ip.h>
#include <holy/net/netbuff.h>
#include <holy/net/udp.h>
#include <holy/datetime.h>

#if !defined(holy_MACHINE_EFI) && (defined(__i386__) || defined(__x86_64__))
#define holy_NET_BOOTP_ARCH 0x0000
#elif defined(holy_MACHINE_EFI) && defined(__x86_64__)
#define holy_NET_BOOTP_ARCH 0x0007
#elif defined(holy_MACHINE_EFI) && defined(__aarch64__)
#define holy_NET_BOOTP_ARCH 0x000B
#else
#error "unknown bootp architecture"
#endif

static holy_uint8_t dhcp_option_header[] = {holy_NET_BOOTP_RFC1048_MAGIC_0,
					    holy_NET_BOOTP_RFC1048_MAGIC_1,
					    holy_NET_BOOTP_RFC1048_MAGIC_2,
					    holy_NET_BOOTP_RFC1048_MAGIC_3};
static holy_uint8_t holy_userclass[] = {0x4D, 0x06, 0x05, 'G', 'R', 'U', 'B', '2'};
static holy_uint8_t holy_dhcpdiscover[] = {0x35, 0x01, 0x01};
static holy_uint8_t holy_dhcptime[] = {0x33, 0x04, 0x00, 0x00, 0x0e, 0x10};

static void
parse_dhcp_vendor (const char *name, const void *vend, int limit, int *mask)
{
  const holy_uint8_t *ptr, *ptr0;

  ptr = ptr0 = vend;

  if (ptr[0] != holy_NET_BOOTP_RFC1048_MAGIC_0
      || ptr[1] != holy_NET_BOOTP_RFC1048_MAGIC_1
      || ptr[2] != holy_NET_BOOTP_RFC1048_MAGIC_2
      || ptr[3] != holy_NET_BOOTP_RFC1048_MAGIC_3)
    return;
  ptr = ptr + sizeof (holy_uint32_t);
  while (ptr - ptr0 < limit)
    {
      holy_uint8_t tagtype;
      holy_uint8_t taglength;

      tagtype = *ptr++;

      /* Pad tag.  */
      if (tagtype == holy_NET_BOOTP_PAD)
	continue;

      /* End tag.  */
      if (tagtype == holy_NET_BOOTP_END)
	return;

      taglength = *ptr++;

      switch (tagtype)
	{
	case holy_NET_BOOTP_NETMASK:
	  if (taglength == 4)
	    {
	      int i;
	      for (i = 0; i < 32; i++)
		if (!(ptr[i / 8] & (1 << (7 - (i % 8)))))
		  break;
	      *mask = i;
	    }
	  break;

	case holy_NET_BOOTP_ROUTER:
	  if (taglength == 4)
	    {
	      holy_net_network_level_netaddress_t target;
	      holy_net_network_level_address_t gw;
	      char *rname;
	      
	      target.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
	      target.ipv4.base = 0;
	      target.ipv4.masksize = 0;
	      gw.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
	      holy_memcpy (&gw.ipv4, ptr, sizeof (gw.ipv4));
	      rname = holy_xasprintf ("%s:default", name);
	      if (rname)
		holy_net_add_route_gw (rname, target, gw, NULL);
	      holy_free (rname);
	    }
	  break;
	case holy_NET_BOOTP_DNS:
	  {
	    int i;
	    for (i = 0; i < taglength / 4; i++)
	      {
		struct holy_net_network_level_address s;
		s.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
		s.ipv4 = holy_get_unaligned32 (ptr);
		s.option = DNS_OPTION_PREFER_IPV4;
		holy_net_add_dns_server (&s);
		ptr += 4;
	      }
	  }
	  continue;
	case holy_NET_BOOTP_HOSTNAME:
          holy_env_set_net_property (name, "hostname", (const char *) ptr,
                                     taglength);
          break;

	case holy_NET_BOOTP_DOMAIN:
          holy_env_set_net_property (name, "domain", (const char *) ptr,
                                     taglength);
          break;

	case holy_NET_BOOTP_ROOT_PATH:
          holy_env_set_net_property (name, "rootpath", (const char *) ptr,
                                     taglength);
          break;

	case holy_NET_BOOTP_EXTENSIONS_PATH:
          holy_env_set_net_property (name, "extensionspath", (const char *) ptr,
                                     taglength);
          break;

	  /* If you need any other options please contact holy
	     development team.  */
	}

      ptr += taglength;
    }
}

#define OFFSET_OF(x, y) ((holy_size_t)((holy_uint8_t *)((y)->x) - (holy_uint8_t *)(y)))

struct holy_net_network_level_interface *
holy_net_configure_by_dhcp_ack (const char *name,
				struct holy_net_card *card,
				holy_net_interface_flags_t flags,
				const struct holy_net_bootp_packet *bp,
				holy_size_t size,
				int is_def, char **device, char **path)
{
  holy_net_network_level_address_t addr;
  holy_net_link_level_address_t hwaddr;
  struct holy_net_network_level_interface *inter;
  int mask = -1;
  char server_ip[sizeof ("xxx.xxx.xxx.xxx")];

  addr.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
  addr.ipv4 = bp->your_ip;

  if (device)
    *device = 0;
  if (path)
    *path = 0;

  holy_memcpy (hwaddr.mac, bp->mac_addr,
	       bp->hw_len < sizeof (hwaddr.mac) ? bp->hw_len
	       : sizeof (hwaddr.mac));
  hwaddr.type = holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET;

  inter = holy_net_add_addr (name, card, &addr, &hwaddr, flags);
  if (!inter)
    return 0;

#if 0
  /* This is likely based on misunderstanding. gateway_ip refers to
     address of BOOTP relay and should not be used after BOOTP transaction
     is complete.
     See RFC1542, 3.4 Interpretation of the 'giaddr' field
   */
  if (bp->gateway_ip)
    {
      holy_net_network_level_netaddress_t target;
      holy_net_network_level_address_t gw;
      char *rname;
	  
      target.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
      target.ipv4.base = bp->server_ip;
      target.ipv4.masksize = 32;
      gw.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
      gw.ipv4 = bp->gateway_ip;
      rname = holy_xasprintf ("%s:gw", name);
      if (rname)
	holy_net_add_route_gw (rname, target, gw);
      holy_free (rname);

      target.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
      target.ipv4.base = bp->gateway_ip;
      target.ipv4.masksize = 32;
      holy_net_add_route (name, target, inter);
    }
#endif

  if (size > OFFSET_OF (boot_file, bp))
    holy_env_set_net_property (name, "boot_file", bp->boot_file,
                               sizeof (bp->boot_file));
  if (bp->server_ip)
    {
      holy_snprintf (server_ip, sizeof (server_ip), "%d.%d.%d.%d",
		     ((holy_uint8_t *) &bp->server_ip)[0],
		     ((holy_uint8_t *) &bp->server_ip)[1],
		     ((holy_uint8_t *) &bp->server_ip)[2],
		     ((holy_uint8_t *) &bp->server_ip)[3]);
      holy_env_set_net_property (name, "next_server", server_ip, sizeof (server_ip));
      holy_print_error ();
    }

  if (is_def)
    holy_net_default_server = 0;
  if (is_def && !holy_net_default_server && bp->server_ip)
    {
      holy_net_default_server = holy_strdup (server_ip);
      holy_print_error ();
    }

  if (is_def)
    {
      holy_env_set ("net_default_interface", name);
      holy_env_export ("net_default_interface");
    }

  if (device && !*device && bp->server_ip)
    {
      *device = holy_xasprintf ("tftp,%s", server_ip);
      holy_print_error ();
    }
  if (size > OFFSET_OF (server_name, bp)
      && bp->server_name[0])
    {
      holy_env_set_net_property (name, "dhcp_server_name", bp->server_name,
                                 sizeof (bp->server_name));
      if (is_def && !holy_net_default_server)
	{
	  holy_net_default_server = holy_strdup (bp->server_name);
	  holy_print_error ();
	}
      if (device && !*device)
	{
	  *device = holy_xasprintf ("tftp,%s", bp->server_name);
	  holy_print_error ();
	}
    }

  if (size > OFFSET_OF (boot_file, bp) && path)
    {
      *path = holy_strndup (bp->boot_file, sizeof (bp->boot_file));
      holy_print_error ();
      if (*path)
	{
	  char *slash;
	  slash = holy_strrchr (*path, '/');
	  if (slash)
	    *slash = 0;
	  else
	    **path = 0;
	}
    }
  if (size > OFFSET_OF (vendor, bp))
    parse_dhcp_vendor (name, &bp->vendor, size - OFFSET_OF (vendor, bp), &mask);
  holy_net_add_ipv4_local (inter, mask);
  
  inter->dhcp_ack = holy_malloc (size);
  if (inter->dhcp_ack)
    {
      holy_memcpy (inter->dhcp_ack, bp, size);
      inter->dhcp_acklen = size;
    }
  else
    holy_errno = holy_ERR_NONE;

  return inter;
}

void
holy_net_process_dhcp (struct holy_net_buff *nb,
		       struct holy_net_card *card)
{
  char *name;
  struct holy_net_network_level_interface *inf;

  name = holy_xasprintf ("%s:dhcp", card->name);
  if (!name)
    {
      holy_print_error ();
      return;
    }
  holy_net_configure_by_dhcp_ack (name, card,
				  0, (const struct holy_net_bootp_packet *) nb->data,
				  (nb->tail - nb->data), 0, 0, 0);
  holy_free (name);
  if (holy_errno)
    holy_print_error ();
  else
    {
      FOR_NET_NETWORK_LEVEL_INTERFACES(inf)
	if (holy_memcmp (inf->name, card->name, holy_strlen (card->name)) == 0
	    && holy_memcmp (inf->name + holy_strlen (card->name),
			    ":dhcp_tmp", sizeof (":dhcp_tmp") - 1) == 0)
	  {
	    holy_net_network_level_interface_unregister (inf);
	    break;
	  }
    }
}

static char
hexdigit (holy_uint8_t val)
{
  if (val < 10)
    return val + '0';
  return val + 'a' - 10;
}

static holy_err_t
holy_cmd_dhcpopt (struct holy_command *cmd __attribute__ ((unused)),
		  int argc, char **args)
{
  struct holy_net_network_level_interface *inter;
  int num;
  holy_uint8_t *ptr;
  holy_uint8_t taglength;

  if (argc < 4)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("four arguments expected"));

  FOR_NET_NETWORK_LEVEL_INTERFACES (inter)
    if (holy_strcmp (inter->name, args[1]) == 0)
      break;

  if (!inter)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("unrecognised network interface `%s'"), args[1]);

  if (!inter->dhcp_ack)
    return holy_error (holy_ERR_IO, N_("no DHCP info found"));

  if (inter->dhcp_acklen <= OFFSET_OF (vendor, inter->dhcp_ack))
    return holy_error (holy_ERR_IO, N_("no DHCP options found"));

  num = holy_strtoul (args[2], 0, 0);
  if (holy_errno)
    return holy_errno;

  ptr = inter->dhcp_ack->vendor;

  if (ptr[0] != holy_NET_BOOTP_RFC1048_MAGIC_0
      || ptr[1] != holy_NET_BOOTP_RFC1048_MAGIC_1
      || ptr[2] != holy_NET_BOOTP_RFC1048_MAGIC_2
      || ptr[3] != holy_NET_BOOTP_RFC1048_MAGIC_3)
    return holy_error (holy_ERR_IO, N_("no DHCP options found"));
  ptr = ptr + sizeof (holy_uint32_t);
  while (1)
    {
      holy_uint8_t tagtype;

      if (ptr >= ((holy_uint8_t *) inter->dhcp_ack) + inter->dhcp_acklen)
	return holy_error (holy_ERR_IO, N_("no DHCP option %d found"), num);

      tagtype = *ptr++;

      /* Pad tag.  */
      if (tagtype == 0)
	continue;

      /* End tag.  */
      if (tagtype == 0xff)
	return holy_error (holy_ERR_IO, N_("no DHCP option %d found"), num);

      taglength = *ptr++;
	
      if (tagtype == num)
	break;
      ptr += taglength;
    }

  if (holy_strcmp (args[3], "string") == 0)
    {
      holy_err_t err = holy_ERR_NONE;
      char *val = holy_malloc (taglength + 1);
      if (!val)
	return holy_errno;
      holy_memcpy (val, ptr, taglength);
      val[taglength] = 0;
      if (args[0][0] == '-' && args[0][1] == 0)
	holy_printf ("%s\n", val);
      else
	err = holy_env_set (args[0], val);
      holy_free (val);
      return err;
    }

  if (holy_strcmp (args[3], "number") == 0)
    {
      holy_uint64_t val = 0;
      int i;
      for (i = 0; i < taglength; i++)
	val = (val << 8) | ptr[i];
      if (args[0][0] == '-' && args[0][1] == 0)
	holy_printf ("%llu\n", (unsigned long long) val);
      else
	{
	  char valn[64];
	  holy_snprintf (valn, sizeof (valn), "%lld\n", (unsigned long long) val);
	  return holy_env_set (args[0], valn);
	}
      return holy_ERR_NONE;
    }

  if (holy_strcmp (args[3], "hex") == 0)
    {
      holy_err_t err = holy_ERR_NONE;
      char *val = holy_malloc (2 * taglength + 1);
      int i;
      if (!val)
	return holy_errno;
      for (i = 0; i < taglength; i++)
	{
	  val[2 * i] = hexdigit (ptr[i] >> 4);
	  val[2 * i + 1] = hexdigit (ptr[i] & 0xf);
	}
      val[2 * taglength] = 0;
      if (args[0][0] == '-' && args[0][1] == 0)
	holy_printf ("%s\n", val);
      else
	err = holy_env_set (args[0], val);
      holy_free (val);
      return err;
    }

  return holy_error (holy_ERR_BAD_ARGUMENT,
		     N_("unrecognised DHCP option format specification `%s'"),
		     args[3]);
}

/* FIXME: allow to specify mac address.  */
static holy_err_t
holy_cmd_bootp (struct holy_command *cmd __attribute__ ((unused)),
		int argc, char **args)
{
  struct holy_net_card *card;
  struct holy_net_network_level_interface *ifaces;
  holy_size_t ncards = 0;
  unsigned j = 0;
  int interval;
  holy_err_t err;

  FOR_NET_CARDS (card)
  {
    if (argc > 0 && holy_strcmp (card->name, args[0]) != 0)
      continue;
    ncards++;
  }

  if (ncards == 0)
    return holy_error (holy_ERR_NET_NO_CARD, N_("no network card found"));

  ifaces = holy_zalloc (ncards * sizeof (ifaces[0]));
  if (!ifaces)
    return holy_errno;

  j = 0;
  FOR_NET_CARDS (card)
  {
    if (argc > 0 && holy_strcmp (card->name, args[0]) != 0)
      continue;
    ifaces[j].card = card;
    ifaces[j].next = &ifaces[j+1];
    if (j)
      ifaces[j].prev = &ifaces[j-1].next;
    ifaces[j].name = holy_xasprintf ("%s:dhcp_tmp", card->name);
    card->num_ifaces++;
    if (!ifaces[j].name)
      {
	unsigned i;
	for (i = 0; i < j; i++)
	  holy_free (ifaces[i].name);
	holy_free (ifaces);
	return holy_errno;
      }
    ifaces[j].address.type = holy_NET_NETWORK_LEVEL_PROTOCOL_DHCP_RECV;
    holy_memcpy (&ifaces[j].hwaddress, &card->default_address,
		 sizeof (ifaces[j].hwaddress));
    j++;
  }
  ifaces[ncards - 1].next = holy_net_network_level_interfaces;
  if (holy_net_network_level_interfaces)
    holy_net_network_level_interfaces->prev = & ifaces[ncards - 1].next;
  holy_net_network_level_interfaces = &ifaces[0];
  ifaces[0].prev = &holy_net_network_level_interfaces;
  for (interval = 200; interval < 10000; interval *= 2)
    {
      int done = 0;
      for (j = 0; j < ncards; j++)
	{
	  struct holy_net_bootp_packet *pack;
	  struct holy_datetime date;
	  holy_int32_t t = 0;
	  struct holy_net_buff *nb;
	  struct udphdr *udph;
	  holy_net_network_level_address_t target;
	  holy_net_link_level_address_t ll_target;
	  holy_uint8_t *offset;

	  if (!ifaces[j].prev)
	    continue;
	  nb = holy_netbuff_alloc (sizeof (*pack) + sizeof(dhcp_option_header)
				   + sizeof(holy_userclass)
				   + sizeof(holy_dhcpdiscover)
				   + sizeof(holy_dhcptime) + 64 + 128);
	  if (!nb)
	    {
	      holy_netbuff_free (nb);
	      return holy_errno;
	    }
	  err = holy_netbuff_reserve (nb, sizeof (*pack) + 64 + 128);
	  if (err)
	    {
	      holy_netbuff_free (nb);
	      return err;
	    }
	  err = holy_netbuff_push (nb, sizeof (*pack) + 64);
	  if (err)
	    {
	      holy_netbuff_free (nb);
	      return err;
	    }
	  pack = (void *) nb->data;
	  done = 1;
	  holy_memset (pack, 0, sizeof (*pack) + 64);
	  pack->opcode = 1;
	  pack->hw_type = 1;
	  pack->hw_len = 6;
	  err = holy_get_datetime (&date);
	  if (err || !holy_datetime2unixtime (&date, &t))
	    {
	      holy_errno = holy_ERR_NONE;
	      t = 0;
	    }
	  pack->ident = holy_cpu_to_be32 (t);
	  pack->seconds = holy_cpu_to_be16 (t);

	  holy_memcpy (&pack->mac_addr, &ifaces[j].hwaddress.mac, 6);
	  offset = (holy_uint8_t *)&pack->vendor;
	  holy_memcpy (offset, dhcp_option_header, sizeof(dhcp_option_header));
	  offset += sizeof(dhcp_option_header);
	  holy_memcpy (offset, holy_dhcpdiscover, sizeof(holy_dhcpdiscover));
	  offset += sizeof(holy_dhcpdiscover);
	  holy_memcpy (offset, holy_userclass, sizeof(holy_userclass));
	  offset += sizeof(holy_userclass);
	  holy_memcpy (offset, holy_dhcptime, sizeof(holy_dhcptime));

	  /* insert Client System Architecture (option 93) */
	  offset += sizeof(holy_dhcptime);
	  offset[0] = 93;
	  offset[1] = 2;
	  offset[2] = (holy_NET_BOOTP_ARCH >> 8);
	  offset[3] = (holy_NET_BOOTP_ARCH & 0xFF);

	  /* option terminator */
	  offset[4] = 255;

	  holy_netbuff_push (nb, sizeof (*udph));

	  udph = (struct udphdr *) nb->data;
	  udph->src = holy_cpu_to_be16_compile_time (68);
	  udph->dst = holy_cpu_to_be16_compile_time (67);
	  udph->chksum = 0;
	  udph->len = holy_cpu_to_be16 (nb->tail - nb->data);
	  target.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
	  target.ipv4 = 0xffffffff;
	  err = holy_net_link_layer_resolve (&ifaces[j], &target, &ll_target);
	  if (err)
	    return err;

	  udph->chksum = holy_net_ip_transport_checksum (nb, holy_NET_IP_UDP,
							 &ifaces[j].address,
							 &target);

	  err = holy_net_send_ip_packet (&ifaces[j], &target, &ll_target, nb,
					 holy_NET_IP_UDP);
	  holy_netbuff_free (nb);
	  if (err)
	    return err;
	}
      if (!done)
	break;
      holy_net_poll_cards (interval, 0);
    }

  err = holy_ERR_NONE;
  for (j = 0; j < ncards; j++)
    {
      holy_free (ifaces[j].name);
      if (!ifaces[j].prev)
	continue;
      holy_error_push ();
      holy_net_network_level_interface_unregister (&ifaces[j]);
      err = holy_error (holy_ERR_FILE_NOT_FOUND,
			N_("couldn't autoconfigure %s"),
			ifaces[j].card->name);
    }

  holy_free (ifaces);
  return err;
}

static holy_command_t cmd_getdhcp, cmd_bootp;

void
holy_bootp_init (void)
{
  cmd_bootp = holy_register_command ("net_bootp", holy_cmd_bootp,
				     N_("[CARD]"),
				     N_("perform a bootp autoconfiguration"));
  cmd_getdhcp = holy_register_command ("net_get_dhcp_option", holy_cmd_dhcpopt,
				       N_("VAR INTERFACE NUMBER DESCRIPTION"),
				       N_("retrieve DHCP option and save it into VAR. If VAR is - then print the value."));
}

void
holy_bootp_fini (void)
{
  holy_unregister_command (cmd_getdhcp);
  holy_unregister_command (cmd_bootp);
}
