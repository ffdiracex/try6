/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_XEN_CPU_HYPERCALL_HEADER
#define holy_XEN_CPU_HYPERCALL_HEADER 1

#include <holy/types.h>

int
EXPORT_FUNC (holy_xen_hypercall) (holy_uint32_t callno, holy_uint32_t a0,
				  holy_uint32_t a1, holy_uint32_t a2,
				  holy_uint32_t a3, holy_uint32_t a4,
				  holy_uint32_t a5)
__attribute__ ((regparm (3), cdecl));

static inline int
holy_xen_sched_op (int cmd, void *arg)
{
  return holy_xen_hypercall (__HYPERVISOR_sched_op, cmd, (holy_uint32_t) arg,
			     0, 0, 0, 0);
}

static inline int
holy_xen_mmu_update (const struct mmu_update *reqs,
		     unsigned count, unsigned *done_out, unsigned foreigndom)
{
  return holy_xen_hypercall (__HYPERVISOR_mmu_update, (holy_uint32_t) reqs,
			     (holy_uint32_t) count, (holy_uint32_t) done_out,
			     (holy_uint32_t) foreigndom, 0, 0);
}

static inline int
holy_xen_mmuext_op (mmuext_op_t * ops,
		    unsigned int count,
		    unsigned int *pdone, unsigned int foreigndom)
{
  return holy_xen_hypercall (__HYPERVISOR_mmuext_op, (holy_uint32_t) ops,
			     count, (holy_uint32_t) pdone, foreigndom, 0, 0);
}

static inline int
holy_xen_event_channel_op (int op, void *arg)
{
  return holy_xen_hypercall (__HYPERVISOR_event_channel_op, op,
			     (holy_uint32_t) arg, 0, 0, 0, 0);
}


static inline int
holy_xen_update_va_mapping (void *addr, uint64_t pte, uint32_t flags)
{
  return holy_xen_hypercall (__HYPERVISOR_update_va_mapping,
			     (holy_uint32_t) addr, pte, pte >> 32, flags, 0,
			     0);
}

static inline int
holy_xen_grant_table_op (int a, void *b, int c)
{
  return holy_xen_hypercall (__HYPERVISOR_grant_table_op, a,
			     (holy_uint32_t) b, c, 0, 0, 0);
}

static inline int
holy_xen_vm_assist (int cmd, int type)
{
  return holy_xen_hypercall (__HYPERVISOR_vm_assist, cmd, type, 0, 0, 0, 0);
}

#endif
