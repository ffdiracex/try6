/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/net.h>
#include <holy/net/udp.h>
#include <holy/net/ip.h>
#include <holy/net/netbuff.h>
#include <holy/time.h>

struct holy_net_udp_socket
{
  struct holy_net_udp_socket *next;
  struct holy_net_udp_socket **prev;

  enum { holy_NET_SOCKET_START,
	 holy_NET_SOCKET_ESTABLISHED,
	 holy_NET_SOCKET_CLOSED } status;
  int in_port;
  int out_port;
  holy_err_t (*recv_hook) (holy_net_udp_socket_t sock, struct holy_net_buff *nb,
			   void *recv);
  void *recv_hook_data;
  holy_net_network_level_address_t out_nla;
  holy_net_link_level_address_t ll_target_addr;
  struct holy_net_network_level_interface *inf;
};

static struct holy_net_udp_socket *udp_sockets;

#define FOR_UDP_SOCKETS(var) for (var = udp_sockets; var; var = var->next)

static inline void
udp_socket_register (holy_net_udp_socket_t sock)
{
  holy_list_push (holy_AS_LIST_P (&udp_sockets),
		  holy_AS_LIST (sock));
}

void
holy_net_udp_close (holy_net_udp_socket_t sock)
{
  holy_list_remove (holy_AS_LIST (sock));
  holy_free (sock);
}

holy_net_udp_socket_t
holy_net_udp_open (holy_net_network_level_address_t addr,
		   holy_uint16_t out_port,
		   holy_err_t (*recv_hook) (holy_net_udp_socket_t sock,
					    struct holy_net_buff *nb,
					    void *data),
		   void *recv_hook_data)
{
  holy_err_t err;
  struct holy_net_network_level_interface *inf;
  holy_net_network_level_address_t gateway;
  holy_net_udp_socket_t socket;
  static int in_port = 25300;
  holy_net_link_level_address_t ll_target_addr;

  if (addr.type != holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4
      && addr.type != holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6)
    {
      holy_error (holy_ERR_BUG, "not an IP address");
      return NULL;
    }
 
  err = holy_net_route_address (addr, &gateway, &inf);
  if (err)
    return NULL;

  err = holy_net_link_layer_resolve (inf, &gateway, &ll_target_addr);
  if (err)
    return NULL;

  socket = holy_zalloc (sizeof (*socket));
  if (socket == NULL)
    return NULL; 

  socket->out_port = out_port;
  socket->inf = inf;
  socket->out_nla = addr;
  socket->ll_target_addr = ll_target_addr;
  socket->in_port = in_port++;
  socket->status = holy_NET_SOCKET_START;
  socket->recv_hook = recv_hook;
  socket->recv_hook_data = recv_hook_data;

  udp_socket_register (socket);

  return socket;
}

holy_err_t
holy_net_send_udp_packet (const holy_net_udp_socket_t socket,
			  struct holy_net_buff *nb)
{
  struct udphdr *udph;
  holy_err_t err;

  COMPILE_TIME_ASSERT (holy_NET_UDP_HEADER_SIZE == sizeof (*udph));

  err = holy_netbuff_push (nb, sizeof (*udph));
  if (err)
    return err;

  udph = (struct udphdr *) nb->data;
  udph->src = holy_cpu_to_be16 (socket->in_port);
  udph->dst = holy_cpu_to_be16 (socket->out_port);

  udph->chksum = 0;
  udph->len = holy_cpu_to_be16 (nb->tail - nb->data);

  udph->chksum = holy_net_ip_transport_checksum (nb, holy_NET_IP_UDP,
						 &socket->inf->address,
						 &socket->out_nla);

  return holy_net_send_ip_packet (socket->inf, &(socket->out_nla),
				  &(socket->ll_target_addr), nb,
				  holy_NET_IP_UDP);
}

holy_err_t
holy_net_recv_udp_packet (struct holy_net_buff *nb,
			  struct holy_net_network_level_interface *inf,
			  const holy_net_network_level_address_t *source)
{
  struct udphdr *udph;
  holy_net_udp_socket_t sock;
  holy_err_t err;

  /* Ignore broadcast.  */
  if (!inf)
    {
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }

  udph = (struct udphdr *) nb->data;
  if (nb->tail - nb->data < (holy_ssize_t) sizeof (*udph))
    {
      holy_dprintf ("net", "UDP packet too short: %" PRIuholy_SIZE "\n",
		    (holy_size_t) (nb->tail - nb->data));
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }

  FOR_UDP_SOCKETS (sock)
  {
    if (holy_be_to_cpu16 (udph->dst) == sock->in_port
	&& inf == sock->inf
	&& holy_net_addr_cmp (source, &sock->out_nla) == 0
	&& (sock->status == holy_NET_SOCKET_START
	    || holy_be_to_cpu16 (udph->src) == sock->out_port))
      {
	if (udph->chksum)
	  {
	    holy_uint16_t chk, expected;
	    chk = udph->chksum;
	    udph->chksum = 0;
	    expected = holy_net_ip_transport_checksum (nb, holy_NET_IP_UDP,
						       &sock->out_nla,
						       &sock->inf->address);
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

	if (sock->status == holy_NET_SOCKET_START)
	  {
	    sock->out_port = holy_be_to_cpu16 (udph->src);
	    sock->status = holy_NET_SOCKET_ESTABLISHED;
	  }

	err = holy_netbuff_pull (nb, sizeof (*udph));
	if (err)
	  return err;

	/* App protocol remove its own reader.  */
	if (sock->recv_hook)
	  sock->recv_hook (sock, nb, sock->recv_hook_data);
	else
	  holy_netbuff_free (nb);
	return holy_ERR_NONE;
      }
  }
  holy_netbuff_free (nb);
  return holy_ERR_NONE;
}
