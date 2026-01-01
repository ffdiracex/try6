/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_TEST_HEADER
#define holy_TEST_HEADER

#include <holy/dl.h>
#include <holy/list.h>
#include <holy/misc.h>
#include <holy/types.h>
#include <holy/symbol.h>

#include <holy/video.h>
#include <holy/video_fb.h>

#ifdef __cplusplus
extern "C" {
#endif

struct holy_test
{
  /* The next test.  */
  struct holy_test *next;
  struct holy_test **prev;

  /* The test name.  */
  char *name;

  /* The test main function.  */
  void (*main) (void);
};
typedef struct holy_test *holy_test_t;

extern holy_test_t holy_test_list;

void holy_test_register   (const char *name, void (*test) (void));
void holy_test_unregister (const char *name);

/* Execute a test and print results.  */
int holy_test_run (holy_test_t test);

/* Test `cond' for nonzero; log failure otherwise.  */
void holy_test_nonzero (int cond, const char *file,
			const char *func, holy_uint32_t line,
			const char *fmt, ...)
  __attribute__ ((format (GNU_PRINTF, 5, 6)));

/* Macro to fill in location details and an optional error message.  */
void holy_test_assert_helper (int cond, const char *file,
                            const char *func, holy_uint32_t line,
                            const char *condstr, const char *fmt, ...)
  __attribute__ ((format (GNU_PRINTF, 6, 7)));

#define holy_test_assert(cond, ...)				\
  holy_test_assert_helper(cond, holy_FILE, __FUNCTION__, __LINE__,     \
                         #cond, ## __VA_ARGS__);

void holy_unit_test_init (void);
void holy_unit_test_fini (void);

/* Macro to define a unit test.  */
#define holy_UNIT_TEST(name, funp)		\
  void holy_unit_test_init (void)		\
  {						\
    holy_test_register (name, funp);		\
  }						\
						\
  void holy_unit_test_fini (void)		\
  {						\
    holy_test_unregister (name);		\
  }

/* Macro to define a functional test.  */
#define holy_FUNCTIONAL_TEST(name, funp)	\
  holy_MOD_INIT(name)				\
  {						\
    holy_test_register (#name, funp);		\
  }						\
						\
  holy_MOD_FINI(name)				\
  {						\
    holy_test_unregister (#name);		\
  }

void
holy_video_checksum (const char *basename_in);
void
holy_video_checksum_end (void);
void
holy_terminal_input_fake_sequence (int *seq_in, int nseq_in);
void
holy_terminal_input_fake_sequence_end (void);
const char *
holy_video_checksum_get_modename (void);


#define holy_TEST_VIDEO_SMALL_N_MODES 7
#define holy_TEST_VIDEO_ALL_N_MODES 31

extern struct holy_video_mode_info holy_test_video_modes[holy_TEST_VIDEO_ALL_N_MODES];

int
holy_test_use_gfxterm (void);
void holy_test_use_gfxterm_end (void);

#ifdef __cplusplus
}
#endif

#endif /* ! holy_TEST_HEADER */
