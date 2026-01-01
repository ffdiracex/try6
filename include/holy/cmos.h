/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	holy_CMOS_H
#define	holy_CMOS_H	1

#include <holy/types.h>
#if !defined (__powerpc__) && !defined (__sparc__)
#include <holy/cpu/io.h>
#include <holy/cpu/cmos.h>
#endif

#define holy_CMOS_INDEX_SECOND		0
#define holy_CMOS_INDEX_SECOND_ALARM	1
#define holy_CMOS_INDEX_MINUTE		2
#define holy_CMOS_INDEX_MINUTE_ALARM	3
#define holy_CMOS_INDEX_HOUR		4
#define holy_CMOS_INDEX_HOUR_ALARM	5
#define holy_CMOS_INDEX_DAY_OF_WEEK	6
#define holy_CMOS_INDEX_DAY_OF_MONTH	7
#define holy_CMOS_INDEX_MONTH		8
#define holy_CMOS_INDEX_YEAR		9

#define holy_CMOS_INDEX_STATUS_A	0xA
#define holy_CMOS_INDEX_STATUS_B	0xB
#define holy_CMOS_INDEX_STATUS_C	0xC
#define holy_CMOS_INDEX_STATUS_D	0xD

#define holy_CMOS_STATUS_B_DAYLIGHT	1
#define holy_CMOS_STATUS_B_24HOUR	2
#define holy_CMOS_STATUS_B_BINARY	4

static inline holy_uint8_t
holy_bcd_to_num (holy_uint8_t a)
{
  return ((a >> 4) * 10 + (a & 0xF));
}

static inline holy_uint8_t
holy_num_to_bcd (holy_uint8_t a)
{
  return (((a / 10) << 4) + (a % 10));
}

#if !defined (__powerpc__) && !defined (__sparc__)
static inline holy_err_t
holy_cmos_read (holy_uint8_t index, holy_uint8_t *val)
{
  if (index & 0x80)
    {
      holy_outb (index & 0x7f, holy_CMOS_ADDR_REG_HI);
      *val = holy_inb (holy_CMOS_DATA_REG_HI);
    }
  else
    {
      holy_outb (index & 0x7f, holy_CMOS_ADDR_REG);
      *val = holy_inb (holy_CMOS_DATA_REG);
    }
  return holy_ERR_NONE;
}

static inline holy_err_t
holy_cmos_write (holy_uint8_t index, holy_uint8_t value)
{
  if (index & 0x80)
    {
      holy_outb (index & 0x7f, holy_CMOS_ADDR_REG_HI);
      holy_outb (value, holy_CMOS_DATA_REG_HI);
    }
  else
    {
      holy_outb (index & 0x7f, holy_CMOS_ADDR_REG);
      holy_outb (value, holy_CMOS_DATA_REG);
    }
  return holy_ERR_NONE;
}
#else
holy_err_t holy_cmos_find_port (void);
extern volatile holy_uint8_t *holy_cmos_port;

static inline holy_err_t
holy_cmos_read (holy_uint8_t index, holy_uint8_t *val)
{
  if (!holy_cmos_port)
    {
      holy_err_t err;
      err = holy_cmos_find_port ();
      if (err)
	return err;
    }
  holy_cmos_port[((index & 0x80) >> 6) | 0] = index & 0x7f;
  *val = holy_cmos_port[((index & 0x80) >> 6) | 1];
  return holy_ERR_NONE;
}

static inline holy_err_t
holy_cmos_write (holy_uint8_t index, holy_uint8_t val)
{
  if (!holy_cmos_port)
    {
      holy_err_t err;
      err = holy_cmos_find_port ();
      if (err)
	return err;
    }
  holy_cmos_port[((index & 0x80) >> 6) | 0] = index;
  holy_cmos_port[((index & 0x80) >> 6) | 1] = val;
  return holy_ERR_NONE;
}

#endif

#endif /* holy_CMOS_H */
