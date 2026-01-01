/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <config-util.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>

#include <holy/emu/net.h>

static int fd;

holy_ssize_t
holy_emunet_send (const void *packet, holy_size_t sz)
{
  return write (fd, packet, sz);
}

holy_ssize_t
holy_emunet_receive (void *packet, holy_size_t sz)
{
  return read (fd, packet, sz);
}

int
holy_emunet_create (holy_size_t *mtu)
{
  struct ifreq ifr;
  *mtu = 1500;
  fd = open ("/dev/net/tun", O_RDWR | O_NONBLOCK);
  if (fd < 0)
    return -1;
  memset (&ifr, 0, sizeof (ifr));
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
  if (ioctl (fd, TUNSETIFF, &ifr) < 0)
    {
      close (fd);
      fd = -1;
      return -1;
    }
  return 0;
}

void
holy_emunet_close (void)
{
  if (fd < 0)
    return;

  close (fd);
  fd = -1;
}
