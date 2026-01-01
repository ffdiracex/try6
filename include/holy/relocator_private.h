/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_RELOCATOR_PRIVATE_HEADER
#define holy_RELOCATOR_PRIVATE_HEADER	1

#include <holy/types.h>
#include <holy/err.h>
#include <holy/mm_private.h>

extern holy_size_t holy_relocator_align;
extern holy_size_t holy_relocator_forward_size;
extern holy_size_t holy_relocator_backward_size;
extern holy_size_t holy_relocator_jumper_size;

void
holy_cpu_relocator_init (void);
holy_err_t
holy_relocator_prepare_relocs (struct holy_relocator *rel,
			       holy_addr_t addr,
			       void **relstart, holy_size_t *relsize);
void holy_cpu_relocator_forward (void *rels, void *src, void *tgt,
				 holy_size_t size);
void holy_cpu_relocator_backward (void *rels, void *src, void *tgt,
				 holy_size_t size);
void holy_cpu_relocator_jumper (void *rels, holy_addr_t addr);

/* Remark: holy_RELOCATOR_FIRMWARE_REQUESTS_QUANT_LOG = 1 or 2
   aren't supported.  */
#ifdef holy_MACHINE_IEEE1275
#define holy_RELOCATOR_HAVE_FIRMWARE_REQUESTS 1
#define holy_RELOCATOR_FIRMWARE_REQUESTS_QUANT_LOG 0
#elif defined (holy_MACHINE_EFI)
#define holy_RELOCATOR_HAVE_FIRMWARE_REQUESTS 1
#define holy_RELOCATOR_FIRMWARE_REQUESTS_QUANT_LOG 12
#else
#define holy_RELOCATOR_HAVE_FIRMWARE_REQUESTS 0
#endif

#if holy_RELOCATOR_HAVE_FIRMWARE_REQUESTS && holy_RELOCATOR_FIRMWARE_REQUESTS_QUANT_LOG != 0
#define holy_RELOCATOR_HAVE_LEFTOVERS 1
#else
#define holy_RELOCATOR_HAVE_LEFTOVERS 0
#endif

#if holy_RELOCATOR_HAVE_FIRMWARE_REQUESTS
#define holy_RELOCATOR_FIRMWARE_REQUESTS_QUANT (1 << holy_RELOCATOR_FIRMWARE_REQUESTS_QUANT_LOG)
#endif

struct holy_relocator_mmap_event
{
  enum {
    IN_REG_START = 0, 
    IN_REG_END = 1, 
    REG_BEG_START = 2, 
    REG_BEG_END = REG_BEG_START | 1,
#if holy_RELOCATOR_HAVE_FIRMWARE_REQUESTS
    REG_FIRMWARE_START = 4, 
    REG_FIRMWARE_END = REG_FIRMWARE_START | 1,
    /* To track the regions already in heap.  */
    FIRMWARE_BLOCK_START = 6, 
    FIRMWARE_BLOCK_END = FIRMWARE_BLOCK_START | 1,
#endif
#if holy_RELOCATOR_HAVE_LEFTOVERS
    REG_LEFTOVER_START = 8, 
    REG_LEFTOVER_END = REG_LEFTOVER_START | 1,
#endif
    COLLISION_START = 10,
    COLLISION_END = COLLISION_START | 1
  } type;
  holy_phys_addr_t pos;
  union
  {
    struct
    {
      holy_mm_region_t reg;
      holy_mm_header_t hancestor;
      holy_mm_region_t *regancestor;
      holy_mm_header_t head;
    };
#if holy_RELOCATOR_HAVE_FIRMWARE_REQUESTS
    struct holy_relocator_fw_leftover *leftover;
#endif
  };
};

/* Return 0 on failure, 1 on success. The failure here 
   can be very time-expensive, so please make sure fill events is accurate.  */
#if holy_RELOCATOR_HAVE_FIRMWARE_REQUESTS
int holy_relocator_firmware_alloc_region (holy_phys_addr_t start,
					  holy_size_t size);
unsigned holy_relocator_firmware_fill_events (struct holy_relocator_mmap_event *events);
unsigned holy_relocator_firmware_get_max_events (void);
void holy_relocator_firmware_free_region (holy_phys_addr_t start,
					  holy_size_t size);
#endif

#endif
