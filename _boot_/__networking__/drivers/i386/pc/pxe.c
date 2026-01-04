/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/net.h>
#include <holy/mm.h>
#include <holy/file.h>
#include <holy/misc.h>
#include <holy/env.h>
#include <holy/i18n.h>
#include <holy/loader.h>

#include <holy/machine/pxe.h>
#include <holy/machine/int.h>
#include <holy/machine/memory.h>
#include <holy/machine/kernel.h>

holy_MOD_LICENSE ("GPLv2+");

#define SEGMENT(x)	((x) >> 4)
#define OFFSET(x)	((x) & 0xF)
#define SEGOFS(x)	((SEGMENT(x) << 16) + OFFSET(x))
#define LINEAR(x)	(void *) ((((x) >> 16) << 4) + ((x) & 0xFFFF))

struct holy_pxe_undi_open
{
  holy_uint16_t status;
  holy_uint16_t open_flag;
  holy_uint16_t pkt_filter;
  holy_uint16_t mcast_count;
  holy_uint8_t mcast[8][6];
} holy_PACKED;

struct holy_pxe_undi_info
{
  holy_uint16_t status;
  holy_uint16_t base_io;
  holy_uint16_t int_number;
  holy_uint16_t mtu;
  holy_uint16_t hwtype;
  holy_uint16_t hwaddrlen;
  holy_uint8_t current_addr[16];
  holy_uint8_t permanent_addr[16];
  holy_uint32_t romaddr;
  holy_uint16_t rxbufct;
  holy_uint16_t txbufct;
} holy_PACKED;


struct holy_pxe_undi_isr
{
  holy_uint16_t status;
  holy_uint16_t func_flag;
  holy_uint16_t buffer_len;
  holy_uint16_t frame_len;
  holy_uint16_t frame_hdr_len;
  holy_uint32_t buffer;
  holy_uint8_t prot_type;
  holy_uint8_t pkt_type;
} holy_PACKED;

enum
  {
    holy_PXE_ISR_IN_START = 1,
    holy_PXE_ISR_IN_PROCESS,
    holy_PXE_ISR_IN_GET_NEXT
  };

enum
  {
    holy_PXE_ISR_OUT_OURS = 0,
    holy_PXE_ISR_OUT_NOT_OURS = 1
  };

enum
  {
    holy_PXE_ISR_OUT_DONE = 0,
    holy_PXE_ISR_OUT_TRANSMIT = 2,
    holy_PXE_ISR_OUT_RECEIVE = 3,
    holy_PXE_ISR_OUT_BUSY = 4,
  };

struct holy_pxe_undi_transmit
{
  holy_uint16_t status;
  holy_uint8_t protocol;
  holy_uint8_t xmitflag;
  holy_uint32_t dest;
  holy_uint32_t tbd;
  holy_uint32_t reserved[2];
} holy_PACKED;

struct holy_pxe_undi_tbd
{
  holy_uint16_t len;
  holy_uint32_t buf;
  holy_uint16_t blk_count;
  struct
  {
    holy_uint8_t ptr_type;
    holy_uint8_t reserved;
    holy_uint16_t len;
    holy_uint32_t ptr;
  } blocks[8];
} holy_PACKED;

struct holy_pxe_bangpxe *holy_pxe_pxenv;
static holy_uint32_t pxe_rm_entry = 0;

static struct holy_pxe_bangpxe *
holy_pxe_scan (void)
{
  struct holy_bios_int_registers regs;
  struct holy_pxenv *pxenv;
  struct holy_pxe_bangpxe *bangpxe;

  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;

  regs.ebx = 0;
  regs.ecx = 0;
  regs.eax = 0x5650;
  regs.es = 0;

  holy_bios_interrupt (0x1a, &regs);

  if ((regs.eax & 0xffff) != 0x564e)
    return NULL;

  pxenv = (struct holy_pxenv *) ((regs.es << 4) + (regs.ebx & 0xffff));
  if (holy_memcmp (pxenv->signature, holy_PXE_SIGNATURE,
		   sizeof (pxenv->signature))
      != 0)
    return NULL;

  if (pxenv->version < 0x201)
    return NULL;

  bangpxe = (void *) ((((pxenv->pxe_ptr & 0xffff0000) >> 16) << 4)
		      + (pxenv->pxe_ptr & 0xffff));

  if (!bangpxe)
    return NULL;

  if (holy_memcmp (bangpxe->signature, holy_PXE_BANGPXE_SIGNATURE,
		   sizeof (bangpxe->signature)) != 0)
    return NULL;

  pxe_rm_entry = bangpxe->rm_entry;

  return bangpxe;
}

static struct holy_net_buff *
holy_pxe_recv (struct holy_net_card *dev __attribute__ ((unused)))
{
  struct holy_pxe_undi_isr *isr;
  static int in_progress = 0;
  holy_uint8_t *ptr, *end;
  struct holy_net_buff *buf;

  isr = (void *) holy_MEMORY_MACHINE_SCRATCH_ADDR;

  if (!in_progress)
    {
      holy_memset (isr, 0, sizeof (*isr));
      isr->func_flag = holy_PXE_ISR_IN_START;
      holy_pxe_call (holy_PXENV_UNDI_ISR, isr, pxe_rm_entry);
      /* Normally according to the specification we should also check
	 that isr->func_flag != holy_PXE_ISR_OUT_OURS but unfortunately it
	 breaks on intel cards.
       */
      if (isr->status)
	{
	  in_progress = 0;
	  return NULL;
	}
      holy_memset (isr, 0, sizeof (*isr));
      isr->func_flag = holy_PXE_ISR_IN_PROCESS;
      holy_pxe_call (holy_PXENV_UNDI_ISR, isr, pxe_rm_entry);
    }
  else
    {
      holy_memset (isr, 0, sizeof (*isr));
      isr->func_flag = holy_PXE_ISR_IN_GET_NEXT;
      holy_pxe_call (holy_PXENV_UNDI_ISR, isr, pxe_rm_entry);
    }

  while (isr->func_flag != holy_PXE_ISR_OUT_RECEIVE)
    {
      if (isr->status || isr->func_flag == holy_PXE_ISR_OUT_DONE)
	{
	  in_progress = 0;
	  return NULL;
	}
      holy_memset (isr, 0, sizeof (*isr));
      isr->func_flag = holy_PXE_ISR_IN_GET_NEXT;
      holy_pxe_call (holy_PXENV_UNDI_ISR, isr, pxe_rm_entry);
    }

  buf = holy_netbuff_alloc (isr->frame_len + 2);
  if (!buf)
    return NULL;
  /* Reserve 2 bytes so that 2 + 14/18 bytes of ethernet header is divisible
     by 4. So that IP header is aligned on 4 bytes. */
  if (holy_netbuff_reserve (buf, 2))
    {
      holy_netbuff_free (buf);
      return NULL;
    }
  ptr = buf->data;
  end = ptr + isr->frame_len;
  holy_netbuff_put (buf, isr->frame_len);
  holy_memcpy (ptr, LINEAR (isr->buffer), isr->buffer_len);
  ptr += isr->buffer_len;
  while (ptr < end)
    {
      holy_memset (isr, 0, sizeof (*isr));
      isr->func_flag = holy_PXE_ISR_IN_GET_NEXT;
      holy_pxe_call (holy_PXENV_UNDI_ISR, isr, pxe_rm_entry);
      if (isr->status || isr->func_flag != holy_PXE_ISR_OUT_RECEIVE)
	{
	  in_progress = 1;
	  holy_netbuff_free (buf);
	  return NULL;
	}

      holy_memcpy (ptr, LINEAR (isr->buffer), isr->buffer_len);
      ptr += isr->buffer_len;
    }
  in_progress = 1;

  return buf;
}

static holy_err_t
holy_pxe_send (struct holy_net_card *dev __attribute__ ((unused)),
	       struct holy_net_buff *pack)
{
  struct holy_pxe_undi_transmit *trans;
  struct holy_pxe_undi_tbd *tbd;
  char *buf;

  trans = (void *) holy_MEMORY_MACHINE_SCRATCH_ADDR;
  holy_memset (trans, 0, sizeof (*trans));
  tbd = (void *) (holy_MEMORY_MACHINE_SCRATCH_ADDR + 128);
  holy_memset (tbd, 0, sizeof (*tbd));
  buf = (void *) (holy_MEMORY_MACHINE_SCRATCH_ADDR + 256);
  holy_memcpy (buf, pack->data, pack->tail - pack->data);

  trans->tbd = SEGOFS ((holy_addr_t) tbd);
  trans->protocol = 0;
  tbd->len = pack->tail - pack->data;
  tbd->buf = SEGOFS ((holy_addr_t) buf);

  holy_pxe_call (holy_PXENV_UNDI_TRANSMIT, trans, pxe_rm_entry);
  if (trans->status)
    return holy_error (holy_ERR_IO, N_("couldn't send network packet"));
  return 0;
}

static void
holy_pxe_close (struct holy_net_card *dev __attribute__ ((unused)))
{
  if (pxe_rm_entry)
    holy_pxe_call (holy_PXENV_UNDI_CLOSE,
		   (void *) holy_MEMORY_MACHINE_SCRATCH_ADDR,
		   pxe_rm_entry);
}

static holy_err_t
holy_pxe_open (struct holy_net_card *dev __attribute__ ((unused)))
{
  struct holy_pxe_undi_open *ou;
  ou = (void *) holy_MEMORY_MACHINE_SCRATCH_ADDR;
  holy_memset (ou, 0, sizeof (*ou));
  ou->pkt_filter = 4;
  holy_pxe_call (holy_PXENV_UNDI_OPEN, ou, pxe_rm_entry);

  if (ou->status)
    return holy_error (holy_ERR_IO, "can't open UNDI");
  return holy_ERR_NONE;
} 

struct holy_net_card_driver holy_pxe_card_driver =
{
  .open = holy_pxe_open,
  .close = holy_pxe_close,
  .send = holy_pxe_send,
  .recv = holy_pxe_recv
};

struct holy_net_card holy_pxe_card =
{
  .driver = &holy_pxe_card_driver,
  .name = "pxe"
};

static holy_err_t
holy_pxe_shutdown (int flags)
{
  if (flags & holy_LOADER_FLAG_PXE_NOT_UNLOAD)
    return holy_ERR_NONE;
  if (!pxe_rm_entry)
    return holy_ERR_NONE;

  holy_pxe_call (holy_PXENV_UNDI_CLOSE,
		 (void *) holy_MEMORY_MACHINE_SCRATCH_ADDR,
		 pxe_rm_entry);
  holy_pxe_call (holy_PXENV_UNDI_SHUTDOWN,
		 (void *) holy_MEMORY_MACHINE_SCRATCH_ADDR,
		 pxe_rm_entry);
  holy_pxe_call (holy_PXENV_UNLOAD_STACK,
		 (void *) holy_MEMORY_MACHINE_SCRATCH_ADDR,
		 pxe_rm_entry);
  pxe_rm_entry = 0;
  holy_net_card_unregister (&holy_pxe_card);

  return holy_ERR_NONE;
}

/* Nothing we can do.  */
static holy_err_t
holy_pxe_restore (void)
{
  return holy_ERR_NONE;
}

void *
holy_pxe_get_cached (holy_uint16_t type)
{
  struct holy_pxenv_get_cached_info ci;
  ci.packet_type = type;
  ci.buffer = 0;
  ci.buffer_size = 0;
  holy_pxe_call (holy_PXENV_GET_CACHED_INFO, &ci, pxe_rm_entry);
  if (ci.status)
    return 0;

  return LINEAR (ci.buffer);
}

static void
holy_pc_net_config_real (char **device, char **path)
{
  struct holy_net_bootp_packet *bp;

  bp = holy_pxe_get_cached (holy_PXENV_PACKET_TYPE_DHCP_ACK);

  if (!bp)
    return;
  holy_net_configure_by_dhcp_ack ("pxe", &holy_pxe_card, 0,
				  bp, holy_PXE_BOOTP_SIZE,
				  1, device, path);

}

static struct holy_preboot *fini_hnd;

holy_MOD_INIT(pxe)
{
  struct holy_pxe_bangpxe *pxenv;
  struct holy_pxe_undi_info *ui;
  unsigned i;

  pxenv = holy_pxe_scan ();
  if (! pxenv)
    return;

  ui = (void *) holy_MEMORY_MACHINE_SCRATCH_ADDR;
  holy_memset (ui, 0, sizeof (*ui));
  holy_pxe_call (holy_PXENV_UNDI_GET_INFORMATION, ui, pxe_rm_entry);

  holy_memcpy (holy_pxe_card.default_address.mac, ui->current_addr,
	       sizeof (holy_pxe_card.default_address.mac));
  for (i = 0; i < sizeof (holy_pxe_card.default_address.mac); i++)
    if (holy_pxe_card.default_address.mac[i] != 0)
      break;
  if (i != sizeof (holy_pxe_card.default_address.mac))
    {
      for (i = 0; i < sizeof (holy_pxe_card.default_address.mac); i++)
	if (holy_pxe_card.default_address.mac[i] != 0xff)
	  break;
    }
  if (i == sizeof (holy_pxe_card.default_address.mac))
    holy_memcpy (holy_pxe_card.default_address.mac, ui->permanent_addr,
		 sizeof (holy_pxe_card.default_address.mac));
  holy_pxe_card.mtu = ui->mtu;

  holy_pxe_card.default_address.type = holy_NET_LINK_LEVEL_PROTOCOL_ETHERNET;

  holy_net_card_register (&holy_pxe_card);
  holy_pc_net_config = holy_pc_net_config_real;
  fini_hnd = holy_loader_register_preboot_hook (holy_pxe_shutdown,
						holy_pxe_restore,
						holy_LOADER_PREBOOT_HOOK_PRIO_DISK);
}

holy_MOD_FINI(pxe)
{
  holy_pc_net_config = 0;
  holy_pxe_shutdown (0);
  holy_loader_unregister_preboot_hook (fini_hnd);
}
