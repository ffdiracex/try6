/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/net.h>
#include <holy/net/ip.h>
#include <holy/net/tcp.h>
#include <holy/net/netbuff.h>
#include <holy/time.h>
#include <holy/priority_queue.h>

#define TCP_SYN_RETRANSMISSION_TIMEOUT holy_NET_INTERVAL
#define TCP_SYN_RETRANSMISSION_COUNT holy_NET_TRIES
#define TCP_RETRANSMISSION_TIMEOUT holy_NET_INTERVAL
#define TCP_RETRANSMISSION_COUNT holy_NET_TRIES

struct unacked
{
  struct unacked *next;
  struct unacked **prev;
  struct holy_net_buff *nb;
  holy_uint64_t last_try;
  int try_count;
};

enum
  {
    TCP_FIN = 0x1,
    TCP_SYN = 0x2,
    TCP_RST = 0x4,
    TCP_PUSH = 0x8,
    TCP_ACK = 0x10,
    TCP_URG = 0x20,
  };

struct holy_net_tcp_socket
{
  struct holy_net_tcp_socket *next;
  struct holy_net_tcp_socket **prev;

  int established;
  int i_closed;
  int they_closed;
  int in_port;
  int out_port;
  int errors;
  int they_reseted;
  int i_reseted;
  int i_stall;
  holy_uint32_t my_start_seq;
  holy_uint32_t my_cur_seq;
  holy_uint32_t their_start_seq;
  holy_uint32_t their_cur_seq;
  holy_uint16_t my_window;
  struct unacked *unack_first;
  struct unacked *unack_last;
  holy_err_t (*recv_hook) (holy_net_tcp_socket_t sock, struct holy_net_buff *nb,
			   void *recv);
  void (*error_hook) (holy_net_tcp_socket_t sock, void *recv);
  void (*fin_hook) (holy_net_tcp_socket_t sock, void *recv);
  void *hook_data;
  holy_net_network_level_address_t out_nla;
  holy_net_link_level_address_t ll_target_addr;
  struct holy_net_network_level_interface *inf;
  holy_net_packets_t packs;
  holy_priority_queue_t pq;
};

struct holy_net_tcp_listen
{
  struct holy_net_tcp_listen *next;
  struct holy_net_tcp_listen **prev;

  holy_uint16_t port;
  const struct holy_net_network_level_interface *inf;

  holy_err_t (*listen_hook) (holy_net_tcp_listen_t listen,
			     holy_net_tcp_socket_t sock,
			     void *data);
  void *hook_data;
};

struct tcphdr
{
  holy_uint16_t src;
  holy_uint16_t dst;
  holy_uint32_t seqnr;
  holy_uint32_t ack;
  holy_uint16_t flags;
  holy_uint16_t window;
  holy_uint16_t checksum;
  holy_uint16_t urgent;
} holy_PACKED;

struct tcp_pseudohdr
{
  holy_uint32_t src;
  holy_uint32_t dst;
  holy_uint8_t zero;
  holy_uint8_t proto;
  holy_uint16_t tcp_length;
} holy_PACKED;

struct tcp6_pseudohdr
{
  holy_uint64_t src[2];
  holy_uint64_t dst[2];
  holy_uint32_t tcp_length;
  holy_uint8_t zero[3];
  holy_uint8_t proto;
} holy_PACKED;

static struct holy_net_tcp_socket *tcp_sockets;
static struct holy_net_tcp_listen *tcp_listens;

#define FOR_TCP_SOCKETS(var) FOR_LIST_ELEMENTS (var, tcp_sockets)
#define FOR_TCP_LISTENS(var) FOR_LIST_ELEMENTS (var, tcp_listens)

holy_net_tcp_listen_t
holy_net_tcp_listen (holy_uint16_t port,
		     const struct holy_net_network_level_interface *inf,
		     holy_err_t (*listen_hook) (holy_net_tcp_listen_t listen,
						holy_net_tcp_socket_t sock,
						void *data),
		     void *hook_data)
{
  holy_net_tcp_listen_t ret;
  ret = holy_malloc (sizeof (*ret));
  if (!ret)
    return NULL;
  ret->listen_hook = listen_hook;
  ret->hook_data = hook_data;
  ret->port = port;
  ret->inf = inf;
  holy_list_push (holy_AS_LIST_P (&tcp_listens), holy_AS_LIST (ret));
  return ret;
}

void
holy_net_tcp_stop_listen (holy_net_tcp_listen_t listen)
{
  holy_list_remove (holy_AS_LIST (listen));
}

static inline void
tcp_socket_register (holy_net_tcp_socket_t sock)
{
  holy_list_push (holy_AS_LIST_P (&tcp_sockets),
		  holy_AS_LIST (sock));
}

static void
error (holy_net_tcp_socket_t sock)
{
  struct unacked *unack, *next;

  if (sock->error_hook)
    sock->error_hook (sock, sock->hook_data);

  for (unack = sock->unack_first; unack; unack = next)
    {
      next = unack->next;
      holy_netbuff_free (unack->nb);
      holy_free (unack);
    }

  sock->unack_first = NULL;
  sock->unack_last = NULL;
}

static holy_err_t
tcp_send (struct holy_net_buff *nb, holy_net_tcp_socket_t socket)
{
  holy_err_t err;
  holy_uint8_t *nbd;
  struct unacked *unack;
  struct tcphdr *tcph;
  holy_size_t size;

  tcph = (struct tcphdr *) nb->data;

  tcph->seqnr = holy_cpu_to_be32 (socket->my_cur_seq);
  size = (nb->tail - nb->data - (holy_be_to_cpu16 (tcph->flags) >> 12) * 4);
  if (holy_be_to_cpu16 (tcph->flags) & TCP_FIN)
    size++;
  socket->my_cur_seq += size;
  tcph->src = holy_cpu_to_be16 (socket->in_port);
  tcph->dst = holy_cpu_to_be16 (socket->out_port);
  tcph->checksum = 0;
  tcph->checksum = holy_net_ip_transport_checksum (nb, holy_NET_IP_TCP,
						   &socket->inf->address,
						   &socket->out_nla);
  nbd = nb->data;
  if (size)
    {
      unack = holy_malloc (sizeof (*unack));
      if (!unack)
	return holy_errno;

      unack->next = NULL;
      unack->nb = nb;
      unack->try_count = 1;
      unack->last_try = holy_get_time_ms ();
      if (!socket->unack_last)
	socket->unack_first = socket->unack_last = unack;
      else
	socket->unack_last->next = unack;
    }

  err = holy_net_send_ip_packet (socket->inf, &(socket->out_nla),
				 &(socket->ll_target_addr), nb,
				 holy_NET_IP_TCP);
  if (err)
    return err;
  nb->data = nbd;
  if (!size)
    holy_netbuff_free (nb);
  return holy_ERR_NONE;
}

void
holy_net_tcp_close (holy_net_tcp_socket_t sock,
		    int discard_received)
{
  struct holy_net_buff *nb_fin;
  struct tcphdr *tcph_fin;
  holy_err_t err;

  if (discard_received != holy_NET_TCP_CONTINUE_RECEIVING)
    {
      sock->recv_hook = NULL;
      sock->error_hook = NULL;
      sock->fin_hook = NULL;
    }

  if (discard_received == holy_NET_TCP_ABORT)
    sock->i_reseted = 1;

  if (sock->i_closed)
    return;

  sock->i_closed = 1;

  nb_fin = holy_netbuff_alloc (sizeof (*tcph_fin)
			       + holy_NET_OUR_MAX_IP_HEADER_SIZE
			       + holy_NET_MAX_LINK_HEADER_SIZE);
  if (!nb_fin)
    return;
  err = holy_netbuff_reserve (nb_fin, holy_NET_OUR_MAX_IP_HEADER_SIZE
			       + holy_NET_MAX_LINK_HEADER_SIZE);
  if (err)
    {
      holy_netbuff_free (nb_fin);
      holy_dprintf ("net", "error closing socket\n");
      holy_errno = holy_ERR_NONE;
      return;
    }

  err = holy_netbuff_put (nb_fin, sizeof (*tcph_fin));
  if (err)
    {
      holy_netbuff_free (nb_fin);
      holy_dprintf ("net", "error closing socket\n");
      holy_errno = holy_ERR_NONE;
      return;
    }
  tcph_fin = (void *) nb_fin->data;
  tcph_fin->ack = holy_cpu_to_be32 (sock->their_cur_seq);
  tcph_fin->flags = holy_cpu_to_be16_compile_time ((5 << 12) | TCP_FIN
						   | TCP_ACK);
  tcph_fin->window = holy_cpu_to_be16_compile_time (0);
  tcph_fin->urgent = 0;
  err = tcp_send (nb_fin, sock);
  if (err)
    {
      holy_netbuff_free (nb_fin);
      holy_dprintf ("net", "error closing socket\n");
      holy_errno = holy_ERR_NONE;
    }
  return;
}

static void
ack_real (holy_net_tcp_socket_t sock, int res)
{
  struct holy_net_buff *nb_ack;
  struct tcphdr *tcph_ack;
  holy_err_t err;

  nb_ack = holy_netbuff_alloc (sizeof (*tcph_ack) + 128);
  if (!nb_ack)
    return;
  err = holy_netbuff_reserve (nb_ack, 128);
  if (err)
    {
      holy_netbuff_free (nb_ack);
      holy_dprintf ("net", "error closing socket\n");
      holy_errno = holy_ERR_NONE;
      return;
    }

  err = holy_netbuff_put (nb_ack, sizeof (*tcph_ack));
  if (err)
    {
      holy_netbuff_free (nb_ack);
      holy_dprintf ("net", "error closing socket\n");
      holy_errno = holy_ERR_NONE;
      return;
    }
  tcph_ack = (void *) nb_ack->data;
  if (res)
    {
      tcph_ack->ack = holy_cpu_to_be32_compile_time (0);
      tcph_ack->flags = holy_cpu_to_be16_compile_time ((5 << 12) | TCP_RST);
      tcph_ack->window = holy_cpu_to_be16_compile_time (0);
    }
  else
    {
      tcph_ack->ack = holy_cpu_to_be32 (sock->their_cur_seq);
      tcph_ack->flags = holy_cpu_to_be16_compile_time ((5 << 12) | TCP_ACK);
      tcph_ack->window = !sock->i_stall ? holy_cpu_to_be16 (sock->my_window)
	: 0;
    }
  tcph_ack->urgent = 0;
  tcph_ack->src = holy_cpu_to_be16 (sock->in_port);
  tcph_ack->dst = holy_cpu_to_be16 (sock->out_port);
  err = tcp_send (nb_ack, sock);
  if (err)
    {
      holy_dprintf ("net", "error acking socket\n");
      holy_errno = holy_ERR_NONE;
    }
}

static void
ack (holy_net_tcp_socket_t sock)
{
  ack_real (sock, 0);
}

static void
reset (holy_net_tcp_socket_t sock)
{
  ack_real (sock, 1);
}

void
holy_net_tcp_retransmit (void)
{
  holy_net_tcp_socket_t sock;
  holy_uint64_t ctime = holy_get_time_ms ();
  holy_uint64_t limit_time = ctime - TCP_RETRANSMISSION_TIMEOUT;

  FOR_TCP_SOCKETS (sock)
  {
    struct unacked *unack;
    for (unack = sock->unack_first; unack; unack = unack->next)
      {
	struct tcphdr *tcph;
	holy_uint8_t *nbd;
	holy_err_t err;

	if (unack->last_try > limit_time)
	  continue;
	
	if (unack->try_count > TCP_RETRANSMISSION_COUNT)
	  {
	    error (sock);
	    break;
	  }
	unack->try_count++;
	unack->last_try = ctime;
	nbd = unack->nb->data;
	tcph = (struct tcphdr *) nbd;

	if ((tcph->flags & holy_cpu_to_be16_compile_time (TCP_ACK))
	    && tcph->ack != holy_cpu_to_be32 (sock->their_cur_seq))
	  {
	    tcph->checksum = 0;
	    tcph->checksum = holy_net_ip_transport_checksum (unack->nb,
							     holy_NET_IP_TCP,
							     &sock->inf->address,
							     &sock->out_nla);
	  }

	err = holy_net_send_ip_packet (sock->inf, &(sock->out_nla),
				       &(sock->ll_target_addr), unack->nb,
				       holy_NET_IP_TCP);
	unack->nb->data = nbd;
	if (err)
	  {
	    holy_dprintf ("net", "TCP retransmit failed: %s\n", holy_errmsg);
	    holy_errno = holy_ERR_NONE;
	  }
      }
  }
}

holy_uint16_t
holy_net_ip_transport_checksum (struct holy_net_buff *nb,
				holy_uint16_t proto,
				const holy_net_network_level_address_t *src,
				const holy_net_network_level_address_t *dst)
{
  holy_uint16_t a, b = 0;
  holy_uint32_t c;
  a = ~holy_be_to_cpu16 (holy_net_ip_chksum ((void *) nb->data,
					     nb->tail - nb->data));

  switch (dst->type)
    {
    case holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4:
      {
	struct tcp_pseudohdr ph;
	ph.src = src->ipv4;
	ph.dst = dst->ipv4;
	ph.zero = 0;
	ph.tcp_length = holy_cpu_to_be16 (nb->tail - nb->data);
	ph.proto = proto;
	b = ~holy_be_to_cpu16 (holy_net_ip_chksum ((void *) &ph, sizeof (ph)));
	break;
      }
    case holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6:
      {
	struct tcp6_pseudohdr ph;
	holy_memcpy (ph.src, src->ipv6, sizeof (ph.src));
	holy_memcpy (ph.dst, dst->ipv6, sizeof (ph.dst));
	holy_memset (ph.zero, 0, sizeof (ph.zero));
	ph.tcp_length = holy_cpu_to_be32 (nb->tail - nb->data);
	ph.proto = proto;
	b = ~holy_be_to_cpu16 (holy_net_ip_chksum ((void *) &ph, sizeof (ph)));
	break;
      }
    case holy_NET_NETWORK_LEVEL_PROTOCOL_DHCP_RECV:
      b = 0;
      break;
    }
  c = (holy_uint32_t) a + (holy_uint32_t) b;
  if (c >= 0xffff)
    c -= 0xffff;
  return holy_cpu_to_be16 (~c);
}

/* FIXME: overflow. */
static int
cmp (const void *a__, const void *b__)
{
  struct holy_net_buff *a_ = *(struct holy_net_buff **) a__;
  struct holy_net_buff *b_ = *(struct holy_net_buff **) b__;
  struct tcphdr *a = (struct tcphdr *) a_->data;
  struct tcphdr *b = (struct tcphdr *) b_->data;
  /* We want the first elements to be on top.  */
  if (holy_be_to_cpu32 (a->seqnr) < holy_be_to_cpu32 (b->seqnr))
    return +1;
  if (holy_be_to_cpu32 (a->seqnr) > holy_be_to_cpu32 (b->seqnr))
    return -1;
  return 0;
}

static void
destroy_pq (holy_net_tcp_socket_t sock)
{
  struct holy_net_buff **nb_p;
  while ((nb_p = holy_priority_queue_top (sock->pq)))
    {
      holy_netbuff_free (*nb_p);
      holy_priority_queue_pop (sock->pq);
    }

  holy_priority_queue_destroy (sock->pq);
}

holy_err_t
holy_net_tcp_accept (holy_net_tcp_socket_t sock,
		     holy_err_t (*recv_hook) (holy_net_tcp_socket_t sock,
					      struct holy_net_buff *nb,
					      void *data),
		     void (*error_hook) (holy_net_tcp_socket_t sock,
					 void *data),
		     void (*fin_hook) (holy_net_tcp_socket_t sock,
				       void *data),
		     void *hook_data)
{
  struct holy_net_buff *nb_ack;
  struct tcphdr *tcph;
  holy_err_t err;
  holy_net_network_level_address_t gateway;
  struct holy_net_network_level_interface *inf;

  sock->recv_hook = recv_hook;
  sock->error_hook = error_hook;
  sock->fin_hook = fin_hook;
  sock->hook_data = hook_data;

  err = holy_net_route_address (sock->out_nla, &gateway, &inf);
  if (err)
    return err;

  err = holy_net_link_layer_resolve (sock->inf, &gateway, &(sock->ll_target_addr));
  if (err)
    return err;

  nb_ack = holy_netbuff_alloc (sizeof (*tcph)
			       + holy_NET_OUR_MAX_IP_HEADER_SIZE
			       + holy_NET_MAX_LINK_HEADER_SIZE);
  if (!nb_ack)
    return holy_errno;
  err = holy_netbuff_reserve (nb_ack, holy_NET_OUR_MAX_IP_HEADER_SIZE
			      + holy_NET_MAX_LINK_HEADER_SIZE);
  if (err)
    {
      holy_netbuff_free (nb_ack);
      return err;
    }

  err = holy_netbuff_put (nb_ack, sizeof (*tcph));
  if (err)
    {
      holy_netbuff_free (nb_ack);
      return err;
    }
  tcph = (void *) nb_ack->data;
  tcph->ack = holy_cpu_to_be32 (sock->their_cur_seq);
  tcph->flags = holy_cpu_to_be16_compile_time ((5 << 12) | TCP_SYN | TCP_ACK);
  tcph->window = holy_cpu_to_be16 (sock->my_window);
  tcph->urgent = 0;
  sock->established = 1;
  tcp_socket_register (sock);
  err = tcp_send (nb_ack, sock);
  if (err)
    return err;
  sock->my_cur_seq++;
  return holy_ERR_NONE;
}

holy_net_tcp_socket_t
holy_net_tcp_open (char *server,
		   holy_uint16_t out_port,
		   holy_err_t (*recv_hook) (holy_net_tcp_socket_t sock,
					    struct holy_net_buff *nb,
					    void *data),
		   void (*error_hook) (holy_net_tcp_socket_t sock,
				       void *data),
		   void (*fin_hook) (holy_net_tcp_socket_t sock,
				     void *data),
		   void *hook_data)
{
  holy_err_t err;
  holy_net_network_level_address_t addr;
  struct holy_net_network_level_interface *inf;
  holy_net_network_level_address_t gateway;
  holy_net_tcp_socket_t socket;
  static holy_uint16_t in_port = 21550;
  struct holy_net_buff *nb;
  struct tcphdr *tcph;
  int i;
  holy_uint8_t *nbd;
  holy_net_link_level_address_t ll_target_addr;

  err = holy_net_resolve_address (server, &addr);
  if (err)
    return NULL;

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
  socket->recv_hook = recv_hook;
  socket->error_hook = error_hook;
  socket->fin_hook = fin_hook;
  socket->hook_data = hook_data;

  nb = holy_netbuff_alloc (sizeof (*tcph) + 128);
  if (!nb)
    {
      holy_free (socket);
      return NULL;
    }

  err = holy_netbuff_reserve (nb, 128);
  if (err)
    {
      holy_free (socket);
      holy_netbuff_free (nb);
      return NULL;
    }

  err = holy_netbuff_put (nb, sizeof (*tcph));
  if (err)
    {
      holy_free (socket);
      holy_netbuff_free (nb);
      return NULL;
    }
  socket->pq = holy_priority_queue_new (sizeof (struct holy_net_buff *), cmp);
  if (!socket->pq)
    {
      holy_free (socket);
      holy_netbuff_free (nb);
      return NULL;
    }

  tcph = (void *) nb->data;
  socket->my_start_seq = holy_get_time_ms ();
  socket->my_cur_seq = socket->my_start_seq + 1;
  socket->my_window = 8192;
  tcph->seqnr = holy_cpu_to_be32 (socket->my_start_seq);
  tcph->ack = holy_cpu_to_be32_compile_time (0);
  tcph->flags = holy_cpu_to_be16_compile_time ((5 << 12) | TCP_SYN);
  tcph->window = holy_cpu_to_be16 (socket->my_window);
  tcph->urgent = 0;
  tcph->src = holy_cpu_to_be16 (socket->in_port);
  tcph->dst = holy_cpu_to_be16 (socket->out_port);
  tcph->checksum = 0;
  tcph->checksum = holy_net_ip_transport_checksum (nb, holy_NET_IP_TCP,
						   &socket->inf->address,
						   &socket->out_nla);

  tcp_socket_register (socket);

  nbd = nb->data;
  for (i = 0; i < TCP_SYN_RETRANSMISSION_COUNT; i++)
    {
      int j;
      nb->data = nbd;
      err = holy_net_send_ip_packet (socket->inf, &(socket->out_nla),
				     &(socket->ll_target_addr), nb,
				     holy_NET_IP_TCP);
      if (err)
	{
	  holy_list_remove (holy_AS_LIST (socket));
	  holy_free (socket);
	  holy_netbuff_free (nb);
	  return NULL;
	}
      for (j = 0; (j < TCP_SYN_RETRANSMISSION_TIMEOUT / 50 
		   && !socket->established); j++)
	holy_net_poll_cards (50, &socket->established);
      if (socket->established)
	break;
    }
  if (!socket->established)
    {
      holy_list_remove (holy_AS_LIST (socket));
      if (socket->they_reseted)
	holy_error (holy_ERR_NET_PORT_CLOSED,
		    N_("connection refused"));
      else
	holy_error (holy_ERR_NET_NO_ANSWER,
		    N_("connection timeout"));

      holy_netbuff_free (nb);
      destroy_pq (socket);
      holy_free (socket);
      return NULL;
    }

  holy_netbuff_free (nb);
  return socket;
}

holy_err_t
holy_net_send_tcp_packet (const holy_net_tcp_socket_t socket,
			  struct holy_net_buff *nb, int push)
{
  struct tcphdr *tcph;
  holy_err_t err;
  holy_ssize_t fraglen;
  COMPILE_TIME_ASSERT (sizeof (struct tcphdr) == holy_NET_TCP_HEADER_SIZE);
  if (socket->out_nla.type == holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4)
    fraglen = (socket->inf->card->mtu - holy_NET_OUR_IPV4_HEADER_SIZE
	       - sizeof (*tcph));
  else
    fraglen = 1280 - holy_NET_OUR_IPV6_HEADER_SIZE;

  while (nb->tail - nb->data > fraglen)
    {
      struct holy_net_buff *nb2;

      nb2 = holy_netbuff_alloc (fraglen + sizeof (*tcph)
				+ holy_NET_OUR_MAX_IP_HEADER_SIZE
				+ holy_NET_MAX_LINK_HEADER_SIZE);
      if (!nb2)
	return holy_errno;
      err = holy_netbuff_reserve (nb2, holy_NET_MAX_LINK_HEADER_SIZE
				  + holy_NET_OUR_MAX_IP_HEADER_SIZE);
      if (err)
	return err;
      err = holy_netbuff_put (nb2, sizeof (*tcph));
      if (err)
	return err;

      tcph = (struct tcphdr *) nb2->data;
      tcph->ack = holy_cpu_to_be32 (socket->their_cur_seq);
      tcph->flags = holy_cpu_to_be16_compile_time ((5 << 12) | TCP_ACK);
      tcph->window = !socket->i_stall ? holy_cpu_to_be16 (socket->my_window)
	: 0;
      tcph->urgent = 0;
      err = holy_netbuff_put (nb2, fraglen);
      if (err)
	return err;
      holy_memcpy (tcph + 1, nb->data, fraglen);
      err = holy_netbuff_pull (nb, fraglen);
      if (err)
	return err;

      err = tcp_send (nb2, socket);
      if (err)
	return err;
    }

  err = holy_netbuff_push (nb, sizeof (*tcph));
  if (err)
    return err;

  tcph = (struct tcphdr *) nb->data;
  tcph->ack = holy_cpu_to_be32 (socket->their_cur_seq);
  tcph->flags = (holy_cpu_to_be16_compile_time ((5 << 12) | TCP_ACK)
		 | (push ? holy_cpu_to_be16_compile_time (TCP_PUSH) : 0));
  tcph->window = !socket->i_stall ? holy_cpu_to_be16 (socket->my_window) : 0;
  tcph->urgent = 0;
  return tcp_send (nb, socket);
}

holy_err_t
holy_net_recv_tcp_packet (struct holy_net_buff *nb,
			  struct holy_net_network_level_interface *inf,
			  const holy_net_network_level_address_t *source)
{
  struct tcphdr *tcph;
  holy_net_tcp_socket_t sock;
  holy_err_t err;

  /* Ignore broadcast.  */
  if (!inf)
    {
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }

  tcph = (struct tcphdr *) nb->data;
  if ((holy_be_to_cpu16 (tcph->flags) >> 12) < 5)
    {
      holy_dprintf ("net", "TCP header too short: %u\n",
		    holy_be_to_cpu16 (tcph->flags) >> 12);
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }
  if (nb->tail - nb->data < (holy_ssize_t) ((holy_be_to_cpu16 (tcph->flags)
					     >> 12) * sizeof (holy_uint32_t)))
    {
      holy_dprintf ("net", "TCP packet too short: %" PRIuholy_SIZE "\n",
		    (holy_size_t) (nb->tail - nb->data));
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }

  FOR_TCP_SOCKETS (sock)
  {
    if (!(holy_be_to_cpu16 (tcph->dst) == sock->in_port
	  && holy_be_to_cpu16 (tcph->src) == sock->out_port
	  && inf == sock->inf
	  && holy_net_addr_cmp (source, &sock->out_nla) == 0))
      continue;
    if (tcph->checksum)
      {
	holy_uint16_t chk, expected;
	chk = tcph->checksum;
	tcph->checksum = 0;
	expected = holy_net_ip_transport_checksum (nb, holy_NET_IP_TCP,
						   &sock->out_nla,
						   &sock->inf->address);
	if (expected != chk)
	  {
	    holy_dprintf ("net", "Invalid TCP checksum. "
			  "Expected %x, got %x\n",
			  holy_be_to_cpu16 (expected),
			  holy_be_to_cpu16 (chk));
	    holy_netbuff_free (nb);
	    return holy_ERR_NONE;
	  }
	tcph->checksum = chk;
      }

    if ((holy_be_to_cpu16 (tcph->flags) & TCP_SYN)
	&& (holy_be_to_cpu16 (tcph->flags) & TCP_ACK)
	&& !sock->established)
      {
	sock->their_start_seq = holy_be_to_cpu32 (tcph->seqnr);
	sock->their_cur_seq = sock->their_start_seq + 1;
	sock->established = 1;
      }

    if (holy_be_to_cpu16 (tcph->flags) & TCP_RST)
      {
	sock->they_reseted = 1;

	error (sock);

	holy_netbuff_free (nb);

	return holy_ERR_NONE;
      }

    if (holy_be_to_cpu16 (tcph->flags) & TCP_ACK)
      {
	struct unacked *unack, *next;
	holy_uint32_t acked = holy_be_to_cpu32 (tcph->ack);
	for (unack = sock->unack_first; unack; unack = next)
	  {
	    holy_uint32_t seqnr;
	    struct tcphdr *unack_tcph;
	    next = unack->next;
	    seqnr = holy_be_to_cpu32 (((struct tcphdr *) unack->nb->data)
				      ->seqnr);
	    unack_tcph = (struct tcphdr *) unack->nb->data;
	    seqnr += (unack->nb->tail - unack->nb->data
		      - (holy_be_to_cpu16 (unack_tcph->flags) >> 12) * 4);
	    if (holy_be_to_cpu16 (unack_tcph->flags) & TCP_FIN)
	      seqnr++;

	    if (seqnr > acked)
	      break;
	    holy_netbuff_free (unack->nb);
	    holy_free (unack);
	  }
	sock->unack_first = unack;
	if (!sock->unack_first)
	  sock->unack_last = NULL;
      }

    if (holy_be_to_cpu32 (tcph->seqnr) < sock->their_cur_seq)
      {
	ack (sock);
	holy_netbuff_free (nb);
	return holy_ERR_NONE;
      }
    if (sock->i_reseted && (nb->tail - nb->data
			    - (holy_be_to_cpu16 (tcph->flags)
			       >> 12) * sizeof (holy_uint32_t)) > 0)
      {
	reset (sock);
      }

    err = holy_priority_queue_push (sock->pq, &nb);
    if (err)
      {
	holy_netbuff_free (nb);
	return err;
      }

    {
      struct holy_net_buff **nb_top_p, *nb_top;
      int do_ack = 0;
      int just_closed = 0;
      while (1)
	{
	  nb_top_p = holy_priority_queue_top (sock->pq);
	  if (!nb_top_p)
	    return holy_ERR_NONE;
	  nb_top = *nb_top_p;
	  tcph = (struct tcphdr *) nb_top->data;
	  if (holy_be_to_cpu32 (tcph->seqnr) >= sock->their_cur_seq)
	    break;
	  holy_netbuff_free (nb_top);
	  holy_priority_queue_pop (sock->pq);
	}
      if (holy_be_to_cpu32 (tcph->seqnr) != sock->their_cur_seq)
	{
	  ack (sock);
	  return holy_ERR_NONE;
	}
      while (1)
	{
	  nb_top_p = holy_priority_queue_top (sock->pq);
	  if (!nb_top_p)
	    break;
	  nb_top = *nb_top_p;
	  tcph = (struct tcphdr *) nb_top->data;

	  if (holy_be_to_cpu32 (tcph->seqnr) != sock->their_cur_seq)
	    break;
	  holy_priority_queue_pop (sock->pq);

	  err = holy_netbuff_pull (nb_top, (holy_be_to_cpu16 (tcph->flags)
					    >> 12) * sizeof (holy_uint32_t));
	  if (err)
	    {
	      holy_netbuff_free (nb_top);
	      return err;
	    }

	  sock->their_cur_seq += (nb_top->tail - nb_top->data);
	  if (holy_be_to_cpu16 (tcph->flags) & TCP_FIN)
	    {
	      sock->they_closed = 1;
	      just_closed = 1;
	      sock->their_cur_seq++;
	      do_ack = 1;
	    }
	  /* If there is data, puts packet in socket list. */
	  if ((nb_top->tail - nb_top->data) > 0)
	    {
	      holy_net_put_packet (&sock->packs, nb_top);
	      do_ack = 1;
	    }
	  else
	    holy_netbuff_free (nb_top);
	}
      if (do_ack)
	ack (sock);
      while (sock->packs.first)
	{
	  nb = sock->packs.first->nb;
	  if (sock->recv_hook)
	    sock->recv_hook (sock, sock->packs.first->nb, sock->hook_data);
	  else
	    holy_netbuff_free (nb);
	  holy_net_remove_packet (sock->packs.first);
	}

      if (sock->fin_hook && just_closed)
	sock->fin_hook (sock, sock->hook_data);
    }
	
    return holy_ERR_NONE;
  }
  if (holy_be_to_cpu16 (tcph->flags) & TCP_SYN)
    {
      holy_net_tcp_listen_t listen;

      FOR_TCP_LISTENS (listen)
      {
	if (!(holy_be_to_cpu16 (tcph->dst) == listen->port
	      && (inf == listen->inf || listen->inf == NULL)))
	  continue;
	sock = holy_zalloc (sizeof (*sock));
	if (sock == NULL)
	  return holy_errno;
	
	sock->out_port = holy_be_to_cpu16 (tcph->src);
	sock->in_port = holy_be_to_cpu16 (tcph->dst);
	sock->inf = inf;
	sock->out_nla = *source;
	sock->their_start_seq = holy_be_to_cpu32 (tcph->seqnr);
	sock->their_cur_seq = sock->their_start_seq + 1;
	sock->my_cur_seq = sock->my_start_seq = holy_get_time_ms ();
	sock->my_window = 8192;

	sock->pq = holy_priority_queue_new (sizeof (struct holy_net_buff *),
					    cmp);
	if (!sock->pq)
	  {
	    holy_free (sock);
	    holy_netbuff_free (nb);
	    return holy_errno;
	  }

	err = listen->listen_hook (listen, sock, listen->hook_data);

	holy_netbuff_free (nb);
	return err;

      }
    }
  holy_netbuff_free (nb);
  return holy_ERR_NONE;
}

void
holy_net_tcp_stall (holy_net_tcp_socket_t sock)
{
  if (sock->i_stall)
    return;
  sock->i_stall = 1;
  ack (sock);
}

void
holy_net_tcp_unstall (holy_net_tcp_socket_t sock)
{
  if (!sock->i_stall)
    return;
  sock->i_stall = 0;
  ack (sock);
}
