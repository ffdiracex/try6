/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_NETBUFF_HEADER
#define holy_NETBUFF_HEADER

#include <holy/misc.h>

#define NETBUFF_ALIGN 2048
#define NETBUFFMINLEN 64

struct holy_net_buff
{
  /* Pointer to the start of the buffer.  */
  holy_uint8_t *head;
  /* Pointer to the data.  */
  holy_uint8_t *data;
  /* Pointer to the tail.  */
  holy_uint8_t *tail;
  /* Pointer to the end of the buffer.  */
  holy_uint8_t *end;
};

holy_err_t holy_netbuff_put (struct holy_net_buff *net_buff, holy_size_t len);
holy_err_t holy_netbuff_unput (struct holy_net_buff *net_buff, holy_size_t len);
holy_err_t holy_netbuff_push (struct holy_net_buff *net_buff, holy_size_t len);
holy_err_t holy_netbuff_pull (struct holy_net_buff *net_buff, holy_size_t len);
holy_err_t holy_netbuff_reserve (struct holy_net_buff *net_buff, holy_size_t len);
holy_err_t holy_netbuff_clear (struct holy_net_buff *net_buff);
struct holy_net_buff * holy_netbuff_alloc (holy_size_t len);
struct holy_net_buff * holy_netbuff_make_pkt (holy_size_t len);
void holy_netbuff_free (struct holy_net_buff *net_buff);

#endif
