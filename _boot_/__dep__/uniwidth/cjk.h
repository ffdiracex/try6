/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include "streq.h"

static int
is_cjk_encoding (const char *encoding)
{
  if (0
      /* Legacy Japanese encodings */
      || STREQ_OPT (encoding, "EUC-JP", 'E', 'U', 'C', '-', 'J', 'P', 0, 0, 0)
      /* Legacy Chinese encodings */
      || STREQ_OPT (encoding, "GB2312", 'G', 'B', '2', '3', '1', '2', 0, 0, 0)
      || STREQ_OPT (encoding, "GBK", 'G', 'B', 'K', 0, 0, 0, 0, 0, 0)
      || STREQ_OPT (encoding, "EUC-TW", 'E', 'U', 'C', '-', 'T', 'W', 0, 0, 0)
      || STREQ_OPT (encoding, "BIG5", 'B', 'I', 'G', '5', 0, 0, 0, 0, 0)
      /* Legacy Korean encodings */
      || STREQ_OPT (encoding, "EUC-KR", 'E', 'U', 'C', '-', 'K', 'R', 0, 0, 0)
      || STREQ_OPT (encoding, "CP949", 'C', 'P', '9', '4', '9', 0, 0, 0, 0)
      || STREQ_OPT (encoding, "JOHAB", 'J', 'O', 'H', 'A', 'B', 0, 0, 0, 0))
    return 1;
  return 0;
}
