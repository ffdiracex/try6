/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_ACORN_FILECORE_HEADER
#define holy_ACORN_FILECORE_HEADER	1

#include <holy/types.h>

struct holy_filecore_disc_record
{
  holy_uint8_t log2secsize;
  holy_uint8_t secspertrack;
  holy_uint8_t heads;
  holy_uint8_t density;
  holy_uint8_t idlen;
  holy_uint8_t log2bpmb;
  holy_uint8_t skew;
  holy_uint8_t bootoption;
  /* In bits 0-5, flags in bits 6 and 7.  */
  holy_uint8_t lowsector;
  holy_uint8_t nzones;
  holy_uint16_t zone_spare;
  holy_uint32_t root_address;
  /* Disc size in bytes.  */
  holy_uint32_t disc_size;
  holy_uint16_t cycle_id;
  char disc_name[10];
  /* Yes, it is 32 bits!  */
  holy_uint32_t disctype;
  /* Most significant part of the disc size.  */
  holy_uint32_t disc_size2;
  holy_uint8_t share_size;
  holy_uint8_t big_flag;
  holy_uint8_t reserved[18];
};


#endif /* ! holy_ACORN_FILECORE_HEADER */
