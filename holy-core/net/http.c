/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/net/tcp.h>
#include <holy/net/ip.h>
#include <holy/net/ethernet.h>
#include <holy/net/netbuff.h>
#include <holy/net.h>
#include <holy/mm.h>
#include <holy/dl.h>
#include <holy/file.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

enum
  {
    HTTP_PORT = 80
  };


typedef struct http_data
{
  char *current_line;
  holy_size_t current_line_len;
  int headers_recv;
  int first_line_recv;
  int size_recv;
  holy_net_tcp_socket_t sock;
  char *filename;
  holy_err_t err;
  char *errmsg;
  int chunked;
  holy_size_t chunk_rem;
  int in_chunk_len;
} *http_data_t;

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
parse_line (holy_file_t file, http_data_t data, char *ptr, holy_size_t len)
{
  char *end = ptr + len;
  while (end > ptr && *(end - 1) == '\r')
    end--;
  *end = 0;
  /* Trailing CRLF.  */
  if (data->in_chunk_len == 1)
    {
      data->in_chunk_len = 2;
      return holy_ERR_NONE;
    }
  if (data->in_chunk_len == 2)
    {
      data->chunk_rem = holy_strtoul (ptr, 0, 16);
      holy_errno = holy_ERR_NONE;
      if (data->chunk_rem == 0)
	{
	  file->device->net->eof = 1;
	  file->device->net->stall = 1;
	  if (file->size == holy_FILE_SIZE_UNKNOWN)
	    file->size = have_ahead (file);
	}
      data->in_chunk_len = 0;
      return holy_ERR_NONE;
    }
  if (ptr == end)
    {
      data->headers_recv = 1;
      if (data->chunked)
	data->in_chunk_len = 2;
      return holy_ERR_NONE;
    }

  if (!data->first_line_recv)
    {
      int code;
      if (holy_memcmp (ptr, "HTTP/1.1 ", sizeof ("HTTP/1.1 ") - 1) != 0)
	{
	  data->errmsg = holy_strdup (_("unsupported HTTP response"));
	  data->first_line_recv = 1;
	  return holy_ERR_NONE;
	}
      ptr += sizeof ("HTTP/1.1 ") - 1;
      code = holy_strtoul (ptr, &ptr, 10);
      if (holy_errno)
	return holy_errno;
      switch (code)
	{
	case 200:
	case 206:
	  break;
	case 404:
	  data->err = holy_ERR_FILE_NOT_FOUND;
	  data->errmsg = holy_xasprintf (_("file `%s' not found"), data->filename);
	  return holy_ERR_NONE;
	default:
	  data->err = holy_ERR_NET_UNKNOWN_ERROR;
	  /* TRANSLATORS: holy HTTP code is pretty young. So even perfectly
	     valid answers like 403 will trigger this very generic message.  */
	  data->errmsg = holy_xasprintf (_("unsupported HTTP error %d: %s"),
					 code, ptr);
	  return holy_ERR_NONE;
	}
      data->first_line_recv = 1;
      return holy_ERR_NONE;
    }
  if (holy_memcmp (ptr, "Content-Length: ", sizeof ("Content-Length: ") - 1)
      == 0 && !data->size_recv)
    {
      ptr += sizeof ("Content-Length: ") - 1;
      file->size = holy_strtoull (ptr, &ptr, 10);
      data->size_recv = 1;
      return holy_ERR_NONE;
    }
  if (holy_memcmp (ptr, "Transfer-Encoding: chunked",
		   sizeof ("Transfer-Encoding: chunked") - 1) == 0)
    {
      data->chunked = 1;
      return holy_ERR_NONE;
    }

  return holy_ERR_NONE;
}

static void
http_err (holy_net_tcp_socket_t sock __attribute__ ((unused)),
	  void *f)
{
  holy_file_t file = f;
  http_data_t data = file->data;

  if (data->sock)
    holy_net_tcp_close (data->sock, holy_NET_TCP_ABORT);
  data->sock = 0;
  if (data->current_line)
    holy_free (data->current_line);
  data->current_line = 0;
  file->device->net->eof = 1;
  file->device->net->stall = 1;
  if (file->size == holy_FILE_SIZE_UNKNOWN)
    file->size = have_ahead (file);
}

static holy_err_t
http_receive (holy_net_tcp_socket_t sock __attribute__ ((unused)),
	      struct holy_net_buff *nb,
	      void *f)
{
  holy_file_t file = f;
  http_data_t data = file->data;
  holy_err_t err;

  if (!data->sock)
    {
      holy_netbuff_free (nb);
      return holy_ERR_NONE;
    }

  while (1)
    {
      char *ptr = (char *) nb->data;
      if ((!data->headers_recv || data->in_chunk_len) && data->current_line)
	{
	  int have_line = 1;
	  char *t;
	  ptr = holy_memchr (nb->data, '\n', nb->tail - nb->data);
	  if (ptr)
	    ptr++;
	  else
	    {
	      have_line = 0;
	      ptr = (char *) nb->tail;
	    }
	  t = holy_realloc (data->current_line,
			    data->current_line_len + (ptr - (char *) nb->data));
	  if (!t)
	    {
	      holy_netbuff_free (nb);
	      holy_net_tcp_close (data->sock, holy_NET_TCP_ABORT);
	      return holy_errno;
	    }
	      
	  data->current_line = t;
	  holy_memcpy (data->current_line + data->current_line_len,
		       nb->data, ptr - (char *) nb->data);
	  data->current_line_len += ptr - (char *) nb->data;
	  if (!have_line)
	    {
	      holy_netbuff_free (nb);
	      return holy_ERR_NONE;
	    }
	  err = parse_line (file, data, data->current_line,
			    data->current_line_len);
	  holy_free (data->current_line);
	  data->current_line = 0;
	  data->current_line_len = 0;
	  if (err)
	    {
	      holy_net_tcp_close (data->sock, holy_NET_TCP_ABORT);
	      holy_netbuff_free (nb);
	      return err;
	    }
	}

      while (ptr < (char *) nb->tail && (!data->headers_recv
					 || data->in_chunk_len))
	{
	  char *ptr2;
	  ptr2 = holy_memchr (ptr, '\n', (char *) nb->tail - ptr);
	  if (!ptr2)
	    {
	      data->current_line = holy_malloc ((char *) nb->tail - ptr);
	      if (!data->current_line)
		{
		  holy_netbuff_free (nb);
		  holy_net_tcp_close (data->sock, holy_NET_TCP_ABORT);
		  return holy_errno;
		}
	      data->current_line_len = (char *) nb->tail - ptr;
	      holy_memcpy (data->current_line, ptr, data->current_line_len);
	      holy_netbuff_free (nb);
	      return holy_ERR_NONE;
	    }
	  err = parse_line (file, data, ptr, ptr2 - ptr);
	  if (err)
	    {
	      holy_net_tcp_close (data->sock, holy_NET_TCP_ABORT);
	      holy_netbuff_free (nb);
	      return err;
	    }
	  ptr = ptr2 + 1;
	}

      if (((char *) nb->tail - ptr) <= 0)
	{
	  holy_netbuff_free (nb);
	  return holy_ERR_NONE;
	} 
      err = holy_netbuff_pull (nb, ptr - (char *) nb->data);
      if (err)
	{
	  holy_net_tcp_close (data->sock, holy_NET_TCP_ABORT);
	  holy_netbuff_free (nb);
	  return err;
	}
      if (!(data->chunked && (holy_ssize_t) data->chunk_rem
	    < nb->tail - nb->data))
	{
	  holy_net_put_packet (&file->device->net->packs, nb);
	  if (file->device->net->packs.count >= 20)
	    file->device->net->stall = 1;

	  if (file->device->net->packs.count >= 100)
	    holy_net_tcp_stall (data->sock);

	  if (data->chunked)
	    data->chunk_rem -= nb->tail - nb->data;
	  return holy_ERR_NONE;
	}
      if (data->chunk_rem)
	{
	  struct holy_net_buff *nb2;
	  nb2 = holy_netbuff_alloc (data->chunk_rem);
	  if (!nb2)
	    return holy_errno;
	  holy_netbuff_put (nb2, data->chunk_rem);
	  holy_memcpy (nb2->data, nb->data, data->chunk_rem);
	  if (file->device->net->packs.count >= 20)
	    {
	      file->device->net->stall = 1;
	      holy_net_tcp_stall (data->sock);
	    }

	  holy_net_put_packet (&file->device->net->packs, nb2);
	  holy_netbuff_pull (nb, data->chunk_rem);
	}
      data->in_chunk_len = 1;
    }
}

static holy_err_t
http_establish (struct holy_file *file, holy_off_t offset, int initial)
{
  http_data_t data = file->data;
  holy_uint8_t *ptr;
  int i, port;
  struct holy_net_buff *nb;
  holy_err_t err;

  nb = holy_netbuff_alloc (holy_NET_TCP_RESERVE_SIZE
			   + sizeof ("GET ") - 1
			   + holy_strlen (data->filename)
			   + sizeof (" HTTP/1.1\r\nHost: ") - 1
			   + holy_strlen (file->device->net->server)
			   + sizeof ("\r\nUser-Agent: " PACKAGE_STRING
				     "\r\n") - 1
			   + sizeof ("Range: bytes=XXXXXXXXXXXXXXXXXXXX"
				     "-\r\n\r\n"));
  if (!nb)
    return holy_errno;

  holy_netbuff_reserve (nb, holy_NET_TCP_RESERVE_SIZE);
  ptr = nb->tail;
  err = holy_netbuff_put (nb, sizeof ("GET ") - 1);
  if (err)
    {
      holy_netbuff_free (nb);
      return err;
    }
  holy_memcpy (ptr, "GET ", sizeof ("GET ") - 1);

  ptr = nb->tail;

  err = holy_netbuff_put (nb, holy_strlen (data->filename));
  if (err)
    {
      holy_netbuff_free (nb);
      return err;
    }
  holy_memcpy (ptr, data->filename, holy_strlen (data->filename));

  ptr = nb->tail;
  err = holy_netbuff_put (nb, sizeof (" HTTP/1.1\r\nHost: ") - 1);
  if (err)
    {
      holy_netbuff_free (nb);
      return err;
    }
  holy_memcpy (ptr, " HTTP/1.1\r\nHost: ",
	       sizeof (" HTTP/1.1\r\nHost: ") - 1);

  ptr = nb->tail;
  err = holy_netbuff_put (nb, holy_strlen (file->device->net->server));
  if (err)
    {
      holy_netbuff_free (nb);
      return err;
    }
  holy_memcpy (ptr, file->device->net->server,
	       holy_strlen (file->device->net->server));

  ptr = nb->tail;
  err = holy_netbuff_put (nb,
			  sizeof ("\r\nUser-Agent: " PACKAGE_STRING "\r\n")
			  - 1);
  if (err)
    {
      holy_netbuff_free (nb);
      return err;
    }
  holy_memcpy (ptr, "\r\nUser-Agent: " PACKAGE_STRING "\r\n",
	       sizeof ("\r\nUser-Agent: " PACKAGE_STRING "\r\n") - 1);
  if (!initial)
    {
      ptr = nb->tail;
      holy_snprintf ((char *) ptr,
		     sizeof ("Range: bytes=XXXXXXXXXXXXXXXXXXXX-"
			     "\r\n"),
		     "Range: bytes=%" PRIuholy_UINT64_T "-\r\n",
		     offset);
      holy_netbuff_put (nb, holy_strlen ((char *) ptr));
    }
  ptr = nb->tail;
  holy_netbuff_put (nb, 2);
  holy_memcpy (ptr, "\r\n", 2);

  if (file->device->net->port)
    port = file->device->net->port;
  else
    port = HTTP_PORT;
  data->sock = holy_net_tcp_open (file->device->net->server,
				  port, http_receive,
				  http_err, http_err,
				  file);
  if (!data->sock)
    {
      holy_netbuff_free (nb);
      return holy_errno;
    }

  //  holy_net_poll_cards (5000);

  err = holy_net_send_tcp_packet (data->sock, nb, 1);
  if (err)
    {
      holy_net_tcp_close (data->sock, holy_NET_TCP_ABORT);
      return err;
    }

  for (i = 0; !data->headers_recv && i < 100; i++)
    {
      holy_net_tcp_retransmit ();
      holy_net_poll_cards (300, &data->headers_recv);
    }

  if (!data->headers_recv)
    {
      holy_net_tcp_close (data->sock, holy_NET_TCP_ABORT);
      if (data->err)
	{
	  char *str = data->errmsg;
	  err = holy_error (data->err, "%s", str);
	  holy_free (str);
	  data->errmsg = 0;
	  return data->err;
	}
      return holy_error (holy_ERR_TIMEOUT, N_("time out opening `%s'"), data->filename);
    }
  return holy_ERR_NONE;
}

static holy_err_t
http_seek (struct holy_file *file, holy_off_t off)
{
  struct http_data *old_data, *data;
  holy_err_t err;
  old_data = file->data;
  /* FIXME: Reuse socket?  */
  if (old_data->sock)
    holy_net_tcp_close (old_data->sock, holy_NET_TCP_ABORT);
  old_data->sock = 0;

  while (file->device->net->packs.first)
    {
      holy_netbuff_free (file->device->net->packs.first->nb);
      holy_net_remove_packet (file->device->net->packs.first);
    }

  file->device->net->stall = 0;
  file->device->net->eof = 0;
  file->device->net->offset = off;

  data = holy_zalloc (sizeof (*data));
  if (!data)
    return holy_errno;

  data->size_recv = 1;
  data->filename = old_data->filename;
  if (!data->filename)
    {
      holy_free (data);
      file->data = 0;
      return holy_errno;
    }
  holy_free (old_data);

  file->data = data;
  err = http_establish (file, off, 0);
  if (err)
    {
      holy_free (data->filename);
      holy_free (data);
      file->data = 0;
      return err;
    }
  return holy_ERR_NONE;
}

static holy_err_t
http_open (struct holy_file *file, const char *filename)
{
  holy_err_t err;
  struct http_data *data;

  data = holy_zalloc (sizeof (*data));
  if (!data)
    return holy_errno;
  file->size = holy_FILE_SIZE_UNKNOWN;

  data->filename = holy_strdup (filename);
  if (!data->filename)
    {
      holy_free (data);
      return holy_errno;
    }

  file->not_easily_seekable = 0;
  file->data = data;

  err = http_establish (file, 0, 1);
  if (err)
    {
      holy_free (data->filename);
      holy_free (data);
      return err;
    }

  return holy_ERR_NONE;
}

static holy_err_t
http_close (struct holy_file *file)
{
  http_data_t data = file->data;

  if (!data)
    return holy_ERR_NONE;

  if (data->sock)
    holy_net_tcp_close (data->sock, holy_NET_TCP_ABORT);
  if (data->current_line)
    holy_free (data->current_line);
  holy_free (data->filename);
  holy_free (data);
  return holy_ERR_NONE;
}

static holy_err_t
http_packets_pulled (struct holy_file *file)
{
  http_data_t data = file->data;

  if (file->device->net->packs.count >= 20)
    return 0;

  if (!file->device->net->eof)
    file->device->net->stall = 0;
  if (data && data->sock)
    holy_net_tcp_unstall (data->sock);
  return 0;
}

static struct holy_net_app_protocol holy_http_protocol =
  {
    .name = "http",
    .open = http_open,
    .close = http_close,
    .seek = http_seek,
    .packets_pulled = http_packets_pulled
  };

holy_MOD_INIT (http)
{
  holy_net_app_level_register (&holy_http_protocol);
}

holy_MOD_FINI (http)
{
  holy_net_app_level_unregister (&holy_http_protocol);
}
