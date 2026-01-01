/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	holy_IO_H
#define	holy_IO_H	1

#include <holy/types.h>

typedef holy_addr_t holy_port_t;

static __inline unsigned char
holy_inb (holy_port_t port)
{
  return *(volatile holy_uint8_t *) port;
}

static __inline unsigned short int
holy_inw (holy_port_t port)
{
  return *(volatile holy_uint16_t *) port;
}

static __inline unsigned int
holy_inl (holy_port_t port)
{
  return *(volatile holy_uint32_t *) port;
}

static __inline void
holy_outb (unsigned char value, holy_port_t port)
{
  *(volatile holy_uint8_t *) port = value;
}

static __inline void
holy_outw (unsigned short int value, holy_port_t port)
{
  *(volatile holy_uint16_t *) port = value;
}

static __inline void
holy_outl (unsigned int value, holy_port_t port)
{
  *(volatile holy_uint32_t *) port = value;
}

#endif /* _SYS_IO_H */
