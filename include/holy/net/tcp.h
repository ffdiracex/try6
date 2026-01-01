/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_NET_TCP_HEADER
#define holy_NET_TCP_HEADER	1
#include <holy/types.h>
#include <holy/net.h>

struct holy_net_tcp_socket;
typedef struct holy_net_tcp_socket *holy_net_tcp_socket_t;

struct holy_net_tcp_listen;
typedef struct holy_net_tcp_listen *holy_net_tcp_listen_t;

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
		   void *hook_data);

holy_net_tcp_listen_t
holy_net_tcp_listen (holy_uint16_t port,
		     const struct holy_net_network_level_interface *inf,
		     holy_err_t (*listen_hook) (holy_net_tcp_listen_t listen,
						holy_net_tcp_socket_t sock,
						void *data),
		     void *hook_data);

void
holy_net_tcp_stop_listen (holy_net_tcp_listen_t listen);

holy_err_t
holy_net_send_tcp_packet (const holy_net_tcp_socket_t socket,
			  struct holy_net_buff *nb,
			  int push);

enum
  {
    holy_NET_TCP_CONTINUE_RECEIVING,
    holy_NET_TCP_DISCARD,
    holy_NET_TCP_ABORT
  };

void
holy_net_tcp_close (holy_net_tcp_socket_t sock, int discard_received);

holy_err_t
holy_net_tcp_accept (holy_net_tcp_socket_t sock,
		     holy_err_t (*recv_hook) (holy_net_tcp_socket_t sock,
					      struct holy_net_buff *nb,
					      void *data),
		     void (*error_hook) (holy_net_tcp_socket_t sock,
					 void *data),
		     void (*fin_hook) (holy_net_tcp_socket_t sock,
				       void *data),
		     void *hook_data);

void
holy_net_tcp_stall (holy_net_tcp_socket_t sock);

void
holy_net_tcp_unstall (holy_net_tcp_socket_t sock);

#endif
