/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/relocator.h>
#include <holy/relocator_private.h>
#include <holy/memory.h>
#include <holy/ieee1275/ieee1275.h>

/* Helper for holy_relocator_firmware_get_max_events.  */
static int
count (holy_uint64_t addr __attribute__ ((unused)),
       holy_uint64_t len __attribute__ ((unused)),
       holy_memory_type_t type __attribute__ ((unused)), void *data)
{
  int *counter = data;

  (*counter)++;
  return 0;
}

unsigned 
holy_relocator_firmware_get_max_events (void)
{
  int counter = 0;

  if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_FORCE_CLAIM))
    return 0;
  holy_machine_mmap_iterate (count, &counter);
  return 2 * counter;
}

/* Context for holy_relocator_firmware_fill_events.  */
struct holy_relocator_firmware_fill_events_ctx
{
  struct holy_relocator_mmap_event *events;
  int counter;
};

/* Helper for holy_relocator_firmware_fill_events.  */
static int
holy_relocator_firmware_fill_events_iter (holy_uint64_t addr,
					  holy_uint64_t len,
					  holy_memory_type_t type, void *data)
{
  struct holy_relocator_firmware_fill_events_ctx *ctx = data;

  if (type != holy_MEMORY_AVAILABLE)
    return 0;

  if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_NO_PRE1_5M_CLAIM))
    {
      if (addr + len <= 0x180000)
	return 0;

      if (addr < 0x180000)
	{
	  len = addr + len - 0x180000;
	  addr = 0x180000;
	}
    }

  ctx->events[ctx->counter].type = REG_FIRMWARE_START;
  ctx->events[ctx->counter].pos = addr;
  ctx->counter++;
  ctx->events[ctx->counter].type = REG_FIRMWARE_END;
  ctx->events[ctx->counter].pos = addr + len;
  ctx->counter++;

  return 0;
}

unsigned 
holy_relocator_firmware_fill_events (struct holy_relocator_mmap_event *events)
{
  struct holy_relocator_firmware_fill_events_ctx ctx = {
    .events = events,
    .counter = 0
  };

  if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_FORCE_CLAIM))
    return 0;
  holy_machine_mmap_iterate (holy_relocator_firmware_fill_events_iter, &ctx);
  return ctx.counter;
}

int
holy_relocator_firmware_alloc_region (holy_addr_t start, holy_size_t size)
{
  holy_err_t err;
  err = holy_claimmap (start, size);
  holy_errno = 0;
  return (err == 0);
}

void
holy_relocator_firmware_free_region (holy_addr_t start, holy_size_t size)
{
  holy_ieee1275_release (start, size);
}
