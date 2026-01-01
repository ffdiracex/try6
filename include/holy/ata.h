/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_ATA_HEADER
#define holy_ATA_HEADER 1

#include <holy/misc.h>
#include <holy/symbol.h>
#include <holy/disk.h>
/* XXX: For now this only works on i386.  */
#include <holy/cpu/io.h>

typedef enum
  {
    holy_ATA_CHS,
    holy_ATA_LBA,
    holy_ATA_LBA48
  } holy_ata_addressing_t;

#define holy_ATA_CH0_PORT1 0x1f0
#define holy_ATA_CH1_PORT1 0x170

#define holy_ATA_CH0_PORT2 0x3f6
#define holy_ATA_CH1_PORT2 0x376

#define holy_ATA_REG_DATA	0
#define holy_ATA_REG_ERROR	1
#define holy_ATA_REG_FEATURES	1
#define holy_ATA_REG_SECTORS	2
#define holy_ATAPI_REG_IREASON	2
#define holy_ATA_REG_SECTNUM	3
#define holy_ATA_REG_CYLLSB	4
#define holy_ATA_REG_CYLMSB	5
#define holy_ATA_REG_LBALOW	3
#define holy_ATA_REG_LBAMID	4
#define holy_ATAPI_REG_CNTLOW	4
#define holy_ATA_REG_LBAHIGH	5
#define holy_ATAPI_REG_CNTHIGH	5
#define holy_ATA_REG_DISK	6
#define holy_ATA_REG_CMD	7
#define holy_ATA_REG_STATUS	7

#define holy_ATA_REG2_CONTROL	0

#define holy_ATA_STATUS_ERR	0x01
#define holy_ATA_STATUS_INDEX	0x02
#define holy_ATA_STATUS_ECC	0x04
#define holy_ATA_STATUS_DRQ	0x08
#define holy_ATA_STATUS_SEEK	0x10
#define holy_ATA_STATUS_WRERR	0x20
#define holy_ATA_STATUS_READY	0x40
#define holy_ATA_STATUS_BUSY	0x80

/* ATAPI interrupt reason values (I/O, D/C bits).  */
#define holy_ATAPI_IREASON_MASK     0x3
#define holy_ATAPI_IREASON_DATA_OUT 0x0
#define holy_ATAPI_IREASON_CMD_OUT  0x1
#define holy_ATAPI_IREASON_DATA_IN  0x2
#define holy_ATAPI_IREASON_ERROR    0x3

enum holy_ata_commands
  {
    holy_ATA_CMD_CHECK_POWER_MODE	= 0xe5,
    holy_ATA_CMD_IDENTIFY_DEVICE	= 0xec,
    holy_ATA_CMD_IDENTIFY_PACKET_DEVICE	= 0xa1,
    holy_ATA_CMD_IDLE			= 0xe3,
    holy_ATA_CMD_PACKET			= 0xa0,
    holy_ATA_CMD_READ_SECTORS		= 0x20,
    holy_ATA_CMD_READ_SECTORS_EXT	= 0x24,
    holy_ATA_CMD_READ_SECTORS_DMA	= 0xc8,
    holy_ATA_CMD_READ_SECTORS_DMA_EXT	= 0x25,

    holy_ATA_CMD_SECURITY_FREEZE_LOCK	= 0xf5,
    holy_ATA_CMD_SET_FEATURES		= 0xef,
    holy_ATA_CMD_SLEEP			= 0xe6,
    holy_ATA_CMD_SMART			= 0xb0,
    holy_ATA_CMD_STANDBY_IMMEDIATE	= 0xe0,
    holy_ATA_CMD_WRITE_SECTORS		= 0x30,
    holy_ATA_CMD_WRITE_SECTORS_EXT	= 0x34,
    holy_ATA_CMD_WRITE_SECTORS_DMA_EXT	= 0x35,
    holy_ATA_CMD_WRITE_SECTORS_DMA	= 0xca,
  };

enum holy_ata_timeout_milliseconds
  {
    holy_ATA_TOUT_STD  =  1000,  /* 1s standard timeout.  */
    holy_ATA_TOUT_DATA = 10000,   /* 10s DATA I/O timeout.  */
    holy_ATA_TOUT_SPINUP  =  10000,  /* Give the device 10s on first try to spinon.  */
  };

typedef union
{
  holy_uint8_t raw[11];
  struct
  {
    union
    {
      holy_uint8_t features;
      holy_uint8_t error;
    };
    union
    {
      holy_uint8_t sectors;
      holy_uint8_t atapi_ireason;
    };
    union
    {
      holy_uint8_t lba_low;
      holy_uint8_t sectnum;
    };
    union
    {
      holy_uint8_t lba_mid;
      holy_uint8_t cyllsb;
      holy_uint8_t atapi_cntlow;
    };
    union
    {
      holy_uint8_t lba_high;
      holy_uint8_t cylmsb;
      holy_uint8_t atapi_cnthigh;
    };
    holy_uint8_t disk;
    union
    {
      holy_uint8_t cmd;
      holy_uint8_t status;
    };
    holy_uint8_t sectors48;
    holy_uint8_t lba48_low;
    holy_uint8_t lba48_mid;
    holy_uint8_t lba48_high;
  };
} holy_ata_regs_t;

/* ATA pass through parameters and function.  */
struct holy_disk_ata_pass_through_parms
{
  holy_ata_regs_t taskfile;
  void * buffer;
  holy_size_t size;
  int write;
  void *cmd;
  int cmdsize;
  int dma;
};

struct holy_ata
{
  /* Addressing methods available for accessing this device.  If CHS
     is only available, use that.  Otherwise use LBA, except for the
     high sectors.  In that case use LBA48.  */
  holy_ata_addressing_t addr;

  /* Sector count.  */
  holy_uint64_t size;
  holy_uint32_t log_sector_size;

  /* CHS maximums.  */
  holy_uint16_t cylinders;
  holy_uint16_t heads;
  holy_uint16_t sectors_per_track;

  /* Set to 0 for ATA, set to 1 for ATAPI.  */
  int atapi;

  int dma;

  holy_size_t maxbuffer;

  int *present;

  void *data;

  struct holy_ata_dev *dev;
};

typedef struct holy_ata *holy_ata_t;

typedef int (*holy_ata_dev_iterate_hook_t) (int id, int bus, void *data);

struct holy_ata_dev
{
  /* Call HOOK with each device name, until HOOK returns non-zero.  */
  int (*iterate) (holy_ata_dev_iterate_hook_t hook, void *hook_data,
		  holy_disk_pull_t pull);

  /* Open the device named NAME, and set up SCSI.  */
  holy_err_t (*open) (int id, int bus, struct holy_ata *scsi);

  /* Close the scsi device SCSI.  */
  void (*close) (struct holy_ata *ata);

  /* Read SIZE bytes from the device SCSI into BUF after sending the
     command CMD of size CMDSIZE.  */
  holy_err_t (*readwrite) (struct holy_ata *ata,
			   struct holy_disk_ata_pass_through_parms *parms,
			   int spinup);

  /* The next scsi device.  */
  struct holy_ata_dev *next;
};

typedef struct holy_ata_dev *holy_ata_dev_t;

void holy_ata_dev_register (holy_ata_dev_t dev);
void holy_ata_dev_unregister (holy_ata_dev_t dev);

#endif /* ! holy_ATA_HEADER */
