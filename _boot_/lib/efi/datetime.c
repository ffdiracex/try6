/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/symbol.h>
#include <holy/efi/api.h>
#include <holy/efi/efi.h>
#include <holy/datetime.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

holy_err_t
holy_get_datetime (struct holy_datetime *datetime)
{
  holy_efi_status_t status;
  struct holy_efi_time efi_time;

  status = efi_call_2 (holy_efi_system_table->runtime_services->get_time,
                       &efi_time, 0);

  if (status)
    return holy_error (holy_ERR_INVALID_COMMAND,
                       "can\'t get datetime using efi");
  else
    {
      datetime->year = efi_time.year;
      datetime->month = efi_time.month;
      datetime->day = efi_time.day;
      datetime->hour = efi_time.hour;
      datetime->minute = efi_time.minute;
      datetime->second = efi_time.second;
    }

  return 0;
}

holy_err_t
holy_set_datetime (struct holy_datetime *datetime)
{
  holy_efi_status_t status;
  struct holy_efi_time efi_time;

  status = efi_call_2 (holy_efi_system_table->runtime_services->get_time,
                       &efi_time, 0);

  if (status)
    return holy_error (holy_ERR_INVALID_COMMAND,
                       "can\'t get datetime using efi");

  efi_time.year = datetime->year;
  efi_time.month = datetime->month;
  efi_time.day = datetime->day;
  efi_time.hour = datetime->hour;
  efi_time.minute = datetime->minute;
  efi_time.second = datetime->second;

  status = efi_call_1 (holy_efi_system_table->runtime_services->set_time,
                       &efi_time);

  if (status)
    return holy_error (holy_ERR_INVALID_COMMAND,
                       "can\'t set datetime using efi");

  return 0;
}
