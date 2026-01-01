/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_DL_H
#define holy_DL_H	1

#include <holy/symbol.h>
#ifndef ASM_FILE
#include <holy/err.h>
#include <holy/types.h>
#include <holy/elf.h>
#include <holy/list.h>
#include <holy/misc.h>
#endif

/*
 * Macros holy_MOD_INIT and holy_MOD_FINI are also used by build rules
 * to collect module names, so we define them only when they are not
 * defined already.
 */
#ifndef ASM_FILE

#ifndef holy_MOD_INIT

#if !defined (holy_UTIL) && !defined (holy_MACHINE_EMU) && !defined (holy_KERNEL)

#define holy_MOD_INIT(name)	\
static void holy_mod_init (holy_dl_t mod __attribute__ ((unused))) __attribute__ ((used)); \
static void \
holy_mod_init (holy_dl_t mod __attribute__ ((unused)))

#define holy_MOD_FINI(name)	\
static void holy_mod_fini (void) __attribute__ ((used)); \
static void \
holy_mod_fini (void)

#elif defined (holy_KERNEL)

#define holy_MOD_INIT(name)	\
static void holy_mod_init (holy_dl_t mod __attribute__ ((unused))) __attribute__ ((used)); \
void \
holy_##name##_init (void) { holy_mod_init (0); } \
static void \
holy_mod_init (holy_dl_t mod __attribute__ ((unused)))

#define holy_MOD_FINI(name)	\
static void holy_mod_fini (void) __attribute__ ((used)); \
void \
holy_##name##_fini (void) { holy_mod_fini (); } \
static void \
holy_mod_fini (void)

#else

#define holy_MOD_INIT(name)	\
static void holy_mod_init (holy_dl_t mod __attribute__ ((unused))) __attribute__ ((used)); \
void holy_##name##_init (void); \
void \
holy_##name##_init (void) { holy_mod_init (0); } \
static void \
holy_mod_init (holy_dl_t mod __attribute__ ((unused)))

#define holy_MOD_FINI(name)	\
static void holy_mod_fini (void) __attribute__ ((used)); \
void holy_##name##_fini (void); \
void \
holy_##name##_fini (void) { holy_mod_fini (); } \
static void \
holy_mod_fini (void)

#endif

#endif

#endif

#ifndef ASM_FILE
#ifdef __APPLE__
#define holy_MOD_SECTION(x) "_" #x ", _" #x ""
#else
#define holy_MOD_SECTION(x) "." #x
#endif
#else
#ifdef __APPLE__
#define holy_MOD_SECTION(x) _ ## x , _ ##x
#else
#define holy_MOD_SECTION(x) . ## x
#endif
#endif

/* Me, Vladimir Serbinenko, hereby I add this module check as per new
   GNU module policy. Note that this license check is informative only.
   Modules have to be licensed under GPLv2 or GPLv2+ (optionally
   multi-licensed under other licences as well) independently of the
   presence of this check and solely by linking (module loading in holy
   constitutes linking) and holy core being licensed under GPLv2+.
   Be sure to understand your license obligations.
*/
#ifndef ASM_FILE
#if GNUC_PREREQ (3,2)
#define ATTRIBUTE_USED __used__
#else
#define ATTRIBUTE_USED __unused__
#endif
#define holy_MOD_LICENSE(license)	\
  static char holy_module_license[] __attribute__ ((section (holy_MOD_SECTION (module_license)), ATTRIBUTE_USED)) = "LICENSE=" license;
#define holy_MOD_DEP(name)	\
static const char holy_module_depend_##name[] \
 __attribute__((section(holy_MOD_SECTION(moddeps)), ATTRIBUTE_USED)) = #name
#define holy_MOD_NAME(name)	\
static const char holy_module_name_##name[] \
 __attribute__((section(holy_MOD_SECTION(modname)), __used__)) = #name
#else
#ifdef __APPLE__
.macro holy_MOD_LICENSE
  .section holy_MOD_SECTION(module_license)
  .ascii "LICENSE="
  .ascii $0
  .byte 0
.endm
#else
.macro holy_MOD_LICENSE license
  .section holy_MOD_SECTION(module_license), "a"
  .ascii "LICENSE="
  .ascii "\license"
  .byte 0
.endm
#endif
#endif

/* Under GPL license obligations you have to distribute your module
   under GPLv2(+). However, you can also distribute the same code under
   another license as long as GPLv2(+) version is provided.
*/
#define holy_MOD_DUAL_LICENSE(x)

#ifndef ASM_FILE

struct holy_dl_segment
{
  struct holy_dl_segment *next;
  void *addr;
  holy_size_t size;
  unsigned section;
};
typedef struct holy_dl_segment *holy_dl_segment_t;

struct holy_dl;

struct holy_dl_dep
{
  struct holy_dl_dep *next;
  struct holy_dl *mod;
};
typedef struct holy_dl_dep *holy_dl_dep_t;

#ifndef holy_UTIL
struct holy_dl
{
  char *name;
  int ref_count;
  holy_dl_dep_t dep;
  holy_dl_segment_t segment;
  Elf_Sym *symtab;
  holy_size_t symsize;
  void (*init) (struct holy_dl *mod);
  void (*fini) (void);
#if !defined (__i386__) && !defined (__x86_64__)
  void *got;
  void *gotptr;
  void *tramp;
  void *trampptr;
#endif
#ifdef __mips__
  holy_uint32_t *reginfo;
#endif
  void *base;
  holy_size_t sz;
  struct holy_dl *next;
};
#endif
typedef struct holy_dl *holy_dl_t;

holy_dl_t holy_dl_load_file (const char *filename);
holy_dl_t EXPORT_FUNC(holy_dl_load) (const char *name);
holy_dl_t holy_dl_load_core (void *addr, holy_size_t size);
holy_dl_t EXPORT_FUNC(holy_dl_load_core_noinit) (void *addr, holy_size_t size);
int EXPORT_FUNC(holy_dl_unload) (holy_dl_t mod);
void holy_dl_unload_unneeded (void);
int EXPORT_FUNC(holy_dl_ref) (holy_dl_t mod);
int EXPORT_FUNC(holy_dl_unref) (holy_dl_t mod);
extern holy_dl_t EXPORT_VAR(holy_dl_head);

#ifndef holy_UTIL

#define FOR_DL_MODULES(var) FOR_LIST_ELEMENTS ((var), (holy_dl_head))

#ifdef holy_MACHINE_EMU
void *
holy_osdep_dl_memalign (holy_size_t align, holy_size_t size);
void
holy_dl_osdep_dl_free (void *ptr);
#endif

static inline void
holy_dl_init (holy_dl_t mod)
{
  if (mod->init)
    (mod->init) (mod);

  mod->next = holy_dl_head;
  holy_dl_head = mod;
}

static inline holy_dl_t
holy_dl_get (const char *name)
{
  holy_dl_t l;

  FOR_DL_MODULES(l)
    if (holy_strcmp (name, l->name) == 0)
      return l;

  return 0;
}

#endif

holy_err_t holy_dl_register_symbol (const char *name, void *addr,
				    int isfunc, holy_dl_t mod);

holy_err_t holy_arch_dl_check_header (void *ehdr);
#ifndef holy_UTIL
holy_err_t
holy_arch_dl_relocate_symbols (holy_dl_t mod, void *ehdr,
			       Elf_Shdr *s, holy_dl_segment_t seg);
#endif

#if defined (_mips)
#define holy_LINKER_HAVE_INIT 1
void holy_arch_dl_init_linker (void);
#endif

#define holy_IA64_DL_TRAMP_ALIGN 16
#define holy_IA64_DL_GOT_ALIGN 16

holy_err_t
holy_ia64_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
				 holy_size_t *got);
holy_err_t
holy_arm64_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
				  holy_size_t *got);

#if defined (__ia64__)
#define holy_ARCH_DL_TRAMP_ALIGN holy_IA64_DL_TRAMP_ALIGN
#define holy_ARCH_DL_GOT_ALIGN holy_IA64_DL_GOT_ALIGN
#define holy_arch_dl_get_tramp_got_size holy_ia64_dl_get_tramp_got_size
#elif defined (__aarch64__)
#define holy_arch_dl_get_tramp_got_size holy_arm64_dl_get_tramp_got_size
#else
holy_err_t
holy_arch_dl_get_tramp_got_size (const void *ehdr, holy_size_t *tramp,
				 holy_size_t *got);
#endif

#if defined (__powerpc__) || defined (__mips__) || defined (__arm__)
#define holy_ARCH_DL_TRAMP_ALIGN 4
#define holy_ARCH_DL_GOT_ALIGN 4
#endif

#if defined (__aarch64__) || defined (__sparc__)
#define holy_ARCH_DL_TRAMP_ALIGN 8
#define holy_ARCH_DL_GOT_ALIGN 8
#endif

#endif

#endif /* ! holy_DL_H */
