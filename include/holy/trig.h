/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_TRIG_HEADER
#define holy_TRIG_HEADER 1

#define holy_TRIG_ANGLE_MAX 256
#define holy_TRIG_ANGLE_MASK 255
#define holy_TRIG_FRACTION_SCALE 16384

extern const short holy_trig_sintab[];
extern const short holy_trig_costab[];

static __inline int
holy_sin (int x)
{
  x &= holy_TRIG_ANGLE_MASK;
  return holy_trig_sintab[x];
}

static __inline int
holy_cos (int x)
{
  x &= holy_TRIG_ANGLE_MASK;
  return holy_trig_costab[x];
}

#endif /* ! holy_TRIG_HEADER */
