/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/net/udp.h>
#include <holy/net/ip.h>
#include <holy/net/ethernet.h>
#include <holy/net/netbuff.h>
#include <holy/net.h>
#include <holy/mm.h>
#include <holy/dl.h>
#include <holy/file.h>
#include <holy/priority_queue.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

/* IP port for the MTFTP server used for Intel's PXE */
enum
  {
    MTFTP_SERVER_PORT = 75,
    MTFTP_CLIENT_PORT = 76,
    /* IP port for the TFTP server */
    TFTP_SERVER_PORT = 69
  };

enum
  {
    TFTP_DEFAULTSIZE_PACKET = 512,
  };

enum
  {
    TFTP_CODE_EOF = 1,
    TFTP_CODE_MORE = 2,
    TFTP_CODE_ERROR = 3,
    TFTP_CODE_BOOT = 4,
    TFTP_CODE_CFG = 5
  };

enum
  {
    TFTP_RRQ = 1,
    TFTP_WRQ = 2,
    TFTP_DATA = 3,
    TFTP_ACK = 4,
    TFTP_ERROR = 5,
    TFTP_OACK = 6
  };

enum
  {
    TFTP_EUNDEF = 0,                   /* not defined */
    TFTP_ENOTFOUND = 1,                /* file not found */
    TFTP_EACCESS = 2,                  /* access violation */
    TFTP_ENOSPACE = 3,                 /* disk full or allocation exceeded */
    TFTP_EBADOP = 4,                   /* illegal TFTP operation */
    TFTP_EBADID = 5,                   /* unknown transfer ID */
    TFTP_EEXISTS = 6,                  /* file already exists */
    TFTP_ENOUSER = 7                  /* no such user */
  };

struct tftphdr {
  holy_uint16_t opcode;
  union {
    holy_int8_t rrq[TFTP_DEFAULTSIZE_PACKET];
    struct {
      holy_uint16_t block;
      holy_int8_t download[0];
    } data;
    struct {
      holy_uint16_t block;
    } ack;
    struct {
      holy_uint16_t errcode;
      holy_int8_t errmsg[TFTP_DEFAULTSIZE_PACKET];
    } err;
    struct {
      holy_int8_t data[TFTP_DEFAULTSIZE_PACKET+2];
    } oack;
  } u;
} holy_PACKED ;


typedef struct tftp_data
{
  holy_uint64_t file_size;
  holy_uint64_t block;
  holy_uint32_t block_size;
  holy_uint64_t ack_sent;
  int have_oack;
  struct holy_error_saved save_err;
  holy_net_udp_socket_t sock;
  holy_priority_queue_t pq;
} *tftp_data_t;

static int
cmp_block (holy_uint16_t a, holy_uint16_t b)
{
  holy_int16_t i = (holy_int16_t) (a - b);
  if (i > 0)
    return +1;
  if (i < 0)
    return -1;
  return 0;
}

static int
cmp (const void *a__, const void *b__)
{
  struct holy_net_buff *a_ = *(struct holy_net_buff **) a__;
  struct holy_net_buff *b_ = *(struct holy_net_buff **) b__;
  struct tftphdr *a = (struct tftphdr *) a_->data;
  struct tftphdr *b = (struct tftphdr *) b_->data;
  /* We want the first elements to be on top.  */
  return -cmp_block (holy_be_to_cpu16 (a->u.data.block), holy_be_to_cpu16 (b->u.data.block));
}

static holy_err_t
ack (tftp_data_t data, holy_uint64_t block)
{
  struct tftphdr *tftph_ack;
  holy_uint8_t nbdata[512];
  struct holy_net_buff nb_ack;
  holy_err_t err;

  nb_ack.head = nbdata;
  nb_ack.end = nbdata + sizeof (nbdata);
  holy_netbuff_clear (&nb_ack);
  holy_netbuff_reserve (&nb_ack, 512);
  err = holy_netbuff_push (&nb_ack, sizeof (tftph_ack->opcode)
			   + sizeof (tftph_ack->u.ack.block));
  if (err)
    return err;

  tftph_ack = (struct tftphdr *) nb_ack.data;
  tftph_ack->opcode = holy_cpu_to_be16_compile_time (TFTP_ACK);
  tftph_ack->u.ack.block = holy_cpu_to_be16 (block);

  err = holy_net_send_udp_packet (data->sock, &nb_ack);
  if (err)
    return err;
  data->ack_sent = block;
  return holy_ERR_NONE;
}

static holy_err_t
tftp_receive (holy_net_udp_socket_t sock __attribute__ ((unused)),
	      struct holy_net_buff *nb,
	      void *f)
{
  holy_file_t file = f;
  struct tftphdr *tftph = (void *) nb->data;
  tftp_data_t data = file->data;
  holy_err_t err;
  holy_uint8_t *ptr;

  if (nb->tail - nb->data < (holy_ssize_t) sizeof (tftph->opcode))
    {
      holy_dprintf ("tftp", "TFTP packet too small\n");
      return holy_ERR_NONE;
    }

  tftph = (struct tftphdr *) nb->data;
  switch (holy_be_to_cpu16 (tftph->opcode))
    {
    case TFTP_OACK:
      data->block_size = TFTP_DEFAULTSIZE_PACKET;
      data->have_oack = 1; 
      for (ptr = nb->data + sizeof (tftph->opcode); ptr < nb->tail;)
	{
	  if (holy_memcmp (ptr, "tsize\0", sizeof ("tsize\0") - 1) == 0)
	    data->file_size = holy_strtoul ((char *) ptr + sizeof ("tsize\0")
					    - 1, 0, 0);
	  if (holy_memcmp (ptr, "blksize\0", sizeof ("blksize\0") - 1) == 0)
	    data->block_size = holy_strtoul ((char *) ptr + sizeof ("blksize\0")
					     - 1, 0, 0);
	  while (ptr < nb->tail && *ptr)
	    ptr++;
	  ptr++;
	}
      data->block = 0;
      holy_netbuff_free (nb);
      err = ack (data, 0);
      holy_error_save (&data->save_err);
      return holy_ERR_NONE;
    case TFTP_DATA:
      if (nb->tail - nb->data < (holy_ssize_t) (sizeof (tftph->opcode)
						+ sizeof (tftph->u.data.block)))
	{
	  holy_dprintf ("tftp", "TFTP packet too small\n");
	  return holy_ERR_NONE;
	}

      err = holy_priority_queue_push (data->pq, &nb);
      if (err)
	return err;

      {
	struct holy_net_buff **nb_top_p, *nb_top;
	while (1)
	  {
	    nb_top_p = holy_priority_queue_top (data->pq);
	    if (!nb_top_p)
	      return holy_ERR_NONE;
	    nb_top = *nb_top_p;
	    tftph = (struct tftphdr *) nb_top->data;
	    if (cmp_block (holy_be_to_cpu16 (tftph->u.data.block), data->block + 1) >= 0)
	      break;
	    ack (data, holy_be_to_cpu16 (tftph->u.data.block));
	    holy_netbuff_free (nb_top);
	    holy_priority_queue_pop (data->pq);
	  }
	while (cmp_block (holy_be_to_cpu16 (tftph->u.data.block), data->block + 1) == 0)
	  {
	    unsigned size;

	    holy_priority_queue_pop (data->pq);

	    if (file->device->net->packs.count < 50)
	      err = ack (data, data->block + 1);
	    else
	      {
		file->device->net->stall = 1;
		err = 0;
	      }
	    if (err)
	      return err;

	    err = holy_netbuff_pull (nb_top, sizeof (tftph->opcode) +
				     sizeof (tftph->u.data.block));
	    if (err)
	      return err;
	    size = nb_top->tail - nb_top->data;

	    data->block++;
	    if (size < data->block_size)
	      {
		if (data->ack_sent < data->block)
		  ack (data, data->block);
		file->device->net->eof = 1;
		file->device->net->stall = 1;
		holy_net_udp_close (data->sock);
		data->sock = NULL;
	      }
	    /* Prevent garbage in broken cards. Is it still necessary
	       given that IP implementation has been fixed?
	     */
	    if (size > data->block_size)
	      {
		err = holy_netbuff_unput (nb_top, size - data->block_size);
		if (err)
		  return err;
	      }
	    /* If there is data, puts packet in socket list. */
	    if ((nb_top->tail - nb_top->data) > 0)
	      holy_net_put_packet (&file->device->net->packs, nb_top);
	    else
	      holy_netbuff_free (nb_top);
	  }
      }
      return holy_ERR_NONE;
    case TFTP_ERROR:
      data->have_oack = 1;
      holy_netbuff_free (nb);
      holy_error (holy_ERR_IO, (char *) tftph->u.err.errmsg);
      holy_error_save (&data->save_err);
      return holy_ERR_NONE;
    default:
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }
}

static void
destroy_pq (tftp_data_t data)
{
  struct holy_net_buff **nb_p;
  while ((nb_p = holy_priority_queue_top (data->pq)))
    {
      holy_netbuff_free (*nb_p);
      holy_priority_queue_pop (data->pq);
    }

  holy_priority_queue_destroy (data->pq);
}

static holy_err_t
tftp_open (struct holy_file *file, const char *filename)
{
  struct tftphdr *tftph;
  char *rrq;
  int i;
  int rrqlen;
  int hdrlen;
  holy_uint8_t open_data[1500];
  struct holy_net_buff nb;
  tftp_data_t data;
  holy_err_t err;
  holy_uint8_t *nbd;
  holy_net_network_level_address_t addr;

  data = holy_zalloc (sizeof (*data));
  if (!data)
    return holy_errno;

  nb.head = open_data;
  nb.end = open_data + sizeof (open_data);
  holy_netbuff_clear (&nb);

  holy_netbuff_reserve (&nb, 1500);
  err = holy_netbuff_push (&nb, sizeof (*tftph));
  if (err)
    {
      holy_free (data);
      return err;
    }

  tftph = (struct tftphdr *) nb.data;

  rrq = (char *) tftph->u.rrq;
  rrqlen = 0;

  tftph->opcode = holy_cpu_to_be16_compile_time (TFTP_RRQ);
  holy_strcpy (rrq, filename);
  rrqlen += holy_strlen (filename) + 1;
  rrq += holy_strlen (filename) + 1;

  holy_strcpy (rrq, "octet");
  rrqlen += holy_strlen ("octet") + 1;
  rrq += holy_strlen ("octet") + 1;

  holy_strcpy (rrq, "blksize");
  rrqlen += holy_strlen ("blksize") + 1;
  rrq += holy_strlen ("blksize") + 1;

  holy_strcpy (rrq, "1024");
  rrqlen += holy_strlen ("1024") + 1;
  rrq += holy_strlen ("1024") + 1;

  holy_strcpy (rrq, "tsize");
  rrqlen += holy_strlen ("tsize") + 1;
  rrq += holy_strlen ("tsize") + 1;

  holy_strcpy (rrq, "0");
  rrqlen += holy_strlen ("0") + 1;
  rrq += holy_strlen ("0") + 1;
  hdrlen = sizeof (tftph->opcode) + rrqlen;

  err = holy_netbuff_unput (&nb, nb.tail - (nb.data + hdrlen));
  if (err)
    {
      holy_free (data);
      return err;
    }

  file->not_easily_seekable = 1;
  file->data = data;

  data->pq = holy_priority_queue_new (sizeof (struct holy_net_buff *), cmp);
  if (!data->pq)
    {
      holy_free (data);
      return holy_errno;
    }

  err = holy_net_resolve_address (file->device->net->server, &addr);
  if (err)
    {
      destroy_pq (data);
      holy_free (data);
      return err;
    }

  data->sock = holy_net_udp_open (addr,
				  TFTP_SERVER_PORT, tftp_receive,
				  file);
  if (!data->sock)
    {
      destroy_pq (data);
      holy_free (data);
      return holy_errno;
    }

  /* Receive OACK packet.  */
  nbd = nb.data;
  for (i = 0; i < holy_NET_TRIES; i++)
    {
      nb.data = nbd;
      err = holy_net_send_udp_packet (data->sock, &nb);
      if (err)
	{
	  holy_net_udp_close (data->sock);
	  destroy_pq (data);
	  holy_free (data);
	  return err;
	}
      holy_net_poll_cards (holy_NET_INTERVAL + (i * holy_NET_INTERVAL_ADDITION),
                           &data->have_oack);
      if (data->have_oack)
	break;
    }

  if (!data->have_oack)
    holy_error (holy_ERR_TIMEOUT, N_("time out opening `%s'"), filename);
  else
    holy_error_load (&data->save_err);
  if (holy_errno)
    {
      holy_net_udp_close (data->sock);
      destroy_pq (data);
      holy_free (data);
      return holy_errno;
    }

  file->size = data->file_size;

  return holy_ERR_NONE;
}

static holy_err_t
tftp_close (struct holy_file *file)
{
  tftp_data_t data = file->data;

  if (data->sock)
    {
      holy_uint8_t nbdata[512];
      holy_err_t err;
      struct holy_net_buff nb_err;
      struct tftphdr *tftph;

      nb_err.head = nbdata;
      nb_err.end = nbdata + sizeof (nbdata);

      holy_netbuff_clear (&nb_err);
      holy_netbuff_reserve (&nb_err, 512);
      err = holy_netbuff_push (&nb_err, sizeof (tftph->opcode)
			       + sizeof (tftph->u.err.errcode)
			       + sizeof ("closed"));
      if (!err)
	{
	  tftph = (struct tftphdr *) nb_err.data;
	  tftph->opcode = holy_cpu_to_be16_compile_time (TFTP_ERROR);
	  tftph->u.err.errcode = holy_cpu_to_be16_compile_time (TFTP_EUNDEF);
	  holy_memcpy (tftph->u.err.errmsg, "closed", sizeof ("closed"));

	  err = holy_net_send_udp_packet (data->sock, &nb_err);
	}
      if (err)
	holy_print_error ();
      holy_net_udp_close (data->sock);
    }
  destroy_pq (data);
  holy_free (data);
  return holy_ERR_NONE;
}

static holy_err_t
tftp_packets_pulled (struct holy_file *file)
{
  tftp_data_t data = file->data;
  if (file->device->net->packs.count >= 50)
    return 0;

  if (!file->device->net->eof)
    file->device->net->stall = 0;
  if (data->ack_sent >= data->block)
    return 0;
  return ack (data, data->block);
}

static struct holy_net_app_protocol holy_tftp_protocol =
  {
    .name = "tftp",
    .open = tftp_open,
    .close = tftp_close,
    .packets_pulled = tftp_packets_pulled
  };

holy_MOD_INIT (tftp)
{
  holy_net_app_level_register (&holy_tftp_protocol);
}

holy_MOD_FINI (tftp)
{
  holy_net_app_level_unregister (&holy_tftp_protocol);
}
