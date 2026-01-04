/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/net/netbuff.h>
#include <holy/dl.h>
#include <holy/net.h>
#include <holy/time.h>
#include <holy/efi/api.h>
#include <holy/efi/efi.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

/* GUID.  */
static holy_efi_guid_t net_io_guid = holy_EFI_SIMPLE_NETWORK_GUID;
static holy_efi_guid_t pxe_io_guid = holy_EFI_PXE_GUID;

static holy_err_t
send_card_buffer (struct holy_net_card *dev,
		  struct holy_net_buff *pack)
{
  holy_efi_status_t st;
  holy_efi_simple_network_t *net = dev->efi_net;
  holy_uint64_t limit_time = holy_get_time_ms () + 4000;
  void *txbuf;

  if (dev->txbusy)
    while (1)
      {
	txbuf = NULL;
	st = efi_call_3 (net->get_status, net, 0, &txbuf);
	if (st != holy_EFI_SUCCESS)
	  return holy_error (holy_ERR_IO,
			     N_("couldn't send network packet"));
	/*
	   Some buggy firmware could return an arbitrary address instead of the
	   txbuf address we trasmitted, so just check that txbuf is non NULL
	   for success.  This is ok because we open the SNP protocol in
	   exclusive mode so we know we're the only ones transmitting on this
	   box and since we only transmit one packet at a time we know our
	   transmit was successfull.
	 */
	if (txbuf)
	  {
	    dev->txbusy = 0;
	    break;
	  }
	if (limit_time < holy_get_time_ms ())
	  return holy_error (holy_ERR_TIMEOUT,
			     N_("couldn't send network packet"));
      }

  dev->last_pkt_size = (pack->tail - pack->data);
  if (dev->last_pkt_size > dev->mtu)
    dev->last_pkt_size = dev->mtu;

  holy_memcpy (dev->txbuf, pack->data, dev->last_pkt_size);

  st = efi_call_7 (net->transmit, net, 0, dev->last_pkt_size,
		   dev->txbuf, NULL, NULL, NULL);
  if (st != holy_EFI_SUCCESS)
    return holy_error (holy_ERR_IO, N_("couldn't send network packet"));

  /*
     The card may have sent out the packet immediately - set txbusy
     to 0 in this case.
     Cases were observed where checking txbuf at the next call
     of send_card_buffer() is too late: 0 is returned in txbuf and
     we run in the holy_ERR_TIMEOUT case above.
     Perhaps a timeout in the FW has discarded the recycle buffer.
   */
  txbuf = NULL;
  st = efi_call_3 (net->get_status, net, 0, &txbuf);
  dev->txbusy = !(st == holy_EFI_SUCCESS && txbuf);

  return holy_ERR_NONE;
}

static struct holy_net_buff *
get_card_packet (struct holy_net_card *dev)
{
  holy_efi_simple_network_t *net = dev->efi_net;
  holy_err_t err;
  holy_efi_status_t st;
  holy_efi_uintn_t bufsize = dev->rcvbufsize;
  struct holy_net_buff *nb;
  int i;

  for (i = 0; i < 2; i++)
    {
      if (!dev->rcvbuf)
	dev->rcvbuf = holy_malloc (dev->rcvbufsize);
      if (!dev->rcvbuf)
	return NULL;

      st = efi_call_7 (net->receive, net, NULL, &bufsize,
		       dev->rcvbuf, NULL, NULL, NULL);
      if (st != holy_EFI_BUFFER_TOO_SMALL)
	break;
      dev->rcvbufsize = 2 * ALIGN_UP (dev->rcvbufsize > bufsize
				      ? dev->rcvbufsize : bufsize, 64);
      holy_free (dev->rcvbuf);
      dev->rcvbuf = 0;
    }

  if (st != holy_EFI_SUCCESS)
    return NULL;

  nb = holy_netbuff_alloc (bufsize + 2);
  if (!nb)
    return NULL;

  /* Reserve 2 bytes so that 2 + 14/18 bytes of ethernet header is divisible
     by 4. So that IP header is aligned on 4 bytes. */
  if (holy_netbuff_reserve (nb, 2))
    {
      holy_netbuff_free (nb);
      return NULL;
    }
  holy_memcpy (nb->data, dev->rcvbuf, bufsize);
  err = holy_netbuff_put (nb, bufsize);
  if (err)
    {
      holy_netbuff_free (nb);
      return NULL;
    }

  return nb;
}

static holy_err_t
open_card (struct holy_net_card *dev)
{
  holy_efi_simple_network_t *net;

  /* Try to reopen SNP exlusively to close any active MNP protocol instance
     that may compete for packet polling
   */
  net = holy_efi_open_protocol (dev->efi_handle, &net_io_guid,
				holy_EFI_OPEN_PROTOCOL_BY_EXCLUSIVE);
  if (net)
    {
      if (net->mode->state == holy_EFI_NETWORK_STOPPED
	  && efi_call_1 (net->start, net) != holy_EFI_SUCCESS)
	return holy_error (holy_ERR_NET_NO_CARD, "%s: net start failed",
			   dev->name);

      if (net->mode->state == holy_EFI_NETWORK_STOPPED)
	return holy_error (holy_ERR_NET_NO_CARD, "%s: card stopped",
			   dev->name);

      if (net->mode->state == holy_EFI_NETWORK_STARTED
	  && efi_call_3 (net->initialize, net, 0, 0) != holy_EFI_SUCCESS)
	return holy_error (holy_ERR_NET_NO_CARD, "%s: net initialize failed",
			   dev->name);

      /* Enable hardware receive filters if driver declares support for it.
	 We need unicast and broadcast and additionaly all nodes and
	 solicited multicast for IPv6. Solicited multicast is per-IPv6
	 address and we currently do not have API to do it so simply
	 try to enable receive of all multicast packets or evertyhing in
	 the worst case (i386 PXE driver always enables promiscuous too).

	 This does trust firmware to do what it claims to do.
       */
      if (net->mode->receive_filter_mask)
	{
	  holy_uint32_t filters = holy_EFI_SIMPLE_NETWORK_RECEIVE_UNICAST   |
				  holy_EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST |
				  holy_EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST;

	  filters &= net->mode->receive_filter_mask;
	  if (!(filters & holy_EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST))
	    filters |= (net->mode->receive_filter_mask &
			holy_EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS);

	  efi_call_6 (net->receive_filters, net, filters, 0, 0, 0, NULL);
	}

      efi_call_4 (holy_efi_system_table->boot_services->close_protocol,
		  dev->efi_net, &net_io_guid,
		  holy_efi_image_handle, dev->efi_handle);
      dev->efi_net = net;
    }

  /* If it failed we just try to run as best as we can */
  return holy_ERR_NONE;
}

static void
close_card (struct holy_net_card *dev)
{
  efi_call_1 (dev->efi_net->shutdown, dev->efi_net);
  efi_call_1 (dev->efi_net->stop, dev->efi_net);
  efi_call_4 (holy_efi_system_table->boot_services->close_protocol,
	      dev->efi_net, &net_io_guid,
	      holy_efi_image_handle, dev->efi_handle);
}

static struct holy_net_card_driver efidriver =
  {
    .name = "efinet",
    .open = open_card,
    .close = close_card,
    .send = send_card_buffer,
    .recv = get_card_packet
  };

holy_efi_handle_t
holy_efinet_get_device_handle (struct holy_net_card *card)
{
  if (!card || card->driver != &efidriver)
    return 0;
  return card->efi_handle;
}

static void
holy_efinet_findcards (void)
{
  holy_efi_uintn_t num_handles;
  holy_efi_handle_t *handles;
  holy_efi_handle_t *handle;
  int i = 0;

  /* Find handles which support the disk io interface.  */
  handles = holy_efi_locate_handle (holy_EFI_BY_PROTOCOL, &net_io_guid,
				    0, &num_handles);
  if (! handles)
    return;
  for (handle = handles; num_handles--; handle++)
    {
      holy_efi_simple_network_t *net;
      struct holy_net_card *card;
      holy_efi_device_path_t *dp, *parent = NULL, *child = NULL;

      /* EDK2 UEFI PXE driver creates IPv4 and IPv6 messaging devices as
	 children of main MAC messaging device. We only need one device with
	 bound SNP per physical card, otherwise they compete with each other
	 when polling for incoming packets.
       */
      dp = holy_efi_get_device_path (*handle);
      if (!dp)
	continue;
      for (; ! holy_EFI_END_ENTIRE_DEVICE_PATH (dp); dp = holy_EFI_NEXT_DEVICE_PATH (dp))
	{
	  parent = child;
	  child = dp;
	}
      if (child
	  && holy_EFI_DEVICE_PATH_TYPE (child) == holy_EFI_MESSAGING_DEVICE_PATH_TYPE
	  && (holy_EFI_DEVICE_PATH_SUBTYPE (child) == holy_EFI_IPV4_DEVICE_PATH_SUBTYPE
	      || holy_EFI_DEVICE_PATH_SUBTYPE (child) == holy_EFI_IPV6_DEVICE_PATH_SUBTYPE)
	  && parent
	  && holy_EFI_DEVICE_PATH_TYPE (parent) == holy_EFI_MESSAGING_DEVICE_PATH_TYPE
	  && holy_EFI_DEVICE_PATH_SUBTYPE (parent) == holy_EFI_MAC_ADDRESS_DEVICE_PATH_SUBTYPE)
	continue;

      net = holy_efi_open_protocol (*handle, &net_io_guid,
				    holy_EFI_OPEN_PROTOCOL_GET_PROTOCOL);
      if (! net)
	/* This should not happen... Why?  */
	continue;

      if (net->mode->state == holy_EFI_NETWORK_STOPPED
	  && efi_call_1 (net->start, net) != holy_EFI_SUCCESS)
	continue;

      if (net->mode->state == holy_EFI_NETWORK_STOPPED)
	continue;

      if (net->mode->state == holy_EFI_NETWORK_STARTED
	  && efi_call_3 (net->initialize, net, 0, 0) != holy_EFI_SUCCESS)
	continue;

      card = holy_zalloc (sizeof (struct holy_net_card));
      if (!card)
	{
	  holy_print_error ();
	  holy_free (handles);
	  return;
	}

      card->mtu = net->mode->max_packet_size;
      card->txbufsize = ALIGN_UP (card->mtu, 64) + 256;
      card->txbuf = holy_zalloc (card->txbufsize);
      if (!card->txbuf)
	{
	  holy_print_error ();
	  holy_free (handles);
	  holy_free (card);
	  return;
	}
      card->txbusy = 0;

      card->rcvbufsize = ALIGN_UP (card->mtu, 64) + 256;

      card->name = holy_xasprintf ("efinet%d", i++);
      card->driver = &efidriver;
      card->flags = 0;
      card->default_address.type = holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET;
      holy_memcpy (card->default_address.mac,
		   net->mode->current_address,
		   sizeof (card->default_address.mac));
      card->efi_net = net;
      card->efi_handle = *handle;

      holy_net_card_register (card);
    }
  holy_free (handles);
}

static void
holy_efi_net_config_real (holy_efi_handle_t hnd, char **device,
			  char **path)
{
  struct holy_net_card *card;
  holy_efi_device_path_t *dp;

  dp = holy_efi_get_device_path (hnd);
  if (! dp)
    return;

  FOR_NET_CARDS (card)
  {
    holy_efi_device_path_t *cdp;
    struct holy_efi_pxe *pxe;
    struct holy_efi_pxe_mode *pxe_mode;
    if (card->driver != &efidriver)
      continue;
    cdp = holy_efi_get_device_path (card->efi_handle);
    if (! cdp)
      continue;
    if (holy_efi_compare_device_paths (dp, cdp) != 0)
      {
	holy_efi_device_path_t *ldp, *dup_dp, *dup_ldp;
	int match;

	/* EDK2 UEFI PXE driver creates pseudo devices with type IPv4/IPv6
	   as children of Ethernet card and binds PXE and Load File protocols
	   to it. Loaded Image Device Path protocol will point to these pseudo
	   devices. We skip them when enumerating cards, so here we need to
	   find matching MAC device.
         */
	ldp = holy_efi_find_last_device_path (dp);
	if (holy_EFI_DEVICE_PATH_TYPE (ldp) != holy_EFI_MESSAGING_DEVICE_PATH_TYPE
	    || (holy_EFI_DEVICE_PATH_SUBTYPE (ldp) != holy_EFI_IPV4_DEVICE_PATH_SUBTYPE
		&& holy_EFI_DEVICE_PATH_SUBTYPE (ldp) != holy_EFI_IPV6_DEVICE_PATH_SUBTYPE))
	  continue;
	dup_dp = holy_efi_duplicate_device_path (dp);
	if (!dup_dp)
	  continue;
	dup_ldp = holy_efi_find_last_device_path (dup_dp);
	dup_ldp->type = holy_EFI_END_DEVICE_PATH_TYPE;
	dup_ldp->subtype = holy_EFI_END_ENTIRE_DEVICE_PATH_SUBTYPE;
	dup_ldp->length = sizeof (*dup_ldp);
	match = holy_efi_compare_device_paths (dup_dp, cdp) == 0;
	holy_free (dup_dp);
	if (!match)
	  continue;
      }
    pxe = holy_efi_open_protocol (hnd, &pxe_io_guid,
				  holy_EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (! pxe)
      continue;
    pxe_mode = pxe->mode;
    holy_net_configure_by_dhcp_ack (card->name, card, 0,
				    (struct holy_net_bootp_packet *)
				    &pxe_mode->dhcp_ack,
				    sizeof (pxe_mode->dhcp_ack),
				    1, device, path);
    return;
  }
}

holy_MOD_INIT(efinet)
{
  holy_efinet_findcards ();
  holy_efi_net_config = holy_efi_net_config_real;
}

holy_MOD_FINI(efinet)
{
  struct holy_net_card *card, *next;

  FOR_NET_CARDS_SAFE (card, next) 
    if (card->driver == &efidriver)
      holy_net_card_unregister (card);
}

