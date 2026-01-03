#include <holy/symbol.h>
#include <holy/xen.h>

FUNCTION(holy_xen_sched_op)
	movq $__HYPERVISOR_sched_op, %rax
	syscall
	ret

FUNCTION(holy_xen_event_channel_op)
	movq $__HYPERVISOR_event_channel_op, %rax
	syscall
	ret

FUNCTION(holy_xen_update_va_mapping)
	movq $__HYPERVISOR_update_va_mapping, %rax
	syscall
	ret

FUNCTION(holy_xen_mmuext_op)
	movq %rcx, %r10
	movq $__HYPERVISOR_mmuext_op, %rax
	syscall
	ret

FUNCTION(holy_xen_grant_table_op)
	movq $__HYPERVISOR_grant_table_op, %rax
	syscall
	ret

FUNCTION(holy_xen_mmu_update)
	movq %rcx, %r10
	movq $__HYPERVISOR_mmu_update, %rax
	syscall
	ret
