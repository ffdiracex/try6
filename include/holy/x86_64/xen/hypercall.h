/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_XEN_CPU_HYPERCALL_HEADER
#define holy_XEN_CPU_HYPERCALL_HEADER 1

#include <holy/misc.h>

int EXPORT_FUNC (holy_xen_sched_op) (int cmd, void *arg) holy_ASM_ATTR;
int holy_xen_update_va_mapping (void *addr, uint64_t pte, uint64_t flags) holy_ASM_ATTR;
int EXPORT_FUNC (holy_xen_event_channel_op) (int op, void *arg) holy_ASM_ATTR;

int holy_xen_mmuext_op (mmuext_op_t * ops,
			unsigned int count,
			unsigned int *pdone, unsigned int foreigndom) holy_ASM_ATTR;
int EXPORT_FUNC (holy_xen_mmu_update) (const struct mmu_update * reqs,
				       unsigned count, unsigned *done_out,
				       unsigned foreigndom) holy_ASM_ATTR;
int EXPORT_FUNC (holy_xen_grant_table_op) (int, void *, int) holy_ASM_ATTR;

#endif
