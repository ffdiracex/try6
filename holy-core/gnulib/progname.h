/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef _PROGNAME_H
#define _PROGNAME_H

/* Programs using this file should do the following in main():
     set_program_name (argv[0]);
 */


#ifdef __cplusplus
extern "C" {
#endif


/* String containing name the program is called with.  */
extern const char *program_name;

/* Set program_name, based on argv[0].
   argv0 must be a string allocated with indefinite extent, and must not be
   modified after this call.  */
extern void set_program_name (const char *argv0);

#if ENABLE_RELOCATABLE

/* Set program_name, based on argv[0], and original installation prefix and
   directory, for relocatability.  */
extern void set_program_name_and_installdir (const char *argv0,
                                             const char *orig_installprefix,
                                             const char *orig_installdir);
#undef set_program_name
#define set_program_name(ARG0) \
  set_program_name_and_installdir (ARG0, INSTALLPREFIX, INSTALLDIR)

/* Return the full pathname of the current executable, based on the earlier
   call to set_program_name_and_installdir.  Return NULL if unknown.  */
extern char *get_full_program_name (void);

#endif


#ifdef __cplusplus
}
#endif


#endif /* _PROGNAME_H */
