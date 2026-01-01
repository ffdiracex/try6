/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_UTIL_INSTALL_HEADER
#define holy_UTIL_INSTALL_HEADER	1

#include <sys/types.h>
#include <stdio.h>

#include <holy/device.h>
#include <holy/disk.h>
#include <holy/emu/hostfile.h>

#define holy_INSTALL_OPTIONS					  \
  { "modules",      holy_INSTALL_OPTIONS_MODULES, N_("MODULES"),	  \
    0, N_("pre-load specified modules MODULES"), 1 },			  \
  { "install-modules", holy_INSTALL_OPTIONS_INSTALL_MODULES,	  \
    N_("MODULES"), 0,							  \
    N_("install only MODULES and their dependencies [default=all]"), 1 }, \
  { "themes", holy_INSTALL_OPTIONS_INSTALL_THEMES, N_("THEMES"),   \
    0, N_("install THEMES [default=%s]"), 1 },	 		          \
  { "fonts", holy_INSTALL_OPTIONS_INSTALL_FONTS, N_("FONTS"),	  \
    0, N_("install FONTS [default=%s]"), 1  },	  		          \
  { "locales", holy_INSTALL_OPTIONS_INSTALL_LOCALES, N_("LOCALES"),\
    0, N_("install only LOCALES [default=all]"), 1 },			  \
  { "compress", holy_INSTALL_OPTIONS_INSTALL_COMPRESS,		  \
    "no|xz|gz|lzo", 0,				  \
    N_("compress holy files [optional]"), 1 },			          \
  {"core-compress", holy_INSTALL_OPTIONS_INSTALL_CORE_COMPRESS,		\
      "xz|none|auto",						\
      0, N_("choose the compression to use for core image"), 2},	\
    /* TRANSLATORS: platform here isn't identifier. It can be translated. */ \
  { "directory", 'd', N_("DIR"), 0,					\
    N_("use images and modules under DIR [default=%s/<platform>]"), 1 },  \
  { "override-directory", holy_INSTALL_OPTIONS_DIRECTORY2,		\
      N_("DIR"), OPTION_HIDDEN,						\
    N_("use images and modules under DIR [default=%s/<platform>]"), 1 },  \
  { "locale-directory", holy_INSTALL_OPTIONS_LOCALE_DIRECTORY,		\
      N_("DIR"), 0,							\
    N_("use translations under DIR [default=%s]"), 1 },			\
  { "themes-directory", holy_INSTALL_OPTIONS_THEMES_DIRECTORY,		\
      N_("DIR"), OPTION_HIDDEN,						\
    N_("use themes under DIR [default=%s]"), 1 },			\
  { "holy-mkimage", holy_INSTALL_OPTIONS_holy_MKIMAGE,		\
      "FILE", OPTION_HIDDEN, 0, 1 },					\
    /* TRANSLATORS: "embed" is a verb (command description).  "*/	\
  { "pubkey",   'k', N_("FILE"), 0,					\
      N_("embed FILE as public key for signature checking"), 0},	\
  { "verbose", 'v', 0, 0,						\
    N_("print verbose messages."), 1 }

int
holy_install_parse (int key, char *arg);

void
holy_install_push_module (const char *val);

void
holy_install_pop_module (void);

char *
holy_install_help_filter (int key, const char *text,
			  void *input __attribute__ ((unused)));

enum holy_install_plat
  {
    holy_INSTALL_PLATFORM_I386_PC,
    holy_INSTALL_PLATFORM_I386_EFI,
    holy_INSTALL_PLATFORM_I386_QEMU,
    holy_INSTALL_PLATFORM_I386_COREBOOT,
    holy_INSTALL_PLATFORM_I386_MULTIBOOT,
    holy_INSTALL_PLATFORM_I386_IEEE1275,
    holy_INSTALL_PLATFORM_X86_64_EFI,
    holy_INSTALL_PLATFORM_MIPSEL_LOONGSON,
    holy_INSTALL_PLATFORM_SPARC64_IEEE1275,
    holy_INSTALL_PLATFORM_POWERPC_IEEE1275,
    holy_INSTALL_PLATFORM_MIPSEL_ARC,
    holy_INSTALL_PLATFORM_MIPS_ARC,
    holy_INSTALL_PLATFORM_IA64_EFI,
    holy_INSTALL_PLATFORM_ARM_UBOOT,
    holy_INSTALL_PLATFORM_ARM_EFI,
    holy_INSTALL_PLATFORM_MIPSEL_QEMU_MIPS,
    holy_INSTALL_PLATFORM_MIPS_QEMU_MIPS,
    holy_INSTALL_PLATFORM_I386_XEN,
    holy_INSTALL_PLATFORM_X86_64_XEN,
    holy_INSTALL_PLATFORM_ARM64_EFI,
    holy_INSTALL_PLATFORM_MAX
  };

enum holy_install_options {
  holy_INSTALL_OPTIONS_DIRECTORY = 'd',
  holy_INSTALL_OPTIONS_VERBOSITY = 'v',
  holy_INSTALL_OPTIONS_MODULES = 0x201,
  holy_INSTALL_OPTIONS_INSTALL_MODULES,
  holy_INSTALL_OPTIONS_INSTALL_THEMES,
  holy_INSTALL_OPTIONS_INSTALL_FONTS,
  holy_INSTALL_OPTIONS_INSTALL_LOCALES,
  holy_INSTALL_OPTIONS_INSTALL_COMPRESS,
  holy_INSTALL_OPTIONS_DIRECTORY2,
  holy_INSTALL_OPTIONS_LOCALE_DIRECTORY,
  holy_INSTALL_OPTIONS_THEMES_DIRECTORY,
  holy_INSTALL_OPTIONS_holy_MKIMAGE,
  holy_INSTALL_OPTIONS_INSTALL_CORE_COMPRESS
};

extern char *holy_install_source_directory;

enum holy_install_plat
holy_install_get_target (const char *src);
void
holy_install_mkdir_p (const char *dst);

void
holy_install_copy_files (const char *src,
			 const char *dst,
			 enum holy_install_plat platid);
char *
holy_install_get_platform_name (enum holy_install_plat platid);

const char *
holy_install_get_platform_cpu (enum holy_install_plat platid);

const char *
holy_install_get_platform_platform (enum holy_install_plat platid);

char *
holy_install_get_platforms_string (void);

typedef enum {
  holy_COMPRESSION_AUTO,
  holy_COMPRESSION_NONE,
  holy_COMPRESSION_XZ,
  holy_COMPRESSION_LZMA
} holy_compression_t;

void
holy_install_make_image_wrap (const char *dir, const char *prefix,
			      const char *outname, char *memdisk_path,
			      char *config_path,
			      const char *format, int note);
void
holy_install_make_image_wrap_file (const char *dir, const char *prefix,
				   FILE *fp, const char *outname,
				   char *memdisk_path,
				   char *config_path,
				   const char *mkimage_target, int note);

int
holy_install_copy_file (const char *src,
			const char *dst,
			int is_critical);

struct holy_install_image_target_desc;

void
holy_install_generate_image (const char *dir, const char *prefix,
			     FILE *out,
			     const char *outname, char *mods[],
			     char *memdisk_path, char **pubkey_paths,
			     size_t npubkeys,
			     char *config_path,
			     const struct holy_install_image_target_desc *image_target,
			     int note,
			     holy_compression_t comp);

const struct holy_install_image_target_desc *
holy_install_get_image_target (const char *arg);

void
holy_util_bios_setup (const char *dir,
		      const char *boot_file, const char *core_file,
		      const char *dest, int force,
		      int fs_probe, int allow_floppy,
		      int add_rs_codes);
void
holy_util_sparc_setup (const char *dir,
		       const char *boot_file, const char *core_file,
		       const char *dest, int force,
		       int fs_probe, int allow_floppy,
		       int add_rs_codes);

char *
holy_install_get_image_targets_string (void);

const char *
holy_util_get_target_dirname (const struct holy_install_image_target_desc *t);

void
holy_install_create_envblk_file (const char *name);

const char *
holy_install_get_default_x86_platform (void);

void
holy_install_register_efi (holy_device_t efidir_holy_dev,
			   const char *efifile_path,
			   const char *efi_distributor);

void
holy_install_register_ieee1275 (int is_prep, const char *install_device,
				int partno, const char *relpath);

void
holy_install_sgi_setup (const char *install_device,
			const char *imgfile, const char *destname);

int 
holy_install_compress_gzip (const char *src, const char *dest);
int 
holy_install_compress_lzop (const char *src, const char *dest);
int 
holy_install_compress_xz (const char *src, const char *dest);

void
holy_install_get_blocklist (holy_device_t root_dev,
			    const char *core_path, const char *core_img,
			    size_t core_size,
			    void (*callback) (holy_disk_addr_t sector,
					      unsigned offset,
					      unsigned length,
					      void *data),
			    void *hook_data);

void
holy_util_create_envblk_file (const char *name);

void
holy_util_glue_efi (const char *file32, const char *file64, const char *out);

void
holy_util_render_label (const char *label_font,
			const char *label_bgcolor,
			const char *label_color,
			const char *label_string,
			const char *label);

const char *
holy_util_get_target_name (const struct holy_install_image_target_desc *t);

extern char *holy_install_copy_buffer;
#define holy_INSTALL_COPY_BUFFER_SIZE 1048576

#endif
