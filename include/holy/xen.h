/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_XEN_HEADER
#define holy_XEN_HEADER 1

#define __XEN_INTERFACE_VERSION__ 0x0003020a

#define memset holy_memset

#ifdef ASM_FILE
#define __ASSEMBLY__
#include <xen/xen.h>
#else

#include <holy/symbol.h>
#include <holy/types.h>
#include <holy/err.h>

#ifndef holy_SYMBOL_GENERATOR
typedef holy_int8_t int8_t;
typedef holy_int16_t int16_t;
typedef holy_uint8_t uint8_t;
typedef holy_uint16_t uint16_t;
typedef holy_uint32_t uint32_t;
typedef holy_uint64_t uint64_t;
#include <xen/xen.h>

#include <xen/sched.h>
#include <xen/grant_table.h>
#include <xen/io/console.h>
#include <xen/io/xs_wire.h>
#include <xen/io/xenbus.h>
#include <xen/io/protocols.h>
#endif

#include <holy/cpu/xen/hypercall.h>

extern holy_size_t EXPORT_VAR (holy_xen_n_allocated_shared_pages);


#define holy_XEN_LOG_PAGE_SIZE 12
#define holy_XEN_PAGE_SIZE (1 << holy_XEN_LOG_PAGE_SIZE)

extern volatile struct xencons_interface *holy_xen_xcons;
extern volatile struct shared_info *EXPORT_VAR (holy_xen_shared_info);
extern volatile struct xenstore_domain_interface *holy_xen_xenstore;
extern volatile grant_entry_v1_t *holy_xen_grant_table;

void EXPORT_FUNC (holy_xen_store_send) (const void *buf_, holy_size_t len);
void EXPORT_FUNC (holy_xen_store_recv) (void *buf_, holy_size_t len);
holy_err_t
EXPORT_FUNC (holy_xenstore_dir) (const char *dir,
				 int (*hook) (const char *dir,
					      void *hook_data),
				 void *hook_data);
void *EXPORT_FUNC (holy_xenstore_get_file) (const char *dir,
					    holy_size_t * len);
holy_err_t EXPORT_FUNC (holy_xenstore_write_file) (const char *dir,
						   const void *buf,
						   holy_size_t len);

typedef unsigned int holy_xen_grant_t;

void *EXPORT_FUNC (holy_xen_alloc_shared_page) (domid_t dom,
						holy_xen_grant_t * grnum);
void EXPORT_FUNC (holy_xen_free_shared_page) (void *ptr);

#define mb() asm volatile("mfence;sfence;" : : : "memory");
extern struct start_info *EXPORT_VAR (holy_xen_start_page_addr);

void holy_console_init (void);

void holy_xendisk_fini (void);
void holy_xendisk_init (void);

#ifdef __x86_64__
typedef holy_uint64_t holy_xen_mfn_t;
#else
typedef holy_uint32_t holy_xen_mfn_t;
#endif
typedef unsigned int holy_xen_evtchn_t;
#endif

#endif
