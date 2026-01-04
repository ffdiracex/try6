/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/net/netbuff.h>
#include <holy/ieee1275/ieee1275.h>
#include <holy/dl.h>
#include <holy/net.h>
#include <holy/time.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

struct holy_ofnetcard_data
{
  char *path;
  char *suffix;
  holy_ieee1275_ihandle_t handle;
};

static holy_err_t
card_open (struct holy_net_card *dev)
{
  int status;
  struct holy_ofnetcard_data *data = dev->data;

  status = holy_ieee1275_open (data->path, &(data->handle));

  if (status)
    return holy_error (holy_ERR_IO, "Couldn't open network card.");

  return holy_ERR_NONE;
}

static void
card_close (struct holy_net_card *dev)
{
  struct holy_ofnetcard_data *data = dev->data;

  if (data->handle)
    holy_ieee1275_close (data->handle);
}

static holy_err_t
send_card_buffer (struct holy_net_card *dev, struct holy_net_buff *pack)
{
  holy_ssize_t actual;
  int status;
  struct holy_ofnetcard_data *data = dev->data;
  holy_size_t len;

  len = (pack->tail - pack->data);
  if (len > dev->mtu)
    len = dev->mtu;

  holy_memcpy (dev->txbuf, pack->data, len);
  status = holy_ieee1275_write (data->handle, dev->txbuf,
				len, &actual);

  if (status)
    return holy_error (holy_ERR_IO, N_("couldn't send network packet"));
  return holy_ERR_NONE;
}

static struct holy_net_buff *
get_card_packet (struct holy_net_card *dev)
{
  holy_ssize_t actual;
  int rc;
  struct holy_ofnetcard_data *data = dev->data;
  holy_uint64_t start_time;
  struct holy_net_buff *nb;

  start_time = holy_get_time_ms ();
  do
    rc = holy_ieee1275_read (data->handle, dev->rcvbuf, dev->rcvbufsize, &actual);
  while ((actual <= 0 || rc < 0) && (holy_get_time_ms () - start_time < 200));

  if (actual <= 0)
    return NULL;

  nb = holy_netbuff_alloc (actual + 2);
  if (!nb)
    return NULL;
  /* Reserve 2 bytes so that 2 + 14/18 bytes of ethernet header is divisible
     by 4. So that IP header is aligned on 4 bytes. */
  holy_netbuff_reserve (nb, 2);

  holy_memcpy (nb->data, dev->rcvbuf, actual);

  if (holy_netbuff_put (nb, actual))
    {
      holy_netbuff_free (nb);
      return NULL;
    }

  return nb;
}

static struct holy_net_card_driver ofdriver =
  {
    .name = "ofnet",
    .open = card_open,
    .close = card_close,
    .send = send_card_buffer,
    .recv = get_card_packet
  };

static const struct
{
  const char *name;
  int offset;
}

bootp_response_properties[] =
  {
    { .name = "bootp-response", .offset = 0},
    { .name = "dhcp-response", .offset = 0},
    { .name = "bootpreply-packet", .offset = 0x2a},
  };

enum
{
  BOOTARGS_SERVER_ADDR,
  BOOTARGS_FILENAME,
  BOOTARGS_CLIENT_ADDR,
  BOOTARGS_GATEWAY_ADDR,
  BOOTARGS_BOOTP_RETRIES,
  BOOTARGS_TFTP_RETRIES,
  BOOTARGS_SUBNET_MASK,
  BOOTARGS_BLOCKSIZE
};

static int
holy_ieee1275_parse_bootpath (const char *devpath, char *bootpath,
                              char **device, struct holy_net_card **card)
{
  char *args;
  char *comma_char = 0;
  char *equal_char = 0;
  holy_size_t field_counter = 0;

  holy_net_network_level_address_t client_addr, gateway_addr, subnet_mask;
  holy_net_link_level_address_t hw_addr;
  holy_net_interface_flags_t flags = 0;
  struct holy_net_network_level_interface *inter = NULL;

  hw_addr.type = holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET;

  args = bootpath + holy_strlen (devpath) + 1;
  do
    {
      comma_char = holy_strchr (args, ',');
      if (comma_char != 0)
        *comma_char = 0;

      /* Check if it's an option (like speed=auto) and not a default parameter */
      equal_char = holy_strchr (args, '=');
      if (equal_char != 0)
        {
          *equal_char = 0;
          holy_env_set_net_property ((*card)->name, args, equal_char + 1,
                                     holy_strlen(equal_char + 1));
          *equal_char = '=';
        }
      else
        {
          switch (field_counter++)
            {
            case BOOTARGS_SERVER_ADDR:
              *device = holy_xasprintf ("tftp,%s", args);
              if (!*device)
                return holy_errno;
              break;

            case BOOTARGS_CLIENT_ADDR:
              holy_net_resolve_address (args, &client_addr);
              break;

            case BOOTARGS_GATEWAY_ADDR:
              holy_net_resolve_address (args, &gateway_addr);
              break;

            case BOOTARGS_SUBNET_MASK:
              holy_net_resolve_address (args, &subnet_mask);
              break;
            }
        }
      args = comma_char + 1;
      if (comma_char != 0)
        *comma_char = ',';
    } while (comma_char != 0);

  if ((client_addr.ipv4 != 0) && (subnet_mask.ipv4 != 0))
    {
      holy_ieee1275_phandle_t devhandle;
      holy_ieee1275_finddevice (devpath, &devhandle);
      holy_ieee1275_get_property (devhandle, "mac-address",
                                  hw_addr.mac, sizeof(hw_addr.mac), 0);
      inter = holy_net_add_addr ((*card)->name, *card, &client_addr, &hw_addr,
                                 flags);
      holy_net_add_ipv4_local (inter,
                          __builtin_ctz (~holy_le_to_cpu32 (subnet_mask.ipv4)));
    }

  if (gateway_addr.ipv4 != 0)
    {
      holy_net_network_level_netaddress_t target;
      char *rname;

      target.type = holy_NET_NETWORK_LEVEL_PROTOCOL_IPV4;
      target.ipv4.base = 0;
      target.ipv4.masksize = 0;
      rname = holy_xasprintf ("%s:default", ((*card)->name));
      if (rname)
        holy_net_add_route_gw (rname, target, gateway_addr, inter);
      else
        return holy_errno;
    }

  return 0;
}

static void
holy_ieee1275_net_config_real (const char *devpath, char **device, char **path,
                               char *bootpath)
{
  struct holy_net_card *card;

  /* FIXME: Check that it's the right card.  */
  FOR_NET_CARDS (card)
  {
    char *bootp_response;
    char *canon;
    char c;
    struct holy_ofnetcard_data *data;

    holy_ssize_t size = -1;
    unsigned int i;

    if (card->driver != &ofdriver)
      continue;

    data = card->data;
    c = *data->suffix;
    *data->suffix = '\0';
    canon = holy_ieee1275_canonicalise_devname (data->path);
    *data->suffix = c;
    if (holy_strcmp (devpath, canon) != 0)
      {
	holy_free (canon);
	continue;
      }
    holy_free (canon);

    holy_ieee1275_parse_bootpath (devpath, bootpath, device, &card);

    for (i = 0; i < ARRAY_SIZE (bootp_response_properties); i++)
      if (holy_ieee1275_get_property_length (holy_ieee1275_chosen,
					     bootp_response_properties[i].name,
					     &size) >= 0)
	break;

    if (size < 0)
      return;

    bootp_response = holy_malloc (size);
    if (!bootp_response)
      {
	holy_print_error ();
	return;
      }
    if (holy_ieee1275_get_property (holy_ieee1275_chosen,
				    bootp_response_properties[i].name,
				    bootp_response, size, 0) < 0)
      return;

    holy_net_configure_by_dhcp_ack (card->name, card, 0,
				    (struct holy_net_bootp_packet *)
				    (bootp_response
				     + bootp_response_properties[i].offset),
				    size - bootp_response_properties[i].offset,
				    1, device, path);
    holy_free (bootp_response);
    return;
  }
}

/* Allocate memory with alloc-mem */
static void *
holy_ieee1275_alloc_mem (holy_size_t len)
{
  struct alloc_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t method;
    holy_ieee1275_cell_t len;
    holy_ieee1275_cell_t catch;
    holy_ieee1275_cell_t result;
  }
  args;

  if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_CANNOT_INTERPRET))
    {
      holy_error (holy_ERR_UNKNOWN_COMMAND, N_("interpret is not supported"));
      return NULL;
    }

  INIT_IEEE1275_COMMON (&args.common, "interpret", 2, 2);
  args.len = len;
  args.method = (holy_ieee1275_cell_t) "alloc-mem";

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1 || args.catch)
    {
      holy_error (holy_ERR_INVALID_COMMAND, N_("alloc-mem failed"));
      return NULL;
    }
  else
    return (void *)args.result;
}

/* Free memory allocated by alloc-mem */
static holy_err_t
holy_ieee1275_free_mem (void *addr, holy_size_t len)
{
  struct free_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t method;
    holy_ieee1275_cell_t len;
    holy_ieee1275_cell_t addr;
    holy_ieee1275_cell_t catch;
  }
  args;

  if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_CANNOT_INTERPRET))
    {
      holy_error (holy_ERR_UNKNOWN_COMMAND, N_("interpret is not supported"));
      return holy_errno;
    }

  INIT_IEEE1275_COMMON (&args.common, "interpret", 3, 1);
  args.addr = (holy_ieee1275_cell_t)addr;
  args.len = len;
  args.method = (holy_ieee1275_cell_t) "free-mem";

  if (IEEE1275_CALL_ENTRY_FN(&args) == -1 || args.catch)
    {
      holy_error (holy_ERR_INVALID_COMMAND, N_("free-mem failed"));
      return holy_errno;
    }

  return holy_ERR_NONE;
}

static void *
ofnet_alloc_netbuf (holy_size_t len)
{
  if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_VIRT_TO_REAL_BROKEN))
    return holy_ieee1275_alloc_mem (len);
  else
    return holy_zalloc (len);
}

static void
ofnet_free_netbuf (void *addr, holy_size_t len)
{
  if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_VIRT_TO_REAL_BROKEN))
    holy_ieee1275_free_mem (addr, len);
  else
    holy_free (addr);
}

static int
search_net_devices (struct holy_ieee1275_devalias *alias)
{
  struct holy_ofnetcard_data *ofdata;
  struct holy_net_card *card;
  holy_ieee1275_phandle_t devhandle;
  holy_net_link_level_address_t lla;
  holy_ssize_t prop_size;
  holy_uint64_t prop;
  holy_uint8_t *pprop;
  char *shortname;
  char need_suffix = 1;

  if (holy_strcmp (alias->type, "network") != 0)
    return 0;

  ofdata = holy_malloc (sizeof (struct holy_ofnetcard_data));
  if (!ofdata)
    {
      holy_print_error ();
      return 1;
    }
  card = holy_zalloc (sizeof (struct holy_net_card));
  if (!card)
    {
      holy_free (ofdata);
      holy_print_error ();
      return 1;
    }

#define SUFFIX ":speed=auto,duplex=auto,1.1.1.1,dummy,1.1.1.1,1.1.1.1,5,5,1.1.1.1,512"

  if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_NO_OFNET_SUFFIX))
    need_suffix = 0;

  /* sun4v vnet devices do not support setting duplex/speed */
  {
    char *ptr;

    holy_ieee1275_finddevice (alias->path, &devhandle);

    holy_ieee1275_get_property_length (devhandle, "compatible", &prop_size);
    if (prop_size > 0)
      {
	pprop = holy_malloc (prop_size);
	if (!pprop)
	  {
	    holy_free (card);
	    holy_free (ofdata);
	    holy_print_error ();
	    return 1;
	  }

	if (!holy_ieee1275_get_property (devhandle, "compatible",
					 pprop, prop_size, NULL))
	  {
	    for (ptr = (char *) pprop; ptr - (char *) pprop < prop_size;
		 ptr += holy_strlen (ptr) + 1)
	      {
		if (!holy_strcmp(ptr, "SUNW,sun4v-network"))
		  need_suffix = 0;
	      }
	}

	holy_free (pprop);
      }
  }

  if (need_suffix)
    ofdata->path = holy_malloc (holy_strlen (alias->path) + sizeof (SUFFIX));
  else
    ofdata->path = holy_malloc (holy_strlen (alias->path) + 1);
  if (!ofdata->path)
    {
      holy_print_error ();
      return 0;
    }
  ofdata->suffix = holy_stpcpy (ofdata->path, alias->path);
  if (need_suffix)
    holy_memcpy (ofdata->suffix, SUFFIX, sizeof (SUFFIX));
  else
    *ofdata->suffix = '\0';

  holy_ieee1275_finddevice (ofdata->path, &devhandle);

  {
    holy_uint32_t t;
    if (holy_ieee1275_get_integer_property (devhandle,
					    "max-frame-size", &t,
					    sizeof (t), 0))
      card->mtu = 1500;
    else
      card->mtu = t;
  }

  pprop = (holy_uint8_t *) &prop;
  if (holy_ieee1275_get_property (devhandle, "mac-address",
				  pprop, sizeof(prop), &prop_size)
      && holy_ieee1275_get_property (devhandle, "local-mac-address",
				     pprop, sizeof(prop), &prop_size))
    {
      holy_error (holy_ERR_IO, "Couldn't retrieve mac address.");
      holy_print_error ();
      return 0;
    }

  if (prop_size == 8)
    holy_memcpy (&lla.mac, pprop+2, 6);
  else
    holy_memcpy (&lla.mac, pprop, 6);

  lla.type = holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
  card->default_address = lla;

  card->txbufsize = ALIGN_UP (card->mtu, 64) + 256;
  card->rcvbufsize = ALIGN_UP (card->mtu, 64) + 256;

  card->txbuf = ofnet_alloc_netbuf (card->txbufsize);
  if (!card->txbuf)
    goto fail_netbuf;

  card->rcvbuf = ofnet_alloc_netbuf (card->rcvbufsize);
  if (!card->rcvbuf)
    {
      holy_error_push ();
      ofnet_free_netbuf (card->txbuf, card->txbufsize);
      holy_error_pop ();
      goto fail_netbuf;
    }
  card->driver = NULL;
  card->data = ofdata;
  card->flags = 0;
  shortname = holy_ieee1275_get_devname (alias->path);
  card->name = holy_xasprintf ("ofnet_%s", shortname ? : alias->path);
  card->idle_poll_delay_ms = 10;
  holy_free (shortname);

  card->driver = &ofdriver;
  holy_net_card_register (card);
  return 0;

fail_netbuf:
  holy_free (ofdata->path);
  holy_free (ofdata);
  holy_free (card);
  holy_print_error ();
  return 0;
}

static void
holy_ofnet_findcards (void)
{
  /* Look at all nodes for devices of the type network.  */
  holy_ieee1275_devices_iterate (search_net_devices);
}

holy_MOD_INIT(ofnet)
{
  holy_ofnet_findcards ();
  holy_ieee1275_net_config = holy_ieee1275_net_config_real;
}

holy_MOD_FINI(ofnet)
{
  struct holy_net_card *card, *next;

  FOR_NET_CARDS_SAFE (card, next) 
    if (card->driver && holy_strcmp (card->driver->name, "ofnet") == 0)
      holy_net_card_unregister (card);
  holy_ieee1275_net_config = 0;
}
