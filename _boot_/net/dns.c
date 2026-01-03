/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/net.h>
#include <holy/net/udp.h>
#include <holy/command.h>
#include <holy/i18n.h>
#include <holy/err.h>
#include <holy/time.h>

struct dns_cache_element
{
  char *name;
  holy_size_t naddresses;
  struct holy_net_network_level_address *addresses;
  holy_uint64_t limit_time;
};

#define DNS_CACHE_SIZE 1021
#define DNS_HASH_BASE 423

typedef enum holy_dns_qtype_id
  {
    holy_DNS_QTYPE_A = 1,
    holy_DNS_QTYPE_AAAA = 28
  } holy_dns_qtype_id_t;

static struct dns_cache_element dns_cache[DNS_CACHE_SIZE];
static struct holy_net_network_level_address *dns_servers;
static holy_size_t dns_nservers, dns_servers_alloc;

holy_err_t
holy_net_add_dns_server (const struct holy_net_network_level_address *s)
{
  if (dns_servers_alloc <= dns_nservers)
    {
      int na = dns_servers_alloc * 2;
      struct holy_net_network_level_address *ns;
      if (na < 8)
	na = 8;
      ns = holy_realloc (dns_servers, na * sizeof (ns[0]));
      if (!ns)
	return holy_errno;
      dns_servers_alloc = na;
      dns_servers = ns;
    }
  dns_servers[dns_nservers++] = *s;
  return holy_ERR_NONE;
}

void
holy_net_remove_dns_server (const struct holy_net_network_level_address *s)
{
  holy_size_t i;
  for (i = 0; i < dns_nservers; i++)
    if (holy_net_addr_cmp (s, &dns_servers[i]) == 0)
      break;
  if (i < dns_nservers)
    {
      dns_servers[i] = dns_servers[dns_nservers - 1];
      dns_nservers--;
    }
}

struct dns_header
{
  holy_uint16_t id;
  holy_uint8_t flags;
  holy_uint8_t ra_z_r_code;
  holy_uint16_t qdcount;
  holy_uint16_t ancount;
  holy_uint16_t nscount;
  holy_uint16_t arcount;
} holy_PACKED;

enum
  {
    FLAGS_RESPONSE = 0x80,
    FLAGS_OPCODE = 0x78,
    FLAGS_RD = 0x01
  };

enum
  {
    ERRCODE_MASK = 0x0f
  };

enum
  {
    DNS_PORT = 53
  };

struct recv_data
{
  holy_size_t *naddresses;
  struct holy_net_network_level_address **addresses;
  int cache;
  holy_uint16_t id;
  int dns_err;
  char *name;
  const char *oname;
  int stop;
};

static inline int
hash (const char *str)
{
  unsigned v = 0, xn = 1;
  const char *ptr;
  for (ptr = str; *ptr; )
    {
      v = (v + xn * *ptr);
      xn = (DNS_HASH_BASE * xn) % DNS_CACHE_SIZE;
      ptr++;
      if (((ptr - str) & 0x3ff) == 0)
	v %= DNS_CACHE_SIZE;
    }
  return v % DNS_CACHE_SIZE;
}

static int
check_name_real (const holy_uint8_t *name_at, const holy_uint8_t *head,
		 const holy_uint8_t *tail, const char *check_with,
		 int *length, char *set)
{
  const char *readable_ptr = check_with;
  const holy_uint8_t *ptr;
  char *optr = set;
  int bytes_processed = 0;
  if (length)
    *length = 0;
  for (ptr = name_at; ptr < tail && bytes_processed < tail - head + 2; )
    {
      /* End marker.  */
      if (!*ptr)
	{
	  if (length && *length)
	    (*length)--;
	  if (optr && optr != set)
	    optr--;
	  if (optr)
	    *optr = 0;
	  return !readable_ptr || (*readable_ptr == 0);
	}
      if (*ptr & 0xc0)
	{
	  bytes_processed += 2;
	  if (ptr + 1 >= tail)
	    return 0;
	  ptr = head + (((ptr[0] & 0x3f) << 8) | ptr[1]);
	  continue;
	}
      if (readable_ptr && holy_memcmp (ptr + 1, readable_ptr, *ptr) != 0)
	return 0;
      if (holy_memchr (ptr + 1, 0, *ptr)
	  || holy_memchr (ptr + 1, '.', *ptr))
	return 0;
      if (readable_ptr)
	readable_ptr += *ptr;
      if (readable_ptr && *readable_ptr != '.' && *readable_ptr != 0)
	return 0;
      bytes_processed += *ptr + 1;
      if (length)
	*length += *ptr + 1;
      if (optr)
	{
	  holy_memcpy (optr, ptr + 1, *ptr);
	  optr += *ptr;
	}
      if (optr)
	*optr++ = '.';
      if (readable_ptr && *readable_ptr)
	readable_ptr++;
      ptr += *ptr + 1;
    }
  return 0;
}

static int
check_name (const holy_uint8_t *name_at, const holy_uint8_t *head,
	    const holy_uint8_t *tail, const char *check_with)
{
  return check_name_real (name_at, head, tail, check_with, NULL, NULL);
}

static char *
get_name (const holy_uint8_t *name_at, const holy_uint8_t *head,
	  const holy_uint8_t *tail)
{
  int length;
  char *ret;

  if (!check_name_real (name_at, head, tail, NULL, &length, NULL))
    return NULL;
  ret = holy_malloc (length + 1);
  if (!ret)
    return NULL;
  if (!check_name_real (name_at, head, tail, NULL, NULL, ret))
    {
      holy_free (ret);
      return NULL;
    }
  return ret;
}

enum
  {
    DNS_CLASS_A = 1,
    DNS_CLASS_CNAME = 5,
    DNS_CLASS_AAAA = 28
  };

static holy_err_t
recv_hook (holy_net_udp_socket_t sock __attribute__ ((unused)),
	   struct holy_net_buff *nb,
	   void *data_)
{
  struct dns_header *head;
  struct recv_data *data = data_;
  int i, j;
  holy_uint8_t *ptr, *reparse_ptr;
  int redirect_cnt = 0;
  char *redirect_save = NULL;
  holy_uint32_t ttl_all = ~0U;

  /* Code apparently assumed that only one packet is received as response.
     We may get multiple responses due to network condition, so check here
     and quit early. */
  if (*data->addresses)
    {
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }

  head = (struct dns_header *) nb->data;
  ptr = (holy_uint8_t *) (head + 1);
  if (ptr >= nb->tail)
    {
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }
  
  if (head->id != data->id)
    {
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }
  if (!(head->flags & FLAGS_RESPONSE) || (head->flags & FLAGS_OPCODE))
    {
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }
  if (head->ra_z_r_code & ERRCODE_MASK)
    {
      data->dns_err = 1;
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }
  for (i = 0; i < holy_be_to_cpu16 (head->qdcount); i++)
    {
      if (ptr >= nb->tail)
	{
	  holy_netbuff_free (nb);
	  return holy_ERR_NONE;
	}
      while (ptr < nb->tail && !((*ptr & 0xc0) || *ptr == 0))
	ptr += *ptr + 1;
      if (ptr < nb->tail && (*ptr & 0xc0))
	ptr++;
      ptr++;
      ptr += 4;
    }
  *data->addresses = holy_realloc (*data->addresses, sizeof ((*data->addresses)[0])
		     * (holy_be_to_cpu16 (head->ancount) + *data->naddresses));
  if (!*data->addresses)
    {
      holy_errno = holy_ERR_NONE;
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }
  reparse_ptr = ptr;
 reparse:
  for (i = 0, ptr = reparse_ptr; i < holy_be_to_cpu16 (head->ancount); i++)
    {
      int ignored = 0;
      holy_uint8_t class;
      holy_uint32_t ttl = 0;
      holy_uint16_t length;
      if (ptr >= nb->tail)
	{
	  if (!*data->naddresses)
	    holy_free (*data->addresses);
	  return holy_ERR_NONE;
	}
      ignored = !check_name (ptr, nb->data, nb->tail, data->name);
      while (ptr < nb->tail && !((*ptr & 0xc0) || *ptr == 0))
	ptr += *ptr + 1;
      if (ptr < nb->tail && (*ptr & 0xc0))
	ptr++;
      ptr++;
      if (ptr + 10 >= nb->tail)
	{
	  if (!*data->naddresses)
	    holy_free (*data->addresses);
	  holy_netbuff_free (nb);
	  return holy_ERR_NONE;
	}
      if (*ptr++ != 0)
	ignored = 1;
      class = *ptr++;
      if (*ptr++ != 0)
	ignored = 1;
      if (*ptr++ != 1)
	ignored = 1;
      for (j = 0; j < 4; j++)
	{
	  ttl <<= 8;
	  ttl |= *ptr++;
	}
      length = *ptr++ << 8;
      length |= *ptr++;
      if (ptr + length > nb->tail)
	{
	  if (!*data->naddresses)
	    holy_free (*data->addresses);
	  holy_netbuff_free (nb);
	  return holy_ERR_NONE;
	}
      if (!ignored)
	{
	  if (ttl_all > ttl)
	    ttl_all = ttl;
	  switch (class)
	    {
	    case DNS_CLASS_A:
	      if (length != 4)
		break;
	      (*data->addresses)[*data->naddresses].type
		= holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
	      holy_memcpy (&(*data->addresses)[*data->naddresses].ipv4,
			   ptr, 4);
	      (*data->naddresses)++;
	      data->stop = 1;
	      break;
	    case DNS_CLASS_AAAA:
	      if (length != 16)
		break;
	      (*data->addresses)[*data->naddresses].type
		= holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6;
	      holy_memcpy (&(*data->addresses)[*data->naddresses].ipv6,
			   ptr, 16);
	      (*data->naddresses)++;
	      data->stop = 1;
	      break;
	    case DNS_CLASS_CNAME:
	      if (!(redirect_cnt & (redirect_cnt - 1)))
		{
		  holy_free (redirect_save);
		  redirect_save = data->name;
		}
	      else
		holy_free (data->name);
	      redirect_cnt++;
	      data->name = get_name (ptr, nb->data, nb->tail);
	      if (!data->name)
		{
		  data->dns_err = 1;
		  holy_errno = 0;
		  return holy_ERR_NONE;
		}
	      holy_dprintf ("dns", "CNAME %s\n", data->name);
	      if (holy_strcmp (redirect_save, data->name) == 0)
		{
		  data->dns_err = 1;
		  holy_free (redirect_save);
		  return holy_ERR_NONE;
		}
	      goto reparse;
	    }
	}
      ptr += length;
    }
  if (ttl_all && *data->naddresses && data->cache)
    {
      int h;
      holy_dprintf ("dns", "caching for %d seconds\n", ttl_all);
      h = hash (data->oname);
      holy_free (dns_cache[h].name);
      dns_cache[h].name = 0;
      holy_free (dns_cache[h].addresses);
      dns_cache[h].addresses = 0;
      dns_cache[h].name = holy_strdup (data->oname);
      dns_cache[h].naddresses = *data->naddresses;
      dns_cache[h].addresses = holy_malloc (*data->naddresses
					    * sizeof (dns_cache[h].addresses[0]));
      dns_cache[h].limit_time = holy_get_time_ms () + 1000 * ttl_all;
      if (!dns_cache[h].addresses || !dns_cache[h].name)
	{
	  holy_free (dns_cache[h].name);
	  dns_cache[h].name = 0;
	  holy_free (dns_cache[h].addresses);
	  dns_cache[h].addresses = 0;
	}
      holy_memcpy (dns_cache[h].addresses, *data->addresses,
		   *data->naddresses
		   * sizeof (dns_cache[h].addresses[0]));
    }
  holy_netbuff_free (nb);
  holy_free (redirect_save);
  return holy_ERR_NONE;
}

holy_err_t
holy_net_dns_lookup (const char *name,
		     const struct holy_net_network_level_address *servers,
		     holy_size_t n_servers,
		     holy_size_t *naddresses,
		     struct holy_net_network_level_address **addresses,
		     int cache)
{
  holy_size_t send_servers = 0;
  holy_size_t i, j;
  struct holy_net_buff *nb;
  holy_net_udp_socket_t *sockets;
  holy_uint8_t *optr;
  const char *iptr;
  struct dns_header *head;
  static holy_uint16_t id = 1;
  holy_uint8_t *qtypeptr;
  holy_err_t err = holy_ERR_NONE;
  struct recv_data data = {naddresses, addresses, cache,
			   holy_cpu_to_be16 (id++), 0, 0, name, 0};
  holy_uint8_t *nbd;
  holy_size_t try_server = 0;

  if (!servers)
    {
      servers = dns_servers;
      n_servers = dns_nservers;
    }

  if (!n_servers)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("no DNS servers configured"));

  *naddresses = 0;
  if (cache)
    {
      int h;
      h = hash (name);
      if (dns_cache[h].name && holy_strcmp (dns_cache[h].name, name) == 0
	  && holy_get_time_ms () < dns_cache[h].limit_time)
	{
	  holy_dprintf ("dns", "retrieved from cache\n");
	  *addresses = holy_malloc (dns_cache[h].naddresses
				    * sizeof ((*addresses)[0]));
	  if (!*addresses)
	    return holy_errno;
	  *naddresses = dns_cache[h].naddresses;
	  holy_memcpy (*addresses, dns_cache[h].addresses,
		       dns_cache[h].naddresses
		       * sizeof ((*addresses)[0]));
	  return holy_ERR_NONE;
	}
    }

  sockets = holy_malloc (sizeof (sockets[0]) * n_servers);
  if (!sockets)
    return holy_errno;

  data.name = holy_strdup (name);
  if (!data.name)
    {
      holy_free (sockets);
      return holy_errno;
    }

  nb = holy_netbuff_alloc (holy_NET_OUR_MAX_IP_HEADER_SIZE
			   + holy_NET_MAX_LINK_HEADER_SIZE
			   + holy_NET_UDP_HEADER_SIZE
			   + sizeof (struct dns_header)
			   + holy_strlen (name) + 2 + 4);
  if (!nb)
    {
      holy_free (sockets);
      holy_free (data.name);
      return holy_errno;
    }
  holy_netbuff_reserve (nb, holy_NET_OUR_MAX_IP_HEADER_SIZE
			+ holy_NET_MAX_LINK_HEADER_SIZE
			+ holy_NET_UDP_HEADER_SIZE);
  holy_netbuff_put (nb, sizeof (struct dns_header)
		    + holy_strlen (name) + 2 + 4);
  head = (struct dns_header *) nb->data;
  optr = (holy_uint8_t *) (head + 1);
  for (iptr = name; *iptr; )
    {
      const char *dot;
      dot = holy_strchr (iptr, '.');
      if (!dot)
	dot = iptr + holy_strlen (iptr);
      if ((dot - iptr) >= 64)
	{
	  holy_free (sockets);
	  holy_free (data.name);
	  return holy_error (holy_ERR_BAD_ARGUMENT,
			     N_("domain name component is too long"));
	}
      *optr = (dot - iptr);
      optr++;
      holy_memcpy (optr, iptr, dot - iptr);
      optr += dot - iptr;
      iptr = dot;
      if (*iptr)
	iptr++;
    }
  *optr++ = 0;

  /* Type.  */
  *optr++ = 0;
  qtypeptr = optr++;

  /* Class.  */
  *optr++ = 0;
  *optr++ = 1;

  head->id = data.id;
  head->flags = FLAGS_RD;
  head->ra_z_r_code = 0;
  head->qdcount = holy_cpu_to_be16_compile_time (1);
  head->ancount = holy_cpu_to_be16_compile_time (0);
  head->nscount = holy_cpu_to_be16_compile_time (0);
  head->arcount = holy_cpu_to_be16_compile_time (0);

  nbd = nb->data;

  for (i = 0; i < n_servers * 4; i++)
    {
      /* Connect to a next server.  */
      while (!(i & 1) && try_server < n_servers)
	{
	  sockets[send_servers] = holy_net_udp_open (servers[try_server++],
						     DNS_PORT,
						     recv_hook,
						     &data);
	  if (!sockets[send_servers])
	    {
	      err = holy_errno;
	      holy_errno = holy_ERR_NONE;
	    }
	  else
	    {
	      send_servers++;
	      break;
	    }
	}
      if (!send_servers)
	goto out;
      if (*data.naddresses)
	goto out;
      for (j = 0; j < send_servers; j++)
	{
          holy_err_t err2;

          holy_size_t t = 0;
          do
            {
              nb->data = nbd;
              if (servers[j].option == DNS_OPTION_IPV4 ||
                 ((servers[j].option == DNS_OPTION_PREFER_IPV4) && (t++ == 0)) ||
                 ((servers[j].option == DNS_OPTION_PREFER_IPV6) && (t++ == 1)))
                *qtypeptr = holy_DNS_QTYPE_A;
              else
                *qtypeptr = holy_DNS_QTYPE_AAAA;

              holy_dprintf ("dns", "QTYPE: %u QNAME: %s\n", *qtypeptr, name);

              err2 = holy_net_send_udp_packet (sockets[j], nb);
              if (err2)
                {
                  holy_errno = holy_ERR_NONE;
                  err = err2;
                }
              if (*data.naddresses)
                goto out;
            }
          while (t == 1);
	}
      holy_net_poll_cards (200, &data.stop);
    }
 out:
  holy_free (data.name);
  holy_netbuff_free (nb);
  for (j = 0; j < send_servers; j++)
    holy_net_udp_close (sockets[j]);
  
  holy_free (sockets);

  if (*data.naddresses)
    return holy_ERR_NONE;
  if (data.dns_err)
    return holy_error (holy_ERR_NET_NO_DOMAIN,
		       N_("no DNS record found"));
    
  if (err)
    {
      holy_errno = err;
      return err;
    }
  return holy_error (holy_ERR_TIMEOUT,
		     N_("no DNS reply received"));
}

static holy_err_t
holy_cmd_nslookup (struct holy_command *cmd __attribute__ ((unused)),
		   int argc, char **args)
{
  holy_err_t err;
  struct holy_net_network_level_address cmd_server;
  struct holy_net_network_level_address *servers;
  holy_size_t nservers, i, naddresses = 0;
  struct holy_net_network_level_address *addresses = 0;
  if (argc != 2 && argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("two arguments expected"));
  if (argc == 2)
    {
      err = holy_net_resolve_address (args[1], &cmd_server);
      if (err)
	return err;
      servers = &cmd_server;
      nservers = 1;
    }
  else
    {
      servers = dns_servers;
      nservers = dns_nservers;
    }

  holy_net_dns_lookup (args[0], servers, nservers, &naddresses,
                       &addresses, 0);

  for (i = 0; i < naddresses; i++)
    {
      char buf[holy_NET_MAX_STR_ADDR_LEN];
      holy_net_addr_to_str (&addresses[i], buf);
      holy_printf ("%s\n", buf);
    }
  holy_free (addresses);
  if (naddresses)
    return holy_ERR_NONE;
  return holy_error (holy_ERR_NET_NO_DOMAIN, N_("no DNS record found"));
}

static holy_err_t
holy_cmd_list_dns (struct holy_command *cmd __attribute__ ((unused)),
		   int argc __attribute__ ((unused)),
		   char **args __attribute__ ((unused)))
{
  holy_size_t i;
  const char *strtype = "";

  for (i = 0; i < dns_nservers; i++)
    {
      switch (dns_servers[i].option)
        {
        case DNS_OPTION_IPV4:
          strtype = _("only ipv4");
          break;

        case DNS_OPTION_IPV6:
          strtype = _("only ipv6");
          break;

        case DNS_OPTION_PREFER_IPV4:
          strtype = _("prefer ipv4");
          break;

        case DNS_OPTION_PREFER_IPV6:
          strtype = _("prefer ipv6");
          break;
        }

      char buf[holy_NET_MAX_STR_ADDR_LEN];
      holy_net_addr_to_str (&dns_servers[i], buf);
      holy_printf ("%s (%s)\n", buf, strtype);
    }
  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_add_dns (struct holy_command *cmd __attribute__ ((unused)),
		  int argc, char **args)
{
  holy_err_t err;
  struct holy_net_network_level_address server;

  if ((argc < 1) || (argc > 2))
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));
  else if (argc == 1)
    server.option = DNS_OPTION_PREFER_IPV4;
  else
    {
      if (holy_strcmp (args[1], "--only-ipv4") == 0)
          server.option = DNS_OPTION_IPV4;
      else if (holy_strcmp (args[1], "--only-ipv6") == 0)
          server.option = DNS_OPTION_IPV6;
      else if (holy_strcmp (args[1], "--prefer-ipv4") == 0)
          server.option = DNS_OPTION_PREFER_IPV4;
      else if (holy_strcmp (args[1], "--prefer-ipv6") == 0)
          server.option = DNS_OPTION_PREFER_IPV6;
      else
        return holy_error (holy_ERR_BAD_ARGUMENT, N_("invalid argument"));
    }

  err = holy_net_resolve_address (args[0], &server);
  if (err)
    return err;

  return holy_net_add_dns_server (&server);
}

static holy_err_t
holy_cmd_del_dns (struct holy_command *cmd __attribute__ ((unused)),
		  int argc, char **args)
{
  holy_err_t err;
  struct holy_net_network_level_address server;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));
  err = holy_net_resolve_address (args[1], &server);
  if (err)
    return err;

  return holy_net_add_dns_server (&server);
}

static holy_command_t cmd, cmd_add, cmd_del, cmd_list;

void
holy_dns_init (void)
{
  cmd = holy_register_command ("net_nslookup", holy_cmd_nslookup,
			       N_("ADDRESS DNSSERVER"),
			       N_("Perform a DNS lookup"));
  cmd_add = holy_register_command ("net_add_dns", holy_cmd_add_dns,
				   N_("DNSSERVER"),
				   N_("Add a DNS server"));
  cmd_del = holy_register_command ("net_del_dns", holy_cmd_del_dns,
				   N_("DNSSERVER"),
				   N_("Remove a DNS server"));
  cmd_list = holy_register_command ("net_ls_dns", holy_cmd_list_dns,
				   NULL, N_("List DNS servers"));
}

void
holy_dns_fini (void)
{
  holy_unregister_command (cmd);
  holy_unregister_command (cmd_add);
  holy_unregister_command (cmd_del);
  holy_unregister_command (cmd_list);
}
