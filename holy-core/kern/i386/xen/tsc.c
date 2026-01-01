/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/time.h>
#include <holy/misc.h>
#include <holy/i386/tsc.h>
#include <holy/xen.h>

int
holy_tsc_calibrate_from_xen (void)
{
  holy_uint64_t t;
  t = holy_xen_shared_info->vcpu_info[0].time.tsc_to_system_mul;
  if (holy_xen_shared_info->vcpu_info[0].time.tsc_shift > 0)
    t <<= holy_xen_shared_info->vcpu_info[0].time.tsc_shift;
  else
    t >>= -holy_xen_shared_info->vcpu_info[0].time.tsc_shift;
  holy_tsc_rate = holy_divmod64 (t, 1000000, 0);
  return 1;
}
