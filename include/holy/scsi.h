/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	holy_SCSI_H
#define	holy_SCSI_H	1

#include <holy/disk.h>

typedef struct holy_scsi_dev *holy_scsi_dev_t;

void holy_scsi_dev_register (holy_scsi_dev_t dev);
void holy_scsi_dev_unregister (holy_scsi_dev_t dev);

struct holy_scsi;

enum
  {
    holy_SCSI_SUBSYSTEM_USBMS,
    holy_SCSI_SUBSYSTEM_PATA,
    holy_SCSI_SUBSYSTEM_AHCI,
    holy_SCSI_NUM_SUBSYSTEMS
  };

extern const char holy_scsi_names[holy_SCSI_NUM_SUBSYSTEMS][5];

#define holy_SCSI_ID_SUBSYSTEM_SHIFT 24
#define holy_SCSI_ID_BUS_SHIFT 8
#define holy_SCSI_ID_LUN_SHIFT 0

static inline holy_uint32_t
holy_make_scsi_id (int subsystem, int bus, int lun)
{
  return (subsystem << holy_SCSI_ID_SUBSYSTEM_SHIFT)
    | (bus << holy_SCSI_ID_BUS_SHIFT) | (lun << holy_SCSI_ID_LUN_SHIFT);
}

typedef int (*holy_scsi_dev_iterate_hook_t) (int id, int bus, int luns,
					     void *data);

struct holy_scsi_dev
{
  /* Call HOOK with each device name, until HOOK returns non-zero.  */
  int (*iterate) (holy_scsi_dev_iterate_hook_t hook, void *hook_data,
		  holy_disk_pull_t pull);

  /* Open the device named NAME, and set up SCSI.  */
  holy_err_t (*open) (int id, int bus, struct holy_scsi *scsi);

  /* Close the scsi device SCSI.  */
  void (*close) (struct holy_scsi *scsi);

  /* Read SIZE bytes from the device SCSI into BUF after sending the
     command CMD of size CMDSIZE.  */
  holy_err_t (*read) (struct holy_scsi *scsi, holy_size_t cmdsize, char *cmd,
		      holy_size_t size, char *buf);

  /* Write SIZE  bytes from BUF to  the device SCSI  after sending the
     command CMD of size CMDSIZE.  */
  holy_err_t (*write) (struct holy_scsi *scsi, holy_size_t cmdsize, char *cmd,
		       holy_size_t size, const char *buf);

  /* The next scsi device.  */
  struct holy_scsi_dev *next;
};

struct holy_scsi
{
  /* The underlying scsi device.  */
  holy_scsi_dev_t dev;

  /* Type of SCSI device.  XXX: Make enum.  */
  holy_uint8_t devtype;

  int bus;

  /* Number of LUNs.  */
  int luns;

  /* LUN for this `struct holy_scsi'.  */
  int lun;

  /* Set to 0 when not removable, 1 when removable.  */
  int removable;

  /* Size of the device in blocks - 1.  */
  holy_uint64_t last_block;

  /* Size of one block.  */
  holy_uint32_t blocksize;

  /* Device-specific data.  */
  void *data;
};
typedef struct holy_scsi *holy_scsi_t;

#endif /* holy_SCSI_H */
