/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_SMBUS_HEADER
#define holy_SMBUS_HEADER 1

#define holy_SMB_RAM_START_ADDR 0x50
#define holy_SMB_RAM_NUM_MAX 0x08

#define holy_SMBUS_SPD_MEMORY_TYPE_ADDR 2
#define holy_SMBUS_SPD_MEMORY_TYPE_DDR2 8
#define holy_SMBUS_SPD_MEMORY_NUM_BANKS_ADDR 17
#define holy_SMBUS_SPD_MEMORY_NUM_ROWS_ADDR 3
#define holy_SMBUS_SPD_MEMORY_NUM_COLUMNS_ADDR 4
#define holy_SMBUS_SPD_MEMORY_NUM_OF_RANKS_ADDR 5
#define holy_SMBUS_SPD_MEMORY_NUM_OF_RANKS_MASK 0x7
#define holy_SMBUS_SPD_MEMORY_CAS_LATENCY_ADDR 18
#define holy_SMBUS_SPD_MEMORY_CAS_LATENCY_MIN_VALUE 5
#define holy_SMBUS_SPD_MEMORY_TRAS_ADDR 30
#define holy_SMBUS_SPD_MEMORY_TRTP_ADDR 38

#ifndef ASM_FILE

struct holy_smbus_spd
{
  holy_uint8_t written_size;
  holy_uint8_t log_total_flash_size;
  holy_uint8_t memory_type;
  union
  {
    holy_uint8_t unknown[253];
    struct {
      holy_uint8_t num_rows;
      holy_uint8_t num_columns;
      holy_uint8_t num_of_ranks;
      holy_uint8_t unused1[12];
      holy_uint8_t num_of_banks;
      holy_uint8_t unused2[2];
      holy_uint8_t cas_latency;
      holy_uint8_t unused3[9];
      holy_uint8_t rank_capacity;
      holy_uint8_t unused4[1];
      holy_uint8_t tras;
      holy_uint8_t unused5[7];
      holy_uint8_t trtp;
      holy_uint8_t unused6[31];
      holy_uint8_t part_number[18];
      holy_uint8_t unused7[165];
    } ddr2;
  };
};

#endif

#endif
