/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/net/netbuff.h>

holy_err_t
holy_netbuff_put (struct holy_net_buff *nb, holy_size_t len)
{
  nb->tail += len;
  if (nb->tail > nb->end)
    return holy_error (holy_ERR_BUG, "put out of the packet range.");
  return holy_ERR_NONE;
}

holy_err_t
holy_netbuff_unput (struct holy_net_buff *nb, holy_size_t len)
{
  nb->tail -= len;
  if (nb->tail < nb->head)
    return holy_error (holy_ERR_BUG,
		       "unput out of the packet range.");
  return holy_ERR_NONE;
}

holy_err_t
holy_netbuff_push (struct holy_net_buff *nb, holy_size_t len)
{
  nb->data -= len;
  if (nb->data < nb->head)
    return holy_error (holy_ERR_BUG,
		       "push out of the packet range.");
  return holy_ERR_NONE;
}

holy_err_t
holy_netbuff_pull (struct holy_net_buff *nb, holy_size_t len)
{
  nb->data += len;
  if (nb->data > nb->end)
    return holy_error (holy_ERR_BUG,
		       "pull out of the packet range.");
  return holy_ERR_NONE;
}

holy_err_t
holy_netbuff_reserve (struct holy_net_buff *nb, holy_size_t len)
{
  nb->data += len;
  nb->tail += len;
  if ((nb->tail > nb->end) || (nb->data > nb->end))
    return holy_error (holy_ERR_BUG,
		       "reserve out of the packet range.");
  return holy_ERR_NONE;
}

struct holy_net_buff *
holy_netbuff_alloc (holy_size_t len)
{
  struct holy_net_buff *nb;
  void *data;

  COMPILE_TIME_ASSERT (NETBUFF_ALIGN % sizeof (holy_properly_aligned_t) == 0);

  if (len < NETBUFFMINLEN)
    len = NETBUFFMINLEN;

  len = ALIGN_UP (len, NETBUFF_ALIGN);
#ifdef holy_MACHINE_EMU
  data = holy_malloc (len + sizeof (*nb));
#else
  data = holy_memalign (NETBUFF_ALIGN, len + sizeof (*nb));
#endif
  if (!data)
    return NULL;
  nb = (struct holy_net_buff *) ((holy_properly_aligned_t *) data
				 + len / sizeof (holy_properly_aligned_t));
  nb->head = nb->data = nb->tail = data;
  nb->end = (holy_uint8_t *) nb;
  return nb;
}

struct holy_net_buff *
holy_netbuff_make_pkt (holy_size_t len)
{
  struct holy_net_buff *nb;
  holy_err_t err;
  nb = holy_netbuff_alloc (len + 512);
  if (!nb)
    return NULL;
  err = holy_netbuff_reserve (nb, len + 512);
  if (err)
    goto fail;
  err = holy_netbuff_push (nb, len);
  if (err)
    goto fail;
  return nb;
 fail:
  holy_netbuff_free (nb);
  return NULL;
}

void
holy_netbuff_free (struct holy_net_buff *nb)
{
  if (!nb)
    return;
  holy_free (nb->head);
}

holy_err_t
holy_netbuff_clear (struct holy_net_buff *nb)
{
  nb->data = nb->tail = nb->head;
  return holy_ERR_NONE;
}
