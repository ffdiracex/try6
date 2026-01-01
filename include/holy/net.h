/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_NET_HEADER
#define holy_NET_HEADER	1

#include <holy/types.h>
#include <holy/err.h>
#include <holy/list.h>
#include <holy/fs.h>
#include <holy/file.h>
#include <holy/mm.h>
#include <holy/net/netbuff.h>

enum
  {
    holy_NET_MAX_LINK_HEADER_SIZE = 64,
    holy_NET_UDP_HEADER_SIZE = 8,
    holy_NET_TCP_HEADER_SIZE = 20,
    holy_NET_OUR_IPV4_HEADER_SIZE = 20,
    holy_NET_OUR_IPV6_HEADER_SIZE = 40,
    holy_NET_OUR_MAX_IP_HEADER_SIZE = 40,
    holy_NET_TCP_RESERVE_SIZE = holy_NET_TCP_HEADER_SIZE
    + holy_NET_OUR_IPV4_HEADER_SIZE
    + holy_NET_MAX_LINK_HEADER_SIZE
  };

typedef enum holy_link_level_protocol_id
{
  holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET
} holy_link_level_protocol_id_t;

typedef struct holy_net_link_level_address
{
  holy_link_level_protocol_id_t type;
  union
  {
    holy_uint8_t mac[6];
  };
} holy_net_link_level_address_t;

typedef enum holy_net_interface_flags
  {
    holy_NET_INTERFACE_HWADDRESS_IMMUTABLE = 1,
    holy_NET_INTERFACE_ADDRESS_IMMUTABLE = 2,
    holy_NET_INTERFACE_PERMANENT = 4
  } holy_net_interface_flags_t;

typedef enum holy_net_card_flags
  {
    holy_NET_CARD_HWADDRESS_IMMUTABLE = 1,
    holy_NET_CARD_NO_MANUAL_INTERFACES = 2
  } holy_net_card_flags_t;

struct holy_net_card;

struct holy_net_card_driver
{
  struct holy_net_card_driver *next;
  struct holy_net_card_driver **prev;
  const char *name;
  holy_err_t (*open) (struct holy_net_card *dev);
  void (*close) (struct holy_net_card *dev);
  holy_err_t (*send) (struct holy_net_card *dev,
		      struct holy_net_buff *buf);
  struct holy_net_buff * (*recv) (struct holy_net_card *dev);
};

typedef struct holy_net_packet
{
  struct holy_net_packet *next;
  struct holy_net_packet *prev;
  struct holy_net_packets *up;
  struct holy_net_buff *nb;
} holy_net_packet_t;

typedef struct holy_net_packets
{
  holy_net_packet_t *first;
  holy_net_packet_t *last;
  holy_size_t count;
} holy_net_packets_t;

#ifdef holy_MACHINE_EFI
#include <holy/efi/api.h>
#endif

struct holy_net_slaac_mac_list
{
  struct holy_net_slaac_mac_list *next;
  struct holy_net_slaac_mac_list **prev;
  holy_net_link_level_address_t address;
  int slaac_counter;
  char *name;
};

struct holy_net_link_layer_entry;

struct holy_net_card
{
  struct holy_net_card *next;
  struct holy_net_card **prev;
  const char *name;
  struct holy_net_card_driver *driver;
  holy_net_link_level_address_t default_address;
  holy_net_card_flags_t flags;
  int num_ifaces;
  int opened;
  unsigned idle_poll_delay_ms;
  holy_uint64_t last_poll;
  holy_size_t mtu;
  struct holy_net_slaac_mac_list *slaac_list;
  holy_ssize_t new_ll_entry;
  struct holy_net_link_layer_entry *link_layer_table;
  void *txbuf;
  void *rcvbuf;
  holy_size_t rcvbufsize;
  holy_size_t txbufsize;
  int txbusy;
  union
  {
#ifdef holy_MACHINE_EFI
    struct
    {
      struct holy_efi_simple_network *efi_net;
      holy_efi_handle_t efi_handle;
      holy_size_t last_pkt_size;
    };
#endif
    void *data;
    int data_num;
  };
};

struct holy_net_network_level_interface;

typedef enum holy_network_level_protocol_id
{
  holy_NET_NETWORK_LEVEL_PROTOCOL_DHCP_RECV,
  holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4,
  holy_NET_NETWORK_LEVEL_PROTOCOL_IPV6
} holy_network_level_protocol_id_t;

typedef enum
{
  DNS_OPTION_IPV4,
  DNS_OPTION_IPV6,
  DNS_OPTION_PREFER_IPV4,
  DNS_OPTION_PREFER_IPV6
} holy_dns_option_t;

typedef struct holy_net_network_level_address
{
  holy_network_level_protocol_id_t type;
  union
  {
    holy_uint32_t ipv4;
    holy_uint64_t ipv6[2];
  };
  holy_dns_option_t option;
} holy_net_network_level_address_t;

typedef struct holy_net_network_level_netaddress
{
  holy_network_level_protocol_id_t type;
  union
  {
    struct {
      holy_uint32_t base;
      int masksize; 
    } ipv4;
    struct {
      holy_uint64_t base[2];
      int masksize; 
    } ipv6;
  };
} holy_net_network_level_netaddress_t;

struct holy_net_route
{
  struct holy_net_route *next;
  struct holy_net_route **prev;
  holy_net_network_level_netaddress_t target;
  char *name;
  struct holy_net_network_level_protocol *prot;
  int is_gateway;
  struct holy_net_network_level_interface *interface;
  holy_net_network_level_address_t gw;
};

#define FOR_PACKETS(cont,var) for (var = (cont).first; var; var = var->next)

static inline holy_err_t
holy_net_put_packet (holy_net_packets_t *pkts, struct holy_net_buff *nb)
{
  struct holy_net_packet *n;

  n = holy_malloc (sizeof (*n));
  if (!n)
    return holy_errno;

  n->nb = nb;
  n->next = NULL;
  n->prev = NULL;
  n->up = pkts;
  if (pkts->first)
    {
      pkts->last->next = n;
      pkts->last = n;
      n->prev = pkts->last;
    }
  else
    pkts->first = pkts->last = n;

  pkts->count++;

  return holy_ERR_NONE;
}

static inline void
holy_net_remove_packet (holy_net_packet_t *pkt)
{
  pkt->up->count--;

  if (pkt->prev)
    pkt->prev->next = pkt->next;
  else
    pkt->up->first = pkt->next;
  if (pkt->next)
    pkt->next->prev = pkt->prev;
  else
    pkt->up->last = pkt->prev;
  holy_free (pkt);
}

typedef struct holy_net_app_protocol *holy_net_app_level_t;

typedef struct holy_net_socket *holy_net_socket_t;

struct holy_net_app_protocol
{
  struct holy_net_app_protocol *next;
  struct holy_net_app_protocol **prev;
  const char *name;
  holy_err_t (*dir) (holy_device_t device, const char *path,
		     int (*hook) (const char *filename,
				  const struct holy_dirhook_info *info));
  holy_err_t (*open) (struct holy_file *file, const char *filename);
  holy_err_t (*seek) (struct holy_file *file, holy_off_t off);
  holy_err_t (*close) (struct holy_file *file);
  holy_err_t (*packets_pulled) (struct holy_file *file);
};

typedef struct holy_net
{
  char *server;
  char *name;
  holy_net_app_level_t protocol;
  holy_net_packets_t packs;
  holy_off_t offset;
  holy_fs_t fs;
  int eof;
  int stall;
  int port;
} *holy_net_t;

extern holy_net_t (*EXPORT_VAR (holy_net_open)) (const char *name);

struct holy_net_network_level_interface
{
  struct holy_net_network_level_interface *next;
  struct holy_net_network_level_interface **prev;
  char *name;
  struct holy_net_card *card;
  holy_net_network_level_address_t address;
  holy_net_link_level_address_t hwaddress;
  holy_net_interface_flags_t flags;
  struct holy_net_bootp_packet *dhcp_ack;
  holy_size_t dhcp_acklen;
  void *data;
};

struct holy_net_session;

struct holy_net_session_level_protocol
{
  void (*close) (struct holy_net_session *session);
  holy_ssize_t (*recv) (struct holy_net_session *session, void *buf,
		       holy_size_t size);
  holy_err_t (*send) (struct holy_net_session *session, void *buf,
		      holy_size_t size);
};

struct holy_net_session
{
  struct holy_net_session_level_protocol *protocol;
  void *data;
};

static inline void
holy_net_session_close (struct holy_net_session *session)
{
  session->protocol->close (session);
}

static inline holy_err_t
holy_net_session_send (struct holy_net_session *session, void *buf,
		       holy_size_t size)
{
  return session->protocol->send (session, buf, size);
}

static inline holy_ssize_t
holy_net_session_recv (struct holy_net_session *session, void *buf,
		       holy_size_t size)
{
  return session->protocol->recv (session, buf, size);
}

struct holy_net_network_level_interface *
holy_net_add_addr (const char *name,
		   struct holy_net_card *card,
		   const holy_net_network_level_address_t *addr,
		   const holy_net_link_level_address_t *hwaddress,
		   holy_net_interface_flags_t flags);

extern struct holy_net_network_level_interface *holy_net_network_level_interfaces;
#define FOR_NET_NETWORK_LEVEL_INTERFACES(var) for (var = holy_net_network_level_interfaces; var; var = var->next)
#define FOR_NET_NETWORK_LEVEL_INTERFACES_SAFE(var,next) for (var = holy_net_network_level_interfaces, next = (var ? var->next : 0); var; var = next, next = (var ? var->next : 0))


extern holy_net_app_level_t holy_net_app_level_list;

#ifndef holy_LST_GENERATOR
static inline void
holy_net_app_level_register (holy_net_app_level_t proto)
{
  holy_list_push (holy_AS_LIST_P (&holy_net_app_level_list),
		  holy_AS_LIST (proto));
}
#endif

static inline void
holy_net_app_level_unregister (holy_net_app_level_t proto)
{
  holy_list_remove (holy_AS_LIST (proto));
}

#define FOR_NET_APP_LEVEL(var) FOR_LIST_ELEMENTS((var), \
						 (holy_net_app_level_list))

extern struct holy_net_card *holy_net_cards;

static inline void
holy_net_card_register (struct holy_net_card *card)
{
  holy_list_push (holy_AS_LIST_P (&holy_net_cards),
		  holy_AS_LIST (card));
}

void
holy_net_card_unregister (struct holy_net_card *card);

#define FOR_NET_CARDS(var) for (var = holy_net_cards; var; var = var->next)
#define FOR_NET_CARDS_SAFE(var, next) for (var = holy_net_cards, next = (var ? var->next : 0); var; var = next, next = (var ? var->next : 0))


extern struct holy_net_route *holy_net_routes;

static inline void
holy_net_route_register (struct holy_net_route *route)
{
  holy_list_push (holy_AS_LIST_P (&holy_net_routes),
		  holy_AS_LIST (route));
}

#define FOR_NET_ROUTES(var) for (var = holy_net_routes; var; var = var->next)
struct holy_net_session *
holy_net_open_tcp (char *address, holy_uint16_t port);

holy_err_t
holy_net_resolve_address (const char *name,
			  holy_net_network_level_address_t *addr);

holy_err_t
holy_net_resolve_net_address (const char *name,
			      holy_net_network_level_netaddress_t *addr);

holy_err_t
holy_net_route_address (holy_net_network_level_address_t addr,
			holy_net_network_level_address_t *gateway,
			struct holy_net_network_level_interface **interf);


holy_err_t
holy_net_add_route (const char *name,
		    holy_net_network_level_netaddress_t target,
		    struct holy_net_network_level_interface *inter);

holy_err_t
holy_net_add_route_gw (const char *name,
		       holy_net_network_level_netaddress_t target,
		       holy_net_network_level_address_t gw,
		       struct holy_net_network_level_interface *inter);


#define holy_NET_BOOTP_MAC_ADDR_LEN	16

typedef holy_uint8_t holy_net_bootp_mac_addr_t[holy_NET_BOOTP_MAC_ADDR_LEN];

struct holy_net_bootp_packet
{
  holy_uint8_t opcode;
  holy_uint8_t hw_type;		/* hardware type.  */
  holy_uint8_t hw_len;		/* hardware addr len.  */
  holy_uint8_t gate_hops;	/* zero it.  */
  holy_uint32_t ident;		/* random number chosen by client.  */
  holy_uint16_t seconds;	/* seconds since did initial bootstrap.  */
  holy_uint16_t flags;
  holy_uint32_t	client_ip;
  holy_uint32_t your_ip;
  holy_uint32_t	server_ip;
  holy_uint32_t	gateway_ip;
  holy_net_bootp_mac_addr_t mac_addr;
  char server_name[64];
  char boot_file[128];
  holy_uint8_t vendor[0];
} holy_PACKED;

#define	holy_NET_BOOTP_RFC1048_MAGIC_0	0x63
#define	holy_NET_BOOTP_RFC1048_MAGIC_1	0x82
#define	holy_NET_BOOTP_RFC1048_MAGIC_2	0x53
#define	holy_NET_BOOTP_RFC1048_MAGIC_3	0x63

enum
  {
    holy_NET_BOOTP_PAD = 0x00,
    holy_NET_BOOTP_NETMASK = 0x01,
    holy_NET_BOOTP_ROUTER = 0x03,
    holy_NET_BOOTP_DNS = 0x06,
    holy_NET_BOOTP_HOSTNAME = 0x0c,
    holy_NET_BOOTP_DOMAIN = 0x0f,
    holy_NET_BOOTP_ROOT_PATH = 0x11,
    holy_NET_BOOTP_EXTENSIONS_PATH = 0x12,
    holy_NET_BOOTP_END = 0xff
  };

struct holy_net_network_level_interface *
holy_net_configure_by_dhcp_ack (const char *name,
				struct holy_net_card *card,
				holy_net_interface_flags_t flags,
				const struct holy_net_bootp_packet *bp,
				holy_size_t size,
				int is_def, char **device, char **path);

holy_err_t
holy_net_add_ipv4_local (struct holy_net_network_level_interface *inf,
			 int mask);

void
holy_net_process_dhcp (struct holy_net_buff *nb,
		       struct holy_net_card *card);

int
holy_net_hwaddr_cmp (const holy_net_link_level_address_t *a,
		     const holy_net_link_level_address_t *b);
int
holy_net_addr_cmp (const holy_net_network_level_address_t *a,
		   const holy_net_network_level_address_t *b);


/*
  Currently supported adresses:
  IPv4:   XXX.XXX.XXX.XXX
  IPv6:   XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX
 */
#define holy_NET_MAX_STR_ADDR_LEN sizeof ("XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX:XXXX")

/*
  Currently suppoerted adresses:
  ethernet:   XX:XX:XX:XX:XX:XX
 */

#define holy_NET_MAX_STR_HWADDR_LEN (sizeof ("XX:XX:XX:XX:XX:XX"))

void
holy_net_addr_to_str (const holy_net_network_level_address_t *target,
		      char *buf);
void
holy_net_hwaddr_to_str (const holy_net_link_level_address_t *addr, char *str);

holy_err_t
holy_env_set_net_property (const char *intername, const char *suffix,
                           const char *value, holy_size_t len);

void
holy_net_poll_cards (unsigned time, int *stop_condition);

void holy_bootp_init (void);
void holy_bootp_fini (void);

void holy_dns_init (void);
void holy_dns_fini (void);

static inline void
holy_net_network_level_interface_unregister (struct holy_net_network_level_interface *inter)
{
  inter->card->num_ifaces--;
  *inter->prev = inter->next;
  if (inter->next)
    inter->next->prev = inter->prev;
  inter->next = 0;
  inter->prev = 0;
}

void
holy_net_tcp_retransmit (void);

void
holy_net_link_layer_add_address (struct holy_net_card *card,
				 const holy_net_network_level_address_t *nl,
				 const holy_net_link_level_address_t *ll,
				 int override);
int
holy_net_link_layer_resolve_check (struct holy_net_network_level_interface *inf,
				   const holy_net_network_level_address_t *proto_addr);
holy_err_t
holy_net_link_layer_resolve (struct holy_net_network_level_interface *inf,
			     const holy_net_network_level_address_t *proto_addr,
			     holy_net_link_level_address_t *hw_addr);
holy_err_t
holy_net_dns_lookup (const char *name,
		     const struct holy_net_network_level_address *servers,
		     holy_size_t n_servers,
		     holy_size_t *naddresses,
		     struct holy_net_network_level_address **addresses,
		     int cache);
holy_err_t
holy_net_add_dns_server (const struct holy_net_network_level_address *s);
void
holy_net_remove_dns_server (const struct holy_net_network_level_address *s);


extern char *holy_net_default_server;

#define holy_NET_TRIES 40
#define holy_NET_INTERVAL 400
#define holy_NET_INTERVAL_ADDITION 20

#endif /* ! holy_NET_HEADER */
