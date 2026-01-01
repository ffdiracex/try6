/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config-util.h>

#include <holy/disk.h>
#include <holy/partition.h>
#include <holy/msdos_partition.h>
#include <holy/types.h>
#include <holy/err.h>
#include <holy/emu/misc.h>
#include <holy/emu/hostdisk.h>
#include <holy/emu/getroot.h>
#include <holy/misc.h>
#include <holy/i18n.h>
#include <holy/list.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#include <hurd.h>
#include <hurd/lookup.h>
#include <hurd/fs.h>
#include <sys/mman.h>

int
holy_util_hurd_get_disk_info (const char *dev, holy_uint32_t *secsize, holy_disk_addr_t *offset,
			      holy_disk_addr_t *size, char **parent)
{
  file_t file;
  mach_port_t *ports;
  int *ints;
  loff_t *offsets;
  char *data;
  error_t err;
  mach_msg_type_number_t num_ports = 0, num_ints = 0, num_offsets = 0, data_len = 0;

  file = file_name_lookup (dev, 0, 0);
  if (file == MACH_PORT_NULL)
    return 0;

  err = file_get_storage_info (file,
			       &ports, &num_ports,
			       &ints, &num_ints,
			       &offsets, &num_offsets,
			       &data, &data_len);

  if (num_ints < 1)
    holy_util_error (_("Storage information for `%s' does not include type"), dev);
  if (ints[0] != STORAGE_DEVICE)
    holy_util_error (_("`%s' is not a local disk"), dev);

  if (num_offsets != 2)
    holy_util_error (_("Storage information for `%s' indicates neither a plain partition nor a plain disk"), dev);
  if (parent)
    {
      *parent = NULL;
      if (num_ints >= 5)
	{
	  size_t len = ints[4];
	  if (len > data_len)
	    len = data_len;
	  *parent = xmalloc (len+1);
	  memcpy (*parent, data, len);
	  (*parent)[len] = '\0';
	}
    }
  if (offset)
    *offset = offsets[0];
  if (size)
    *size = offsets[1];
  if (secsize)
    *secsize = ints[2];
  if (ports && num_ports > 0)
    {
      mach_msg_type_number_t i;
      for (i = 0; i < num_ports; i++)
        {
	  mach_port_t port = ports[i];
	  if (port != MACH_PORT_NULL)
	    mach_port_deallocate (mach_task_self(), port);
        }
      munmap ((caddr_t) ports, num_ports * sizeof (*ports));
    }

  if (ints && num_ints > 0)
    munmap ((caddr_t) ints, num_ints * sizeof (*ints));
  if (offsets && num_offsets > 0)
    munmap ((caddr_t) offsets, num_offsets * sizeof (*offsets));
  if (data && data_len > 0)
    munmap (data, data_len);
  mach_port_deallocate (mach_task_self (), file);

  return 1;
}

holy_int64_t
holy_util_get_fd_size_os (holy_util_fd_t fd, const char *name, unsigned *log_secsize)
{
  holy_uint32_t sector_size;
  holy_disk_addr_t size;
  unsigned log_sector_size;

  if (!holy_util_hurd_get_disk_info (name, &sector_size, NULL, &size, NULL))
    return -1;

  if (sector_size & (sector_size - 1) || !sector_size)
    return -1;
  for (log_sector_size = 0;
       (1 << log_sector_size) < sector_size;
       log_sector_size++);

  if (log_secsize)
    *log_secsize = log_sector_size;

  return size << log_sector_size;
}

void
holy_hostdisk_flush_initial_buffer (const char *os_dev __attribute__ ((unused)))
{
}
