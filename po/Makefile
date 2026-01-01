# Origin: gettext-0.18
GETTEXT_MACRO_VERSION = 0.18

PACKAGE = holy
VERSION = 2.02
PACKAGE_BUGREPORT = bashscripter123@gmail.com

SHELL = /bin/sh


srcdir = .
top_srcdir = ..


prefix = /usr/local
exec_prefix = /usr/local
datarootdir = /usr/local/share
datadir = ${datarootdir}
localedir = ${datarootdir}/locale
gettextsrcdir = $(datadir)/gettext/po

INSTALL = /usr/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644

# We use $(mkdir_p).
# In automake <= 1.9.x, $(mkdir_p) is defined either as "mkdir -p --" or as
# "$(mkinstalldirs)" or as "$(install_sh) -d". For these automake versions,
# ${SHELL} /home/felix/coding/build-aux/install-sh does not start with $(SHELL), so we add it.
# In automake >= 1.10, $(MKDIR_P) is derived from ${MKDIR_P}, which is defined
# either as "/path/to/mkdir -p" or ".../install-sh -c -d". For these automake
# versions, $(mkinstalldirs) and $(install_sh) are unused.
mkinstalldirs = $(SHELL) ${SHELL} /home/felix/coding/build-aux/install-sh -d
install_sh = $(SHELL) ${SHELL} /home/felix/coding/build-aux/install-sh
MKDIR_P = /usr/bin/mkdir -p
mkdir_p = $(MKDIR_P)

GMSGFMT_ = /usr/bin/msgfmt
GMSGFMT_no = /usr/bin/msgfmt
GMSGFMT_yes = /usr/bin/msgfmt
GMSGFMT = $(GMSGFMT_$(USE_MSGCTXT))
MSGFMT_ = /usr/bin/msgfmt
MSGFMT_no = /usr/bin/msgfmt
MSGFMT_yes = /usr/bin/msgfmt
MSGFMT = $(MSGFMT_$(USE_MSGCTXT))
XGETTEXT_ = /usr/bin/xgettext
XGETTEXT_no = /usr/bin/xgettext
XGETTEXT_yes = /usr/bin/xgettext
XGETTEXT = $(XGETTEXT_$(USE_MSGCTXT))
MSGMERGE = msgmerge
MSGMERGE_UPDATE = /usr/bin/msgmerge --update
MSGINIT = msginit
MSGCONV = msgconv
MSGFILTER = msgfilter

POFILES = 
GMOFILES = 
UPDATEPOFILES = 
DUMMYPOFILES = 
DISTFILES.common = Makefile.in.in remove-potcdate.sin \
$(DISTFILES.common.extra1) $(DISTFILES.common.extra2) \
$(DISTFILES.common.extra3) $(DISTFILES.common.extra4) $(DISTFILES.common.extra5)
DISTFILES = $(DISTFILES.common) Makevars POTFILES.in POTFILES-shell.in \
$(POFILES) $(GMOFILES) \
$(DISTFILES.extra1) $(DISTFILES.extra2) $(DISTFILES.extra3) $(DISTFILES.extra4) $(DISTFILES.extra5) holy.d.sed README

POTFILES = \
     .././config-util.h \
     .././config.h \
     .././holy-core/boot/decompressor/minilib.c \
     .././holy-core/boot/decompressor/none.c \
     .././holy-core/boot/decompressor/xz.c \
     .././holy-core/bus/bonito.c \
     .././holy-core/bus/cs5536.c \
     .././holy-core/bus/emu/pci.c \
     .././holy-core/bus/i386/ieee1275/pci.c \
     .././holy-core/bus/pci.c \
     .././holy-core/bus/usb/ehci.c \
     .././holy-core/bus/usb/ohci.c \
     .././holy-core/bus/usb/serial/common.c \
     .././holy-core/bus/usb/serial/ftdi.c \
     .././holy-core/bus/usb/serial/pl2303.c \
     .././holy-core/bus/usb/serial/usbdebug_late.c \
     .././holy-core/bus/usb/uhci.c \
     .././holy-core/bus/usb/usb.c \
     .././holy-core/bus/usb/usbhub.c \
     .././holy-core/bus/usb/usbtrans.c \
     .././holy-core/commands/acpi.c \
     .././holy-core/commands/acpihalt.c \
     .././holy-core/commands/arc/lsdev.c \
     .././holy-core/commands/blocklist.c \
     .././holy-core/commands/boot.c \
     .././holy-core/commands/boottime.c \
     .././holy-core/commands/cacheinfo.c \
     .././holy-core/commands/cat.c \
     .././holy-core/commands/cmp.c \
     .././holy-core/commands/configfile.c \
     .././holy-core/commands/date.c \
     .././holy-core/commands/echo.c \
     .././holy-core/commands/efi/efifwsetup.c \
     .././holy-core/commands/efi/fixvideo.c \
     .././holy-core/commands/efi/getenv.c \
     .././holy-core/commands/efi/loadbios.c \
     .././holy-core/commands/efi/lsefi.c \
     .././holy-core/commands/efi/lsefimmap.c \
     .././holy-core/commands/efi/lsefisystab.c \
     .././holy-core/commands/efi/lssal.c \
     .././holy-core/commands/efi/smbios.c \
     .././holy-core/commands/eval.c \
     .././holy-core/commands/extcmd.c \
     .././holy-core/commands/file.c \
     .././holy-core/commands/file32.c \
     .././holy-core/commands/file64.c \
     .././holy-core/commands/fileXX.c \
     .././holy-core/commands/fwconfig.c \
     .././holy-core/commands/gptprio.c \
     .././holy-core/commands/gptrepair.c \
     .././holy-core/commands/gptsync.c \
     .././holy-core/commands/halt.c \
     .././holy-core/commands/hashsum.c \
     .././holy-core/commands/hdparm.c \
     .././holy-core/commands/help.c \
     .././holy-core/commands/hexdump.c \
     .././holy-core/commands/i386/cmosdump.c \
     .././holy-core/commands/i386/cmostest.c \
     .././holy-core/commands/i386/coreboot/cb_timestamps.c \
     .././holy-core/commands/i386/coreboot/cbls.c \
     .././holy-core/commands/i386/cpuid.c \
     .././holy-core/commands/i386/pc/drivemap.c \
     .././holy-core/commands/i386/pc/halt.c \
     .././holy-core/commands/i386/pc/lsapm.c \
     .././holy-core/commands/i386/pc/play.c \
     .././holy-core/commands/i386/pc/sendkey.c \
     .././holy-core/commands/i386/pc/smbios.c \
     .././holy-core/commands/ieee1275/suspend.c \
     .././holy-core/commands/iorw.c \
     .././holy-core/commands/keylayouts.c \
     .././holy-core/commands/keystatus.c \
     .././holy-core/commands/legacycfg.c \
     .././holy-core/commands/loadenv.c \
     .././holy-core/commands/ls.c \
     .././holy-core/commands/lsacpi.c \
     .././holy-core/commands/lsmmap.c \
     .././holy-core/commands/lspci.c \
     .././holy-core/commands/macbless.c \
     .././holy-core/commands/memrw.c \
     .././holy-core/commands/menuentry.c \
     .././holy-core/commands/minicmd.c \
     .././holy-core/commands/mips/loongson/lsspd.c \
     .././holy-core/commands/nativedisk.c \
     .././holy-core/commands/parttool.c \
     .././holy-core/commands/password.c \
     .././holy-core/commands/password_pbkdf2.c \
     .././holy-core/commands/pcidump.c \
     .././holy-core/commands/probe.c \
     .././holy-core/commands/read.c \
     .././holy-core/commands/reboot.c \
     .././holy-core/commands/regexp.c \
     .././holy-core/commands/search.c \
     .././holy-core/commands/search_disk_uuid.c \
     .././holy-core/commands/search_file.c \
     .././holy-core/commands/search_label.c \
     .././holy-core/commands/search_part_label.c \
     .././holy-core/commands/search_part_uuid.c \
     .././holy-core/commands/search_uuid.c \
     .././holy-core/commands/search_wrap.c \
     .././holy-core/commands/setpci.c \
     .././holy-core/commands/sleep.c \
     .././holy-core/commands/smbios.c \
     .././holy-core/commands/syslinuxcfg.c \
     .././holy-core/commands/terminal.c \
     .././holy-core/commands/test.c \
     .././holy-core/commands/testload.c \
     .././holy-core/commands/testspeed.c \
     .././holy-core/commands/time.c \
     .././holy-core/commands/tr.c \
     .././holy-core/commands/true.c \
     .././holy-core/commands/usbtest.c \
     .././holy-core/commands/verify.c \
     .././holy-core/commands/videoinfo.c \
     .././holy-core/commands/videotest.c \
     .././holy-core/commands/wildcard.c \
     .././holy-core/commands/xen/lsxen.c \
     .././holy-core/commands/xnu_uuid.c \
     .././holy-core/disk/AFSplitter.c \
     .././holy-core/disk/ahci.c \
     .././holy-core/disk/arc/arcdisk.c \
     .././holy-core/disk/ata.c \
     .././holy-core/disk/cryptodisk.c \
     .././holy-core/disk/diskfilter.c \
     .././holy-core/disk/dmraid_nvidia.c \
     .././holy-core/disk/efi/efidisk.c \
     .././holy-core/disk/geli.c \
     .././holy-core/disk/host.c \
     .././holy-core/disk/i386/pc/biosdisk.c \
     .././holy-core/disk/ieee1275/nand.c \
     .././holy-core/disk/ieee1275/ofdisk.c \
     .././holy-core/disk/ldm.c \
     .././holy-core/disk/loopback.c \
     .././holy-core/disk/luks.c \
     .././holy-core/disk/lvm.c \
     .././holy-core/disk/mdraid1x_linux.c \
     .././holy-core/disk/mdraid_linux.c \
     .././holy-core/disk/mdraid_linux_be.c \
     .././holy-core/disk/memdisk.c \
     .././holy-core/disk/pata.c \
     .././holy-core/disk/raid5_recover.c \
     .././holy-core/disk/raid6_recover.c \
     .././holy-core/disk/scsi.c \
     .././holy-core/disk/uboot/ubootdisk.c \
     .././holy-core/disk/usbms.c \
     .././holy-core/disk/xen/xendisk.c \
     .././holy-core/efiemu/i386/coredetect.c \
     .././holy-core/efiemu/i386/loadcore32.c \
     .././holy-core/efiemu/i386/loadcore64.c \
     .././holy-core/efiemu/i386/nocfgtables.c \
     .././holy-core/efiemu/i386/pc/cfgtables.c \
     .././holy-core/efiemu/loadcore.c \
     .././holy-core/efiemu/loadcore32.c \
     .././holy-core/efiemu/loadcore64.c \
     .././holy-core/efiemu/loadcore_common.c \
     .././holy-core/efiemu/main.c \
     .././holy-core/efiemu/mm.c \
     .././holy-core/efiemu/pnvram.c \
     .././holy-core/efiemu/prepare.c \
     .././holy-core/efiemu/prepare32.c \
     .././holy-core/efiemu/prepare64.c \
     .././holy-core/efiemu/runtime/config.h \
     .././holy-core/efiemu/runtime/efiemu.c \
     .././holy-core/efiemu/symbols.c \
     .././holy-core/font/font.c \
     .././holy-core/font/font_cmd.c \
     .././holy-core/fs/affs.c \
     .././holy-core/fs/afs.c \
     .././holy-core/fs/archelp.c \
     .././holy-core/fs/bfs.c \
     .././holy-core/fs/btrfs.c \
     .././holy-core/fs/cbfs.c \
     .././holy-core/fs/cpio.c \
     .././holy-core/fs/cpio_be.c \
     .././holy-core/fs/cpio_common.c \
     .././holy-core/fs/exfat.c \
     .././holy-core/fs/ext2.c \
     .././holy-core/fs/fat.c \
     .././holy-core/fs/fshelp.c \
     .././holy-core/fs/hfs.c \
     .././holy-core/fs/hfsplus.c \
     .././holy-core/fs/hfspluscomp.c \
     .././holy-core/fs/iso9660.c \
     .././holy-core/fs/jfs.c \
     .././holy-core/fs/minix.c \
     .././holy-core/fs/minix2.c \
     .././holy-core/fs/minix2_be.c \
     .././holy-core/fs/minix3.c \
     .././holy-core/fs/minix3_be.c \
     .././holy-core/fs/minix_be.c \
     .././holy-core/fs/newc.c \
     .././holy-core/fs/nilfs2.c \
     .././holy-core/fs/ntfs.c \
     .././holy-core/fs/ntfscomp.c \
     .././holy-core/fs/odc.c \
     .././holy-core/fs/proc.c \
     .././holy-core/fs/reiserfs.c \
     .././holy-core/fs/romfs.c \
     .././holy-core/fs/sfs.c \
     .././holy-core/fs/squash4.c \
     .././holy-core/fs/tar.c \
     .././holy-core/fs/udf.c \
     .././holy-core/fs/ufs.c \
     .././holy-core/fs/ufs2.c \
     .././holy-core/fs/ufs_be.c \
     .././holy-core/fs/xfs.c \
     .././holy-core/fs/zfs/zfs.c \
     .././holy-core/fs/zfs/zfs_fletcher.c \
     .././holy-core/fs/zfs/zfs_lz4.c \
     .././holy-core/fs/zfs/zfs_lzjb.c \
     .././holy-core/fs/zfs/zfs_sha256.c \
     .././holy-core/fs/zfs/zfscrypt.c \
     .././holy-core/fs/zfs/zfsinfo.c \
     .././holy-core/gdb/cstub.c \
     .././holy-core/gdb/gdb.c \
     .././holy-core/gdb/i386/idt.c \
     .././holy-core/gdb/i386/signal.c \
     .././holy-core/gentrigtables.c \
     .././holy-core/gettext/gettext.c \
     .././holy-core/gfxmenu/font.c \
     .././holy-core/gfxmenu/gfxmenu.c \
     .././holy-core/gfxmenu/gui_box.c \
     .././holy-core/gfxmenu/gui_canvas.c \
     .././holy-core/gfxmenu/gui_circular_progress.c \
     .././holy-core/gfxmenu/gui_image.c \
     .././holy-core/gfxmenu/gui_label.c \
     .././holy-core/gfxmenu/gui_list.c \
     .././holy-core/gfxmenu/gui_progress_bar.c \
     .././holy-core/gfxmenu/gui_string_util.c \
     .././holy-core/gfxmenu/gui_util.c \
     .././holy-core/gfxmenu/icon_manager.c \
     .././holy-core/gfxmenu/theme_loader.c \
     .././holy-core/gfxmenu/view.c \
     .././holy-core/gfxmenu/widget-box.c \
     .././holy-core/gnulib/alloca.c \
     .././holy-core/gnulib/alloca.in.h \
     .././holy-core/gnulib/argp-ba.c \
     .././holy-core/gnulib/argp-eexst.c \
     .././holy-core/gnulib/argp-fmtstream.c \
     .././holy-core/gnulib/argp-fmtstream.h \
     .././holy-core/gnulib/argp-fs-xinl.c \
     .././holy-core/gnulib/argp-help.c \
     .././holy-core/gnulib/argp-namefrob.h \
     .././holy-core/gnulib/argp-parse.c \
     .././holy-core/gnulib/argp-pin.c \
     .././holy-core/gnulib/argp-pv.c \
     .././holy-core/gnulib/argp-pvh.c \
     .././holy-core/gnulib/argp-xinl.c \
     .././holy-core/gnulib/argp.h \
     .././holy-core/gnulib/asnprintf.c \
     .././holy-core/gnulib/basename-lgpl.c \
     .././holy-core/gnulib/btowc.c \
     .././holy-core/gnulib/dirname-lgpl.c \
     .././holy-core/gnulib/dirname.h \
     .././holy-core/gnulib/dosname.h \
     .././holy-core/gnulib/errno.in.h \
     .././holy-core/gnulib/error.c \
     .././holy-core/gnulib/error.h \
     .././holy-core/gnulib/float+.h \
     .././holy-core/gnulib/float.c \
     .././holy-core/gnulib/float.in.h \
     .././holy-core/gnulib/fnmatch.c \
     .././holy-core/gnulib/fnmatch.in.h \
     .././holy-core/gnulib/fnmatch_loop.c \
     .././holy-core/gnulib/getdelim.c \
     .././holy-core/gnulib/getline.c \
     .././holy-core/gnulib/getopt.c \
     .././holy-core/gnulib/getopt.in.h \
     .././holy-core/gnulib/getopt1.c \
     .././holy-core/gnulib/getopt_int.h \
     .././holy-core/gnulib/gettext.h \
     .././holy-core/gnulib/intprops.h \
     .././holy-core/gnulib/itold.c \
     .././holy-core/gnulib/langinfo.in.h \
     .././holy-core/gnulib/localcharset.c \
     .././holy-core/gnulib/localcharset.h \
     .././holy-core/gnulib/locale.in.h \
     .././holy-core/gnulib/localeconv.c \
     .././holy-core/gnulib/malloc.c \
     .././holy-core/gnulib/mbrtowc.c \
     .././holy-core/gnulib/mbsinit.c \
     .././holy-core/gnulib/mbsrtowcs-impl.h \
     .././holy-core/gnulib/mbsrtowcs-state.c \
     .././holy-core/gnulib/mbsrtowcs.c \
     .././holy-core/gnulib/mbswidth.c \
     .././holy-core/gnulib/mbswidth.h \
     .././holy-core/gnulib/mbtowc-impl.h \
     .././holy-core/gnulib/mbtowc.c \
     .././holy-core/gnulib/memchr.c \
     .././holy-core/gnulib/mempcpy.c \
     .././holy-core/gnulib/msvc-inval.c \
     .././holy-core/gnulib/msvc-inval.h \
     .././holy-core/gnulib/msvc-nothrow.c \
     .././holy-core/gnulib/msvc-nothrow.h \
     .././holy-core/gnulib/nl_langinfo.c \
     .././holy-core/gnulib/printf-args.c \
     .././holy-core/gnulib/printf-args.h \
     .././holy-core/gnulib/printf-parse.c \
     .././holy-core/gnulib/printf-parse.h \
     .././holy-core/gnulib/progname.c \
     .././holy-core/gnulib/progname.h \
     .././holy-core/gnulib/rawmemchr.c \
     .././holy-core/gnulib/realloc.c \
     .././holy-core/gnulib/regcomp.c \
     .././holy-core/gnulib/regex.c \
     .././holy-core/gnulib/regex.h \
     .././holy-core/gnulib/regex_internal.c \
     .././holy-core/gnulib/regex_internal.h \
     .././holy-core/gnulib/regexec.c \
     .././holy-core/gnulib/size_max.h \
     .././holy-core/gnulib/sleep.c \
     .././holy-core/gnulib/stdalign.in.h \
     .././holy-core/gnulib/stdbool.in.h \
     .././holy-core/gnulib/stddef.in.h \
     .././holy-core/gnulib/stdint.in.h \
     .././holy-core/gnulib/stdio.in.h \
     .././holy-core/gnulib/stdlib.in.h \
     .././holy-core/gnulib/strcasecmp.c \
     .././holy-core/gnulib/strchrnul.c \
     .././holy-core/gnulib/streq.h \
     .././holy-core/gnulib/strerror-override.c \
     .././holy-core/gnulib/strerror-override.h \
     .././holy-core/gnulib/strerror.c \
     .././holy-core/gnulib/string.in.h \
     .././holy-core/gnulib/strings.in.h \
     .././holy-core/gnulib/stripslash.c \
     .././holy-core/gnulib/strncasecmp.c \
     .././holy-core/gnulib/strndup.c \
     .././holy-core/gnulib/strnlen.c \
     .././holy-core/gnulib/strnlen1.c \
     .././holy-core/gnulib/strnlen1.h \
     .././holy-core/gnulib/sys_types.in.h \
     .././holy-core/gnulib/sysexits.in.h \
     .././holy-core/gnulib/unistd.c \
     .././holy-core/gnulib/unistd.in.h \
     .././holy-core/gnulib/unitypes.in.h \
     .././holy-core/gnulib/uniwidth.in.h \
     .././holy-core/gnulib/uniwidth/cjk.h \
     .././holy-core/gnulib/uniwidth/width.c \
     .././holy-core/gnulib/vasnprintf.c \
     .././holy-core/gnulib/vasnprintf.h \
     .././holy-core/gnulib/verify.h \
     .././holy-core/gnulib/vsnprintf.c \
     .././holy-core/gnulib/wchar.in.h \
     .././holy-core/gnulib/wcrtomb.c \
     .././holy-core/gnulib/wctype-h.c \
     .././holy-core/gnulib/wctype.in.h \
     .././holy-core/gnulib/wcwidth.c \
     .././holy-core/gnulib/xsize.c \
     .././holy-core/gnulib/xsize.h \
     .././holy-core/hello/hello.c \
     .././holy-core/hook/datehook.c \
     .././holy-core/io/bufio.c \
     .././holy-core/io/gzio.c \
     .././holy-core/io/lzopio.c \
     .././holy-core/io/offset.c \
     .././holy-core/io/xzio.c \
     .././holy-core/kern/acpi.c \
     .././holy-core/kern/arm/cache.c \
     .././holy-core/kern/arm/dl.c \
     .././holy-core/kern/arm/dl_helper.c \
     .././holy-core/kern/arm/efi/init.c \
     .././holy-core/kern/arm/efi/misc.c \
     .././holy-core/kern/arm64/cache.c \
     .././holy-core/kern/arm64/dl.c \
     .././holy-core/kern/arm64/dl_helper.c \
     .././holy-core/kern/arm64/efi/init.c \
     .././holy-core/kern/command.c \
     .././holy-core/kern/compiler-rt.c \
     .././holy-core/kern/corecmd.c \
     .././holy-core/kern/device.c \
     .././holy-core/kern/disk.c \
     .././holy-core/kern/disk_common.c \
     .././holy-core/kern/dl.c \
     .././holy-core/kern/efi/acpi.c \
     .././holy-core/kern/efi/efi.c \
     .././holy-core/kern/efi/fdt.c \
     .././holy-core/kern/efi/init.c \
     .././holy-core/kern/efi/mm.c \
     .././holy-core/kern/efi/tpm.c \
     .././holy-core/kern/elf.c \
     .././holy-core/kern/elfXX.c \
     .././holy-core/kern/emu/argp_common.c \
     .././holy-core/kern/emu/cache.c \
     .././holy-core/kern/emu/full.c \
     .././holy-core/kern/emu/hostdisk.c \
     .././holy-core/kern/emu/hostfs.c \
     .././holy-core/kern/emu/lite.c \
     .././holy-core/kern/emu/main.c \
     .././holy-core/kern/emu/misc.c \
     .././holy-core/kern/emu/mm.c \
     .././holy-core/kern/emu/time.c \
     .././holy-core/kern/env.c \
     .././holy-core/kern/err.c \
     .././holy-core/kern/file.c \
     .././holy-core/kern/fs.c \
     .././holy-core/kern/generic/millisleep.c \
     .././holy-core/kern/generic/rtc_get_time_ms.c \
     .././holy-core/kern/i386/coreboot/cbtable.c \
     .././holy-core/kern/i386/coreboot/init.c \
     .././holy-core/kern/i386/coreboot/mmap.c \
     .././holy-core/kern/i386/dl.c \
     .././holy-core/kern/i386/efi/init.c \
     .././holy-core/kern/i386/efi/tsc.c \
     .././holy-core/kern/i386/multiboot_mmap.c \
     .././holy-core/kern/i386/pc/acpi.c \
     .././holy-core/kern/i386/pc/init.c \
     .././holy-core/kern/i386/pc/mmap.c \
     .././holy-core/kern/i386/pc/tpm.c \
     .././holy-core/kern/i386/qemu/init.c \
     .././holy-core/kern/i386/qemu/mmap.c \
     .././holy-core/kern/i386/tsc.c \
     .././holy-core/kern/i386/tsc_pit.c \
     .././holy-core/kern/i386/tsc_pmtimer.c \
     .././holy-core/kern/i386/xen/tsc.c \
     .././holy-core/kern/ia64/cache.c \
     .././holy-core/kern/ia64/dl.c \
     .././holy-core/kern/ia64/dl_helper.c \
     .././holy-core/kern/ia64/efi/init.c \
     .././holy-core/kern/ieee1275/cmain.c \
     .././holy-core/kern/ieee1275/ieee1275.c \
     .././holy-core/kern/ieee1275/init.c \
     .././holy-core/kern/ieee1275/mmap.c \
     .././holy-core/kern/ieee1275/openfw.c \
     .././holy-core/kern/list.c \
     .././holy-core/kern/main.c \
     .././holy-core/kern/mips/arc/init.c \
     .././holy-core/kern/mips/dl.c \
     .././holy-core/kern/mips/init.c \
     .././holy-core/kern/mips/loongson/init.c \
     .././holy-core/kern/mips/qemu_mips/init.c \
     .././holy-core/kern/misc.c \
     .././holy-core/kern/mm.c \
     .././holy-core/kern/parser.c \
     .././holy-core/kern/partition.c \
     .././holy-core/kern/powerpc/dl.c \
     .././holy-core/kern/rescue_parser.c \
     .././holy-core/kern/rescue_reader.c \
     .././holy-core/kern/sparc64/dl.c \
     .././holy-core/kern/sparc64/ieee1275/ieee1275.c \
     .././holy-core/kern/term.c \
     .././holy-core/kern/time.c \
     .././holy-core/kern/tpm.c \
     .././holy-core/kern/uboot/hw.c \
     .././holy-core/kern/uboot/init.c \
     .././holy-core/kern/uboot/uboot.c \
     .././holy-core/kern/vga_init.c \
     .././holy-core/kern/x86_64/dl.c \
     .././holy-core/kern/xen/init.c \
     .././holy-core/lib/LzFind.c \
     .././holy-core/lib/LzmaDec.c \
     .././holy-core/lib/LzmaEnc.c \
     .././holy-core/lib/adler32.c \
     .././holy-core/lib/arc/datetime.c \
     .././holy-core/lib/arg.c \
     .././holy-core/lib/backtrace.c \
     .././holy-core/lib/cmdline.c \
     .././holy-core/lib/cmos_datetime.c \
     .././holy-core/lib/crc.c \
     .././holy-core/lib/crc64.c \
     .././holy-core/lib/crypto.c \
     .././holy-core/lib/disk.c \
     .././holy-core/lib/division.c \
     .././holy-core/lib/efi/datetime.c \
     .././holy-core/lib/efi/halt.c \
     .././holy-core/lib/efi/reboot.c \
     .././holy-core/lib/efi/relocator.c \
     .././holy-core/lib/emu/halt.c \
     .././holy-core/lib/envblk.c \
     .././holy-core/lib/fake_module.c \
     .././holy-core/lib/fdt.c \
     .././holy-core/lib/getline.c \
     .././holy-core/lib/gpt.c \
     .././holy-core/lib/hexdump.c \
     .././holy-core/lib/i386/backtrace.c \
     .././holy-core/lib/i386/halt.c \
     .././holy-core/lib/i386/pc/biosnum.c \
     .././holy-core/lib/i386/pc/vesa_modes_table.c \
     .././holy-core/lib/i386/random.c \
     .././holy-core/lib/i386/reboot.c \
     .././holy-core/lib/i386/relocator.c \
     .././holy-core/lib/i386/relocator_common_c.c \
     .././holy-core/lib/ieee1275/cmos.c \
     .././holy-core/lib/ieee1275/datetime.c \
     .././holy-core/lib/ieee1275/halt.c \
     .././holy-core/lib/ieee1275/reboot.c \
     .././holy-core/lib/ieee1275/relocator.c \
     .././holy-core/lib/legacy_parse.c \
     .././holy-core/lib/libgcrypt/cipher/ac.c \
     .././holy-core/lib/libgcrypt/cipher/arcfour.c \
     .././holy-core/lib/libgcrypt/cipher/bithelp.h \
     .././holy-core/lib/libgcrypt/cipher/blowfish.c \
     .././holy-core/lib/libgcrypt/cipher/camellia-glue.c \
     .././holy-core/lib/libgcrypt/cipher/camellia.c \
     .././holy-core/lib/libgcrypt/cipher/camellia.h \
     .././holy-core/lib/libgcrypt/cipher/cast5.c \
     .././holy-core/lib/libgcrypt/cipher/cipher.c \
     .././holy-core/lib/libgcrypt/cipher/crc.c \
     .././holy-core/lib/libgcrypt/cipher/des.c \
     .././holy-core/lib/libgcrypt/cipher/dsa.c \
     .././holy-core/lib/libgcrypt/cipher/ecc.c \
     .././holy-core/lib/libgcrypt/cipher/elgamal.c \
     .././holy-core/lib/libgcrypt/cipher/hash-common.c \
     .././holy-core/lib/libgcrypt/cipher/hash-common.h \
     .././holy-core/lib/libgcrypt/cipher/hmac-tests.c \
     .././holy-core/lib/libgcrypt/cipher/idea.c \
     .././holy-core/lib/libgcrypt/cipher/kdf.c \
     .././holy-core/lib/libgcrypt/cipher/md.c \
     .././holy-core/lib/libgcrypt/cipher/md4.c \
     .././holy-core/lib/libgcrypt/cipher/md5.c \
     .././holy-core/lib/libgcrypt/cipher/primegen.c \
     .././holy-core/lib/libgcrypt/cipher/pubkey.c \
     .././holy-core/lib/libgcrypt/cipher/rfc2268.c \
     .././holy-core/lib/libgcrypt/cipher/rijndael-tables.h \
     .././holy-core/lib/libgcrypt/cipher/rijndael.c \
     .././holy-core/lib/libgcrypt/cipher/rmd.h \
     .././holy-core/lib/libgcrypt/cipher/rmd160.c \
     .././holy-core/lib/libgcrypt/cipher/rsa.c \
     .././holy-core/lib/libgcrypt/cipher/seed.c \
     .././holy-core/lib/libgcrypt/cipher/serpent.c \
     .././holy-core/lib/libgcrypt/cipher/sha1.c \
     .././holy-core/lib/libgcrypt/cipher/sha256.c \
     .././holy-core/lib/libgcrypt/cipher/sha512.c \
     .././holy-core/lib/libgcrypt/cipher/test-getrusage.c \
     .././holy-core/lib/libgcrypt/cipher/tiger.c \
     .././holy-core/lib/libgcrypt/cipher/twofish.c \
     .././holy-core/lib/libgcrypt/cipher/whirlpool.c \
     .././holy-core/lib/libgcrypt/mpi/amd64/mpi-asm-defs.h \
     .././holy-core/lib/libgcrypt/mpi/ec.c \
     .././holy-core/lib/libgcrypt/mpi/generic/mpi-asm-defs.h \
     .././holy-core/lib/libgcrypt/mpi/generic/mpih-add1.c \
     .././holy-core/lib/libgcrypt/mpi/generic/mpih-lshift.c \
     .././holy-core/lib/libgcrypt/mpi/generic/mpih-mul1.c \
     .././holy-core/lib/libgcrypt/mpi/generic/mpih-mul2.c \
     .././holy-core/lib/libgcrypt/mpi/generic/mpih-mul3.c \
     .././holy-core/lib/libgcrypt/mpi/generic/mpih-rshift.c \
     .././holy-core/lib/libgcrypt/mpi/generic/mpih-sub1.c \
     .././holy-core/lib/libgcrypt/mpi/generic/udiv-w-sdiv.c \
     .././holy-core/lib/libgcrypt/mpi/i386/syntax.h \
     .././holy-core/lib/libgcrypt/mpi/longlong.h \
     .././holy-core/lib/libgcrypt/mpi/m68k/syntax.h \
     .././holy-core/lib/libgcrypt/mpi/mips3/mpi-asm-defs.h \
     .././holy-core/lib/libgcrypt/mpi/mpi-add.c \
     .././holy-core/lib/libgcrypt/mpi/mpi-bit.c \
     .././holy-core/lib/libgcrypt/mpi/mpi-cmp.c \
     .././holy-core/lib/libgcrypt/mpi/mpi-div.c \
     .././holy-core/lib/libgcrypt/mpi/mpi-gcd.c \
     .././holy-core/lib/libgcrypt/mpi/mpi-inline.c \
     .././holy-core/lib/libgcrypt/mpi/mpi-inline.h \
     .././holy-core/lib/libgcrypt/mpi/mpi-internal.h \
     .././holy-core/lib/libgcrypt/mpi/mpi-inv.c \
     .././holy-core/lib/libgcrypt/mpi/mpi-mod.c \
     .././holy-core/lib/libgcrypt/mpi/mpi-mpow.c \
     .././holy-core/lib/libgcrypt/mpi/mpi-mul.c \
     .././holy-core/lib/libgcrypt/mpi/mpi-pow.c \
     .././holy-core/lib/libgcrypt/mpi/mpi-scan.c \
     .././holy-core/lib/libgcrypt/mpi/mpicoder.c \
     .././holy-core/lib/libgcrypt/mpi/mpih-div.c \
     .././holy-core/lib/libgcrypt/mpi/mpih-mul.c \
     .././holy-core/lib/libgcrypt/mpi/mpiutil.c \
     .././holy-core/lib/libgcrypt/mpi/powerpc32/syntax.h \
     .././holy-core/lib/libgcrypt/src/ath.c \
     .././holy-core/lib/libgcrypt/src/ath.h \
     .././holy-core/lib/libgcrypt/src/cipher-proto.h \
     .././holy-core/lib/libgcrypt/src/cipher.h \
     .././holy-core/lib/libgcrypt/src/dumpsexp.c \
     .././holy-core/lib/libgcrypt/src/fips.c \
     .././holy-core/lib/libgcrypt/src/g10lib.h \
     .././holy-core/lib/libgcrypt/src/gcrypt-module.h \
     .././holy-core/lib/libgcrypt/src/gcryptrnd.c \
     .././holy-core/lib/libgcrypt/src/getrandom.c \
     .././holy-core/lib/libgcrypt/src/hmac256.c \
     .././holy-core/lib/libgcrypt/src/hmac256.h \
     .././holy-core/lib/libgcrypt/src/hwfeatures.c \
     .././holy-core/lib/libgcrypt/src/missing-string.c \
     .././holy-core/lib/libgcrypt/src/module.c \
     .././holy-core/lib/libgcrypt/src/mpi.h \
     .././holy-core/lib/libgcrypt/src/secmem.h \
     .././holy-core/lib/libgcrypt/src/sexp.c \
     .././holy-core/lib/libgcrypt/src/stdmem.c \
     .././holy-core/lib/libgcrypt/src/stdmem.h \
     .././holy-core/lib/libgcrypt/src/types.h \
     .././holy-core/lib/libgcrypt/src/visibility.c \
     .././holy-core/lib/libgcrypt/src/visibility.h \
     .././holy-core/lib/libgcrypt_wrap/cipher_wrap.h \
     .././holy-core/lib/libgcrypt_wrap/mem.c \
     .././holy-core/lib/minilzo/lzoconf.h \
     .././holy-core/lib/minilzo/lzodefs.h \
     .././holy-core/lib/minilzo/minilzo.c \
     .././holy-core/lib/minilzo/minilzo.h \
     .././holy-core/lib/mips/arc/reboot.c \
     .././holy-core/lib/mips/loongson/reboot.c \
     .././holy-core/lib/mips/qemu_mips/reboot.c \
     .././holy-core/lib/mips/relocator.c \
     .././holy-core/lib/pbkdf2.c \
     .././holy-core/lib/posix_wrap/assert.h \
     .././holy-core/lib/posix_wrap/ctype.h \
     .././holy-core/lib/posix_wrap/errno.h \
     .././holy-core/lib/posix_wrap/inttypes.h \
     .././holy-core/lib/posix_wrap/langinfo.h \
     .././holy-core/lib/posix_wrap/limits.h \
     .././holy-core/lib/posix_wrap/localcharset.h \
     .././holy-core/lib/posix_wrap/locale.h \
     .././holy-core/lib/posix_wrap/stdint.h \
     .././holy-core/lib/posix_wrap/stdio.h \
     .././holy-core/lib/posix_wrap/stdlib.h \
     .././holy-core/lib/posix_wrap/string.h \
     .././holy-core/lib/posix_wrap/sys/types.h \
     .././holy-core/lib/posix_wrap/unistd.h \
     .././holy-core/lib/posix_wrap/wchar.h \
     .././holy-core/lib/posix_wrap/wctype.h \
     .././holy-core/lib/powerpc/relocator.c \
     .././holy-core/lib/priority_queue.c \
     .././holy-core/lib/progress.c \
     .././holy-core/lib/random.c \
     .././holy-core/lib/reed_solomon.c \
     .././holy-core/lib/relocator.c \
     .././holy-core/lib/syslinux_parse.c \
     .././holy-core/lib/uboot/datetime.c \
     .././holy-core/lib/uboot/halt.c \
     .././holy-core/lib/uboot/reboot.c \
     .././holy-core/lib/x86_64/efi/relocator.c \
     .././holy-core/lib/xen/datetime.c \
     .././holy-core/lib/xen/halt.c \
     .././holy-core/lib/xen/reboot.c \
     .././holy-core/lib/xen/relocator.c \
     .././holy-core/lib/xzembed/xz.h \
     .././holy-core/lib/xzembed/xz_config.h \
     .././holy-core/lib/xzembed/xz_dec_bcj.c \
     .././holy-core/lib/xzembed/xz_dec_lzma2.c \
     .././holy-core/lib/xzembed/xz_dec_stream.c \
     .././holy-core/lib/xzembed/xz_lzma2.h \
     .././holy-core/lib/xzembed/xz_private.h \
     .././holy-core/lib/xzembed/xz_stream.h \
     .././holy-core/loader/aout.c \
     .././holy-core/loader/arm/linux.c \
     .././holy-core/loader/arm64/fdt.c \
     .././holy-core/loader/arm64/linux.c \
     .././holy-core/loader/arm64/xen_boot.c \
     .././holy-core/loader/efi/appleloader.c \
     .././holy-core/loader/efi/chainloader.c \
     .././holy-core/loader/i386/bsd.c \
     .././holy-core/loader/i386/bsd32.c \
     .././holy-core/loader/i386/bsd64.c \
     .././holy-core/loader/i386/bsdXX.c \
     .././holy-core/loader/i386/bsd_pagetable.c \
     .././holy-core/loader/i386/coreboot/chainloader.c \
     .././holy-core/loader/i386/efi/linux.c \
     .././holy-core/loader/i386/linux.c \
     .././holy-core/loader/i386/multiboot_mbi.c \
     .././holy-core/loader/i386/pc/chainloader.c \
     .././holy-core/loader/i386/pc/freedos.c \
     .././holy-core/loader/i386/pc/linux.c \
     .././holy-core/loader/i386/pc/ntldr.c \
     .././holy-core/loader/i386/pc/plan9.c \
     .././holy-core/loader/i386/pc/pxechainloader.c \
     .././holy-core/loader/i386/pc/truecrypt.c \
     .././holy-core/loader/i386/xen.c \
     .././holy-core/loader/i386/xen_file.c \
     .././holy-core/loader/i386/xen_file32.c \
     .././holy-core/loader/i386/xen_file64.c \
     .././holy-core/loader/i386/xen_fileXX.c \
     .././holy-core/loader/i386/xnu.c \
     .././holy-core/loader/ia64/efi/linux.c \
     .././holy-core/loader/linux.c \
     .././holy-core/loader/lzss.c \
     .././holy-core/loader/macho.c \
     .././holy-core/loader/macho32.c \
     .././holy-core/loader/macho64.c \
     .././holy-core/loader/machoXX.c \
     .././holy-core/loader/mips/linux.c \
     .././holy-core/loader/multiboot.c \
     .././holy-core/loader/multiboot_elfxx.c \
     .././holy-core/loader/multiboot_mbi2.c \
     .././holy-core/loader/powerpc/ieee1275/linux.c \
     .././holy-core/loader/sparc64/ieee1275/linux.c \
     .././holy-core/loader/xnu.c \
     .././holy-core/loader/xnu_resume.c \
     .././holy-core/mmap/efi/mmap.c \
     .././holy-core/mmap/i386/mmap.c \
     .././holy-core/mmap/i386/pc/mmap.c \
     .././holy-core/mmap/i386/uppermem.c \
     .././holy-core/mmap/mips/uppermem.c \
     .././holy-core/mmap/mmap.c \
     .././holy-core/net/arp.c \
     .././holy-core/net/bootp.c \
     .././holy-core/net/dns.c \
     .././holy-core/net/drivers/efi/efinet.c \
     .././holy-core/net/drivers/emu/emunet.c \
     .././holy-core/net/drivers/i386/pc/pxe.c \
     .././holy-core/net/drivers/ieee1275/ofnet.c \
     .././holy-core/net/drivers/uboot/ubootnet.c \
     .././holy-core/net/ethernet.c \
     .././holy-core/net/http.c \
     .././holy-core/net/icmp.c \
     .././holy-core/net/icmp6.c \
     .././holy-core/net/ip.c \
     .././holy-core/net/net.c \
     .././holy-core/net/netbuff.c \
     .././holy-core/net/tcp.c \
     .././holy-core/net/tftp.c \
     .././holy-core/net/udp.c \
     .././holy-core/normal/auth.c \
     .././holy-core/normal/autofs.c \
     .././holy-core/normal/charset.c \
     .././holy-core/normal/cmdline.c \
     .././holy-core/normal/color.c \
     .././holy-core/normal/completion.c \
     .././holy-core/normal/context.c \
     .././holy-core/normal/crypto.c \
     .././holy-core/normal/datetime.c \
     .././holy-core/normal/dyncmd.c \
     .././holy-core/normal/main.c \
     .././holy-core/normal/menu.c \
     .././holy-core/normal/menu_entry.c \
     .././holy-core/normal/menu_text.c \
     .././holy-core/normal/misc.c \
     .././holy-core/normal/term.c \
     .././holy-core/osdep/apple/getroot.c \
     .././holy-core/osdep/apple/hostdisk.c \
     .././holy-core/osdep/aros/config.c \
     .././holy-core/osdep/aros/getroot.c \
     .././holy-core/osdep/aros/hostdisk.c \
     .././holy-core/osdep/aros/relpath.c \
     .././holy-core/osdep/basic/compress.c \
     .././holy-core/osdep/basic/emunet.c \
     .././holy-core/osdep/basic/getroot.c \
     .././holy-core/osdep/basic/hostdisk.c \
     .././holy-core/osdep/basic/init.c \
     .././holy-core/osdep/basic/no_platform.c \
     .././holy-core/osdep/basic/ofpath.c \
     .././holy-core/osdep/basic/platform.c \
     .././holy-core/osdep/basic/random.c \
     .././holy-core/osdep/blocklist.c \
     .././holy-core/osdep/bsd/getroot.c \
     .././holy-core/osdep/bsd/hostdisk.c \
     .././holy-core/osdep/compress.c \
     .././holy-core/osdep/config.c \
     .././holy-core/osdep/cputime.c \
     .././holy-core/osdep/devmapper/getroot.c \
     .././holy-core/osdep/devmapper/hostdisk.c \
     .././holy-core/osdep/dl.c \
     .././holy-core/osdep/emuconsole.c \
     .././holy-core/osdep/emunet.c \
     .././holy-core/osdep/exec.c \
     .././holy-core/osdep/freebsd/getroot.c \
     .././holy-core/osdep/freebsd/hostdisk.c \
     .././holy-core/osdep/generic/blocklist.c \
     .././holy-core/osdep/getroot.c \
     .././holy-core/osdep/haiku/getroot.c \
     .././holy-core/osdep/haiku/hostdisk.c \
     .././holy-core/osdep/hostdisk.c \
     .././holy-core/osdep/hurd/getroot.c \
     .././holy-core/osdep/hurd/hostdisk.c \
     .././holy-core/osdep/init.c \
     .././holy-core/osdep/linux/blocklist.c \
     .././holy-core/osdep/linux/emunet.c \
     .././holy-core/osdep/linux/getroot.c \
     .././holy-core/osdep/linux/hostdisk.c \
     .././holy-core/osdep/linux/ofpath.c \
     .././holy-core/osdep/linux/platform.c \
     .././holy-core/osdep/ofpath.c \
     .././holy-core/osdep/password.c \
     .././holy-core/osdep/platform.c \
     .././holy-core/osdep/platform_unix.c \
     .././holy-core/osdep/random.c \
     .././holy-core/osdep/relpath.c \
     .././holy-core/osdep/sleep.c \
     .././holy-core/osdep/sun/getroot.c \
     .././holy-core/osdep/sun/hostdisk.c \
     .././holy-core/osdep/unix/compress.c \
     .././holy-core/osdep/unix/config.c \
     .././holy-core/osdep/unix/cputime.c \
     .././holy-core/osdep/unix/dl.c \
     .././holy-core/osdep/unix/emuconsole.c \
     .././holy-core/osdep/unix/exec.c \
     .././holy-core/osdep/unix/getroot.c \
     .././holy-core/osdep/unix/hostdisk.c \
     .././holy-core/osdep/unix/password.c \
     .././holy-core/osdep/unix/platform.c \
     .././holy-core/osdep/unix/random.c \
     .././holy-core/osdep/unix/relpath.c \
     .././holy-core/osdep/unix/sleep.c \
     .././holy-core/osdep/windows/blocklist.c \
     .././holy-core/osdep/windows/config.c \
     .././holy-core/osdep/windows/cputime.c \
     .././holy-core/osdep/windows/dl.c \
     .././holy-core/osdep/windows/emuconsole.c \
     .././holy-core/osdep/windows/getroot.c \
     .././holy-core/osdep/windows/hostdisk.c \
     .././holy-core/osdep/windows/init.c \
     .././holy-core/osdep/windows/password.c \
     .././holy-core/osdep/windows/platform.c \
     .././holy-core/osdep/windows/random.c \
     .././holy-core/osdep/windows/relpath.c \
     .././holy-core/osdep/windows/sleep.c \
     .././holy-core/partmap/acorn.c \
     .././holy-core/partmap/amiga.c \
     .././holy-core/partmap/apple.c \
     .././holy-core/partmap/bsdlabel.c \
     .././holy-core/partmap/dfly.c \
     .././holy-core/partmap/dvh.c \
     .././holy-core/partmap/gpt.c \
     .././holy-core/partmap/msdos.c \
     .././holy-core/partmap/plan.c \
     .././holy-core/partmap/sun.c \
     .././holy-core/partmap/sunpc.c \
     .././holy-core/parttool/msdospart.c \
     .././holy-core/script/argv.c \
     .././holy-core/script/execute.c \
     .././holy-core/script/function.c \
     .././holy-core/script/lexer.c \
     .././holy-core/script/main.c \
     .././holy-core/script/script.c \
     .././holy-core/term/arc/console.c \
     .././holy-core/term/arc/serial.c \
     .././holy-core/term/at_keyboard.c \
     .././holy-core/term/efi/console.c \
     .././holy-core/term/efi/serial.c \
     .././holy-core/term/gfxterm.c \
     .././holy-core/term/gfxterm_background.c \
     .././holy-core/term/i386/coreboot/cbmemc.c \
     .././holy-core/term/i386/pc/console.c \
     .././holy-core/term/i386/pc/mda_text.c \
     .././holy-core/term/i386/pc/vga_text.c \
     .././holy-core/term/ieee1275/console.c \
     .././holy-core/term/ieee1275/escc.c \
     .././holy-core/term/ieee1275/serial.c \
     .././holy-core/term/morse.c \
     .././holy-core/term/ns8250.c \
     .././holy-core/term/serial.c \
     .././holy-core/term/spkmodem.c \
     .././holy-core/term/terminfo.c \
     .././holy-core/term/tparm.c \
     .././holy-core/term/uboot/console.c \
     .././holy-core/term/usb_keyboard.c \
     .././holy-core/term/xen/console.c \
     .././holy-core/tests/bswap_test.c \
     .././holy-core/tests/checksums.h \
     .././holy-core/tests/cmdline_cat_test.c \
     .././holy-core/tests/cmp_test.c \
     .././holy-core/tests/ctz_test.c \
     .././holy-core/tests/div_test.c \
     .././holy-core/tests/example_functional_test.c \
     .././holy-core/tests/fake_input.c \
     .././holy-core/tests/gfxterm_menu.c \
     .././holy-core/tests/legacy_password_test.c \
     .././holy-core/tests/lib/functional_test.c \
     .././holy-core/tests/lib/test.c \
     .././holy-core/tests/mul_test.c \
     .././holy-core/tests/pbkdf2_test.c \
     .././holy-core/tests/setjmp_test.c \
     .././holy-core/tests/shift_test.c \
     .././holy-core/tests/signature_test.c \
     .././holy-core/tests/signatures.h \
     .././holy-core/tests/sleep_test.c \
     .././holy-core/tests/strtoull_test.c \
     .././holy-core/tests/test_blockarg.c \
     .././holy-core/tests/video_checksum.c \
     .././holy-core/tests/videotest_checksum.c \
     .././holy-core/tests/xnu_uuid_test.c \
     .././holy-core/unidata.c \
     .././holy-core/video/bitmap.c \
     .././holy-core/video/bitmap_scale.c \
     .././holy-core/video/bochs.c \
     .././holy-core/video/capture.c \
     .././holy-core/video/cirrus.c \
     .././holy-core/video/colors.c \
     .././holy-core/video/efi_gop.c \
     .././holy-core/video/efi_uga.c \
     .././holy-core/video/emu/sdl.c \
     .././holy-core/video/fb/fbblit.c \
     .././holy-core/video/fb/fbfill.c \
     .././holy-core/video/fb/fbutil.c \
     .././holy-core/video/fb/video_fb.c \
     .././holy-core/video/i386/coreboot/cbfb.c \
     .././holy-core/video/i386/pc/vbe.c \
     .././holy-core/video/i386/pc/vga.c \
     .././holy-core/video/ieee1275.c \
     .././holy-core/video/radeon_fuloong2e.c \
     .././holy-core/video/radeon_yeeloong3a.c \
     .././holy-core/video/readers/jpeg.c \
     .././holy-core/video/readers/png.c \
     .././holy-core/video/readers/tga.c \
     .././holy-core/video/sis315_init.c \
     .././holy-core/video/sis315pro.c \
     .././holy-core/video/sm712.c \
     .././holy-core/video/sm712_init.c \
     .././holy-core/video/video.c \
     .././include/holy/acorn_filecore.h \
     .././include/holy/acpi.h \
     .././include/holy/aout.h \
     .././include/holy/arc/arc.h \
     .././include/holy/arc/console.h \
     .././include/holy/archelp.h \
     .././include/holy/arm/efi/loader.h \
     .././include/holy/arm/efi/memory.h \
     .././include/holy/arm/linux.h \
     .././include/holy/arm/reloc.h \
     .././include/holy/arm/setjmp.h \
     .././include/holy/arm/system.h \
     .././include/holy/arm/time.h \
     .././include/holy/arm/types.h \
     .././include/holy/arm/uboot/kernel.h \
     .././include/holy/arm64/efi/memory.h \
     .././include/holy/arm64/fdtload.h \
     .././include/holy/arm64/linux.h \
     .././include/holy/arm64/reloc.h \
     .././include/holy/arm64/setjmp.h \
     .././include/holy/arm64/time.h \
     .././include/holy/arm64/types.h \
     .././include/holy/at_keyboard.h \
     .././include/holy/ata.h \
     .././include/holy/auth.h \
     .././include/holy/autoefi.h \
     .././include/holy/backtrace.h \
     .././include/holy/bitmap.h \
     .././include/holy/bitmap_scale.h \
     .././include/holy/boottime.h \
     .././include/holy/bsdlabel.h \
     .././include/holy/btrfs.h \
     .././include/holy/bufio.h \
     .././include/holy/cache.h \
     .././include/holy/cbfs_core.h \
     .././include/holy/charset.h \
     .././include/holy/cmos.h \
     .././include/holy/color.h \
     .././include/holy/command.h \
     .././include/holy/compiler-rt-emu.h \
     .././include/holy/compiler-rt.h \
     .././include/holy/compiler.h \
     .././include/holy/crypto.h \
     .././include/holy/cryptodisk.h \
     .././include/holy/cs5536.h \
     .././include/holy/datetime.h \
     .././include/holy/decompressor.h \
     .././include/holy/deflate.h \
     .././include/holy/device.h \
     .././include/holy/disk.h \
     .././include/holy/diskfilter.h \
     .././include/holy/dl.h \
     .././include/holy/efi/api.h \
     .././include/holy/efi/console.h \
     .././include/holy/efi/console_control.h \
     .././include/holy/efi/disk.h \
     .././include/holy/efi/edid.h \
     .././include/holy/efi/efi.h \
     .././include/holy/efi/graphics_output.h \
     .././include/holy/efi/memory.h \
     .././include/holy/efi/pci.h \
     .././include/holy/efi/pe32.h \
     .././include/holy/efi/tpm.h \
     .././include/holy/efi/uga_draw.h \
     .././include/holy/efiemu/efiemu.h \
     .././include/holy/efiemu/runtime.h \
     .././include/holy/elf.h \
     .././include/holy/elfload.h \
     .././include/holy/emu/config.h \
     .././include/holy/emu/console.h \
     .././include/holy/emu/exec.h \
     .././include/holy/emu/getroot.h \
     .././include/holy/emu/hostdisk.h \
     .././include/holy/emu/hostfile.h \
     .././include/holy/emu/misc.h \
     .././include/holy/emu/net.h \
     .././include/holy/env.h \
     .././include/holy/env_private.h \
     .././include/holy/err.h \
     .././include/holy/exfat.h \
     .././include/holy/extcmd.h \
     .././include/holy/fat.h \
     .././include/holy/fbblit.h \
     .././include/holy/fbfill.h \
     .././include/holy/fbutil.h \
     .././include/holy/fdt.h \
     .././include/holy/file.h \
     .././include/holy/fileid.h \
     .././include/holy/font.h \
     .././include/holy/fontformat.h \
     .././include/holy/fs.h \
     .././include/holy/fshelp.h \
     .././include/holy/gcry/types.h \
     .././include/holy/gcrypt/g10lib.h \
     .././include/holy/gcrypt/gcrypt.h \
     .././include/holy/gcrypt/gpg-error.h \
     .././include/holy/gdb.h \
     .././include/holy/gfxmenu_model.h \
     .././include/holy/gfxmenu_view.h \
     .././include/holy/gfxterm.h \
     .././include/holy/gfxwidgets.h \
     .././include/holy/gpt_partition.h \
     .././include/holy/gui.h \
     .././include/holy/gui_string_util.h \
     .././include/holy/hfs.h \
     .././include/holy/hfsplus.h \
     .././include/holy/i18n.h \
     .././include/holy/i386/at_keyboard.h \
     .././include/holy/i386/bsd.h \
     .././include/holy/i386/cmos.h \
     .././include/holy/i386/coreboot/boot.h \
     .././include/holy/i386/coreboot/console.h \
     .././include/holy/i386/coreboot/lbio.h \
     .././include/holy/i386/coreboot/memory.h \
     .././include/holy/i386/coreboot/serial.h \
     .././include/holy/i386/coreboot/time.h \
     .././include/holy/i386/cpuid.h \
     .././include/holy/i386/efi/memory.h \
     .././include/holy/i386/efi/serial.h \
     .././include/holy/i386/efiemu.h \
     .././include/holy/i386/floppy.h \
     .././include/holy/i386/freebsd_linker.h \
     .././include/holy/i386/freebsd_reboot.h \
     .././include/holy/i386/gdb.h \
     .././include/holy/i386/ieee1275/ieee1275.h \
     .././include/holy/i386/ieee1275/memory.h \
     .././include/holy/i386/ieee1275/serial.h \
     .././include/holy/i386/io.h \
     .././include/holy/i386/linux.h \
     .././include/holy/i386/macho.h \
     .././include/holy/i386/memory.h \
     .././include/holy/i386/memory_raw.h \
     .././include/holy/i386/multiboot.h \
     .././include/holy/i386/multiboot/boot.h \
     .././include/holy/i386/multiboot/console.h \
     .././include/holy/i386/multiboot/kernel.h \
     .././include/holy/i386/multiboot/memory.h \
     .././include/holy/i386/multiboot/serial.h \
     .././include/holy/i386/multiboot/time.h \
     .././include/holy/i386/netbsd_bootinfo.h \
     .././include/holy/i386/netbsd_reboot.h \
     .././include/holy/i386/openbsd_bootarg.h \
     .././include/holy/i386/openbsd_reboot.h \
     .././include/holy/i386/pc/apm.h \
     .././include/holy/i386/pc/biosdisk.h \
     .././include/holy/i386/pc/biosnum.h \
     .././include/holy/i386/pc/boot.h \
     .././include/holy/i386/pc/chainloader.h \
     .././include/holy/i386/pc/console.h \
     .././include/holy/i386/pc/int.h \
     .././include/holy/i386/pc/kernel.h \
     .././include/holy/i386/pc/memory.h \
     .././include/holy/i386/pc/pxe.h \
     .././include/holy/i386/pc/time.h \
     .././include/holy/i386/pc/vbe.h \
     .././include/holy/i386/pc/vesa_modes_table.h \
     .././include/holy/i386/pci.h \
     .././include/holy/i386/pit.h \
     .././include/holy/i386/pmtimer.h \
     .././include/holy/i386/qemu/boot.h \
     .././include/holy/i386/qemu/console.h \
     .././include/holy/i386/qemu/kernel.h \
     .././include/holy/i386/qemu/memory.h \
     .././include/holy/i386/qemu/serial.h \
     .././include/holy/i386/qemu/time.h \
     .././include/holy/i386/reboot.h \
     .././include/holy/i386/relocator.h \
     .././include/holy/i386/relocator_private.h \
     .././include/holy/i386/setjmp.h \
     .././include/holy/i386/time.h \
     .././include/holy/i386/tsc.h \
     .././include/holy/i386/types.h \
     .././include/holy/i386/xen/hypercall.h \
     .././include/holy/i386/xen/memory.h \
     .././include/holy/i386/xnu.h \
     .././include/holy/ia64/efi/memory.h \
     .././include/holy/ia64/efi/time.h \
     .././include/holy/ia64/kernel.h \
     .././include/holy/ia64/reloc.h \
     .././include/holy/ia64/setjmp.h \
     .././include/holy/ia64/time.h \
     .././include/holy/ia64/types.h \
     .././include/holy/icon_manager.h \
     .././include/holy/ieee1275/console.h \
     .././include/holy/ieee1275/ieee1275.h \
     .././include/holy/ieee1275/ofdisk.h \
     .././include/holy/kernel.h \
     .././include/holy/keyboard_layouts.h \
     .././include/holy/legacy_parse.h \
     .././include/holy/lib/LzFind.h \
     .././include/holy/lib/LzHash.h \
     .././include/holy/lib/LzmaDec.h \
     .././include/holy/lib/LzmaEnc.h \
     .././include/holy/lib/LzmaTypes.h \
     .././include/holy/lib/arg.h \
     .././include/holy/lib/cmdline.h \
     .././include/holy/lib/crc.h \
     .././include/holy/lib/envblk.h \
     .././include/holy/lib/hexdump.h \
     .././include/holy/libpciaccess.h \
     .././include/holy/linux.h \
     .././include/holy/list.h \
     .././include/holy/loader.h \
     .././include/holy/lvm.h \
     .././include/holy/macho.h \
     .././include/holy/machoload.h \
     .././include/holy/memory.h \
     .././include/holy/menu.h \
     .././include/holy/menu_viewer.h \
     .././include/holy/mips/arc/kernel.h \
     .././include/holy/mips/arc/memory.h \
     .././include/holy/mips/arc/time.h \
     .././include/holy/mips/asm.h \
     .././include/holy/mips/at_keyboard.h \
     .././include/holy/mips/cmos.h \
     .././include/holy/mips/io.h \
     .././include/holy/mips/kernel.h \
     .././include/holy/mips/loongson.h \
     .././include/holy/mips/loongson/at_keyboard.h \
     .././include/holy/mips/loongson/cmos.h \
     .././include/holy/mips/loongson/ec.h \
     .././include/holy/mips/loongson/kernel.h \
     .././include/holy/mips/loongson/memory.h \
     .././include/holy/mips/loongson/pci.h \
     .././include/holy/mips/loongson/serial.h \
     .././include/holy/mips/loongson/time.h \
     .././include/holy/mips/memory.h \
     .././include/holy/mips/mips.h \
     .././include/holy/mips/multiboot.h \
     .././include/holy/mips/pci.h \
     .././include/holy/mips/qemu_mips/at_keyboard.h \
     .././include/holy/mips/qemu_mips/cmos.h \
     .././include/holy/mips/qemu_mips/console.h \
     .././include/holy/mips/qemu_mips/kernel.h \
     .././include/holy/mips/qemu_mips/loader.h \
     .././include/holy/mips/qemu_mips/memory.h \
     .././include/holy/mips/qemu_mips/serial.h \
     .././include/holy/mips/qemu_mips/time.h \
     .././include/holy/mips/relocator.h \
     .././include/holy/mips/setjmp.h \
     .././include/holy/mips/time.h \
     .././include/holy/mips/types.h \
     .././include/holy/misc.h \
     .././include/holy/mm.h \
     .././include/holy/mm_private.h \
     .././include/holy/module_verifier.h \
     .././include/holy/msdos_partition.h \
     .././include/holy/multiboot.h \
     .././include/holy/multiboot_loader.h \
     .././include/holy/net.h \
     .././include/holy/net/arp.h \
     .././include/holy/net/ethernet.h \
     .././include/holy/net/ip.h \
     .././include/holy/net/netbuff.h \
     .././include/holy/net/tcp.h \
     .././include/holy/net/udp.h \
     .././include/holy/normal.h \
     .././include/holy/ns8250.h \
     .././include/holy/ntfs.h \
     .././include/holy/offsets.h \
     .././include/holy/osdep/hostfile.h \
     .././include/holy/osdep/hostfile_aros.h \
     .././include/holy/osdep/hostfile_unix.h \
     .././include/holy/osdep/hostfile_windows.h \
     .././include/holy/parser.h \
     .././include/holy/partition.h \
     .././include/holy/parttool.h \
     .././include/holy/pci.h \
     .././include/holy/pciutils.h \
     .././include/holy/powerpc/ieee1275/ieee1275.h \
     .././include/holy/powerpc/kernel.h \
     .././include/holy/powerpc/memory.h \
     .././include/holy/powerpc/relocator.h \
     .././include/holy/powerpc/setjmp.h \
     .././include/holy/powerpc/time.h \
     .././include/holy/powerpc/types.h \
     .././include/holy/priority_queue.h \
     .././include/holy/procfs.h \
     .././include/holy/pubkey.h \
     .././include/holy/random.h \
     .././include/holy/reader.h \
     .././include/holy/reed_solomon.h \
     .././include/holy/relocator.h \
     .././include/holy/relocator_private.h \
     .././include/holy/script_sh.h \
     .././include/holy/scsi.h \
     .././include/holy/scsicmd.h \
     .././include/holy/sdl.h \
     .././include/holy/search.h \
     .././include/holy/serial.h \
     .././include/holy/setjmp.h \
     .././include/holy/smbios.h \
     .././include/holy/smbus.h \
     .././include/holy/sparc64/ieee1275/boot.h \
     .././include/holy/sparc64/ieee1275/ieee1275.h \
     .././include/holy/sparc64/ieee1275/kernel.h \
     .././include/holy/sparc64/setjmp.h \
     .././include/holy/sparc64/time.h \
     .././include/holy/sparc64/types.h \
     .././include/holy/speaker.h \
     .././include/holy/symbol.h \
     .././include/holy/syslinux_parse.h \
     .././include/holy/term.h \
     .././include/holy/terminfo.h \
     .././include/holy/test.h \
     .././include/holy/time.h \
     .././include/holy/tparm.h \
     .././include/holy/tpm.h \
     .././include/holy/trig.h \
     .././include/holy/types.h \
     .././include/holy/uboot/api_public.h \
     .././include/holy/uboot/console.h \
     .././include/holy/uboot/disk.h \
     .././include/holy/uboot/image.h \
     .././include/holy/uboot/uboot.h \
     .././include/holy/udf.h \
     .././include/holy/unicode.h \
     .././include/holy/usb.h \
     .././include/holy/usbdesc.h \
     .././include/holy/usbserial.h \
     .././include/holy/usbtrans.h \
     .././include/holy/util/install.h \
     .././include/holy/util/libnvpair.h \
     .././include/holy/util/libzfs.h \
     .././include/holy/util/misc.h \
     .././include/holy/util/mkimage.h \
     .././include/holy/util/ofpath.h \
     .././include/holy/util/resolve.h \
     .././include/holy/util/windows.h \
     .././include/holy/verity-hash.h \
     .././include/holy/vga.h \
     .././include/holy/vgaregs.h \
     .././include/holy/video.h \
     .././include/holy/video_fb.h \
     .././include/holy/x86_64/at_keyboard.h \
     .././include/holy/x86_64/efi/boot.h \
     .././include/holy/x86_64/efi/loader.h \
     .././include/holy/x86_64/efi/memory.h \
     .././include/holy/x86_64/efi/serial.h \
     .././include/holy/x86_64/io.h \
     .././include/holy/x86_64/linux.h \
     .././include/holy/x86_64/macho.h \
     .././include/holy/x86_64/memory.h \
     .././include/holy/x86_64/multiboot.h \
     .././include/holy/x86_64/pci.h \
     .././include/holy/x86_64/relocator.h \
     .././include/holy/x86_64/setjmp.h \
     .././include/holy/x86_64/time.h \
     .././include/holy/x86_64/types.h \
     .././include/holy/x86_64/xen/hypercall.h \
     .././include/holy/x86_64/xnu.h \
     .././include/holy/xen.h \
     .././include/holy/xen/relocator.h \
     .././include/holy/xen_file.h \
     .././include/holy/xnu.h \
     .././include/holy/zfs/dmu.h \
     .././include/holy/zfs/dmu_objset.h \
     .././include/holy/zfs/dnode.h \
     .././include/holy/zfs/dsl_dataset.h \
     .././include/holy/zfs/dsl_dir.h \
     .././include/holy/zfs/sa_impl.h \
     .././include/holy/zfs/spa.h \
     .././include/holy/zfs/uberblock_impl.h \
     .././include/holy/zfs/vdev_impl.h \
     .././include/holy/zfs/zap_impl.h \
     .././include/holy/zfs/zap_leaf.h \
     .././include/holy/zfs/zfs.h \
     .././include/holy/zfs/zfs_acl.h \
     .././include/holy/zfs/zfs_znode.h \
     .././include/holy/zfs/zil.h \
     .././include/holy/zfs/zio.h \
     .././include/holy/zfs/zio_checksum.h \
     .././include/multiboot.h \
     .././include/multiboot2.h \
     .././include/xen/arch-x86/xen-x86_32.h \
     .././include/xen/arch-x86/xen-x86_64.h \
     .././include/xen/arch-x86/xen.h \
     .././include/xen/elfnote.h \
     .././include/xen/event_channel.h \
     .././include/xen/grant_table.h \
     .././include/xen/io/blkif.h \
     .././include/xen/io/console.h \
     .././include/xen/io/protocols.h \
     .././include/xen/io/ring.h \
     .././include/xen/io/xenbus.h \
     .././include/xen/io/xs_wire.h \
     .././include/xen/sched.h \
     .././include/xen/xen-compat.h \
     .././include/xen/xen.h \
     .././tests/cmp_unit_test.c \
     .././tests/date_unit_test.c \
     .././tests/example_unit_test.c \
     .././tests/gpt_unit_test.c \
     .././tests/lib/unit_test.c \
     .././tests/printf_unit_test.c \
     .././util/bin2h.c \
     .././util/config.c \
     .././util/editenv.c \
     .././util/garbage-gen.c \
     .././util/getroot.c \
     .././util/glue-efi.c \
     .././util/holy-editenv.c \
     .././util/holy-file.c \
     .././util/holy-fstest.c \
     .././util/holy-glue-efi.c \
     .././util/holy-install-common.c \
     .././util/holy-install.c \
     .././util/holy-macbless.c \
     .././util/holy-macho2img.c \
     .././util/holy-menulst2cfg.c \
     .././util/holy-mkfont.c \
     .././util/holy-mkimage.c \
     .././util/holy-mkimage32.c \
     .././util/holy-mkimage64.c \
     .././util/holy-mkimagexx.c \
     .././util/holy-mklayout.c \
     .././util/holy-mknetdir.c \
     .././util/holy-mkpasswd-pbkdf2.c \
     .././util/holy-mkrelpath.c \
     .././util/holy-mkrescue.c \
     .././util/holy-mkstandalone.c \
     .././util/holy-module-verifier.c \
     .././util/holy-module-verifier32.c \
     .././util/holy-module-verifier64.c \
     .././util/holy-module-verifierXX.c \
     .././util/holy-mount.c \
     .././util/holy-pe2elf.c \
     .././util/holy-probe.c \
     .././util/holy-render-label.c \
     .././util/holy-script-check.c \
     .././util/holy-setup.c \
     .././util/holy-syslinux2cfg.c \
     .././util/ieee1275/holy-ofpathname.c \
     .././util/misc.c \
     .././util/mkimage.c \
     .././util/probe.c \
     .././util/render-label.c \
     .././util/resolve.c \
     .././util/setup.c \
     .././util/setup_bios.c \
     .././util/setup_sparc.c \
     .././util/spkmodem-recv.c

CATALOGS = 

# Makevars gets inserted here. (Don't remove this line!)
# Makefile variables for PO directory in any package using GNU gettext.

# Usually the message domain is the same as the package name.
DOMAIN = $(PACKAGE)

# These two variables depend on the location of this directory.
subdir = po
top_builddir = ..

# These options get passed to xgettext.
XGETTEXT_OPTIONS = --keyword=_ --keyword=N_

COPYRIGHT_HOLDER = holybooter

MSGID_BUGS_ADDRESS = bashscripter123@gmail.com

# This is the list of locale categories, beyond LC_MESSAGES, for which the
# message catalogs shall be used.  It is usually empty.
EXTRA_LOCALE_CATEGORIES =

.SUFFIXES:
.SUFFIXES: .po .gmo .mo .sed .sin .nop .po-create .po-update

.po.mo:
	@echo "$(MSGFMT) -c -o $@ $<"; \
	$(MSGFMT) --endianness=little -c -o t-$@ $< && mv t-$@ $@

.po.gmo:
	@lang=`echo $* | sed -e 's,.*/,,'`; \
	test "$(srcdir)" = . && cdcmd="" || cdcmd="cd $(srcdir) && "; \
	echo "$${cdcmd}rm -f $${lang}.gmo && $(GMSGFMT) --endianness=little -c --statistics --verbose -o $${lang}.gmo $${lang}.po"; \
	cd $(srcdir) && rm -f $${lang}.gmo && $(GMSGFMT) --endianness=little -c --statistics --verbose -o t-$${lang}.gmo $${lang}.po && mv t-$${lang}.gmo $${lang}.gmo

.sin.sed:
	sed -e '/^#/d' $< > t-$@
	mv t-$@ $@


all: check-macro-version all-yes

all-yes: stamp-po
all-no:

# Ensure that the gettext macros and this Makefile.in.in are in sync.
check-macro-version:
	@test "$(GETTEXT_MACRO_VERSION)" = "0.18" \
	  || { echo "*** error: gettext infrastructure mismatch: using a Makefile.in.in from gettext version $(GETTEXT_MACRO_VERSION) but the autoconf macros are from gettext version 0.18" 1>&2; \
	       exit 1; \
	     }

# $(srcdir)/$(DOMAIN).pot is only created when needed. When xgettext finds no
# internationalized messages, no $(srcdir)/$(DOMAIN).pot is created (because
# we don't want to bother translators with empty POT files). We assume that
# LINGUAS is empty in this case, i.e. $(POFILES) and $(GMOFILES) are empty.
# In this case, stamp-po is a nop (i.e. a phony target).

# stamp-po is a timestamp denoting the last time at which the CATALOGS have
# been loosely updated. Its purpose is that when a developer or translator
# checks out the package via CVS, and the $(DOMAIN).pot file is not in CVS,
# "make" will update the $(DOMAIN).pot and the $(CATALOGS), but subsequent
# invocations of "make" will do nothing. This timestamp would not be necessary
# if updating the $(CATALOGS) would always touch them; however, the rule for
# $(POFILES) has been designed to not touch files that don't need to be
# changed.
stamp-po: $(srcdir)/$(DOMAIN).pot
	test ! -f $(srcdir)/$(DOMAIN).pot || \
	  test -z "$(GMOFILES)" || $(MAKE) $(GMOFILES)
	@test ! -f $(srcdir)/$(DOMAIN).pot || { \
	  echo "touch stamp-po" && \
	  echo timestamp > stamp-poT && \
	  mv stamp-poT stamp-po; \
	}

# Note: Target 'all' must not depend on target '$(DOMAIN).pot-update',
# otherwise packages like GCC can not be built if only parts of the source
# have been downloaded.

# This target rebuilds $(DOMAIN).pot; it is an expensive operation.
# Note that $(DOMAIN).pot is not touched if it doesn't need to be changed.
$(DOMAIN).pot-update: $(POTFILES) $(srcdir)/POTFILES.in $(srcdir)/POTFILES-shell.in remove-potcdate.sed
	if LC_ALL=C grep 'GNU holy' $(top_srcdir)/* 2>/dev/null | grep -v 'libtool:' >/dev/null; then \
	  package_gnu='GNU '; \
	else \
	  package_gnu=''; \
	fi; \
	if test -n '$(MSGID_BUGS_ADDRESS)' || test '$(PACKAGE_BUGREPORT)' = '@'PACKAGE_BUGREPORT'@'; then \
	  msgid_bugs_address='$(MSGID_BUGS_ADDRESS)'; \
	else \
	  msgid_bugs_address='$(PACKAGE_BUGREPORT)'; \
	fi; \
	case `$(XGETTEXT) --version | sed 1q | sed -e 's,^[^0-9]*,,'` in \
	  '' | 0.[0-9] | 0.[0-9].* | 0.1[0-5] | 0.1[0-5].* | 0.16 | 0.16.[0-1]*) \
	    $(XGETTEXT) --default-domain=$(DOMAIN) --directory=$(top_srcdir) \
	      --add-comments=TRANSLATORS: $(XGETTEXT_OPTIONS)  --flag=argp_error:2:c-format --flag=argp_failure:4:c-format --flag=error:3:c-format --flag=error_at_line:5:c-format \
	      --files-from=$(srcdir)/POTFILES.in \
	      --copyright-holder='$(COPYRIGHT_HOLDER)' \
	      --msgid-bugs-address="$$msgid_bugs_address" \
	    ;; \
	  *) \
	    $(XGETTEXT) --default-domain=$(DOMAIN) --directory=$(top_srcdir) \
	      --add-comments=TRANSLATORS: $(XGETTEXT_OPTIONS)  --flag=argp_error:2:c-format --flag=argp_failure:4:c-format --flag=error:3:c-format --flag=error_at_line:5:c-format \
	      --files-from=$(srcdir)/POTFILES.in \
	      --copyright-holder='$(COPYRIGHT_HOLDER)' \
	      --package-name="$${package_gnu}holy" \
	      --package-version='2.02' \
	      --msgid-bugs-address="$$msgid_bugs_address" \
	    ;; \
	esac; \
	case `$(XGETTEXT) --version | sed 1q | sed -e 's,^[^0-9]*,,'` in \
	  '' | 0.[0-9] | 0.[0-9].* | 0.1[0-5] | 0.1[0-5].* | 0.16 | 0.16.[0-1]*) \
	    $(XGETTEXT) --default-domain=$(DOMAIN) --directory=$(top_srcdir) \
	      --add-comments=TRANSLATORS:  --flag=argp_error:2:c-format --flag=argp_failure:4:c-format --flag=error:3:c-format --flag=error_at_line:5:c-format \
	      --files-from=$(srcdir)/POTFILES-shell.in \
	      --copyright-holder='$(COPYRIGHT_HOLDER)' \
	      --msgid-bugs-address="$$msgid_bugs_address" \
	      --join-existing --language=Shell \
	      --keyword=gettext_quoted --keyword=gettext_printf \
	    ;; \
	  *) \
	    $(XGETTEXT) --default-domain=$(DOMAIN) --directory=$(top_srcdir) \
	      --add-comments=TRANSLATORS:  --flag=argp_error:2:c-format --flag=argp_failure:4:c-format --flag=error:3:c-format --flag=error_at_line:5:c-format \
	      --files-from=$(srcdir)/POTFILES-shell.in \
	      --copyright-holder='$(COPYRIGHT_HOLDER)' \
	      --package-name="$${package_gnu}holy" \
	      --package-version='2.02' \
	      --msgid-bugs-address="$$msgid_bugs_address" \
	      --join-existing --language=Shell \
	      --keyword=gettext_quoted --keyword=gettext_printf \
	    ;; \
	esac ; \
	test ! -f $(DOMAIN).po || { \
	  sed -f holy.d.sed < $(DOMAIN).po > $(DOMAIN).1po && \
	  mv $(DOMAIN).1po $(DOMAIN).po; \
	  if test -f $(srcdir)/$(DOMAIN).pot; then \
	    sed -f remove-potcdate.sed < $(srcdir)/$(DOMAIN).pot > $(DOMAIN).1po && \
	    sed -f remove-potcdate.sed < $(DOMAIN).po > $(DOMAIN).2po && \
	    if cmp $(DOMAIN).1po $(DOMAIN).2po >/dev/null 2>&1; then \
	      rm -f $(DOMAIN).1po $(DOMAIN).2po $(DOMAIN).po; \
	    else \
	      rm -f $(DOMAIN).1po $(DOMAIN).2po $(srcdir)/$(DOMAIN).pot && \
	      mv $(DOMAIN).po $(srcdir)/$(DOMAIN).pot; \
	    fi; \
	  else \
	    mv $(DOMAIN).po $(srcdir)/$(DOMAIN).pot; \
	  fi; \
	}

# This rule has no dependencies: we don't need to update $(DOMAIN).pot at
# every "make" invocation, only create it when it is missing.
# Only "make $(DOMAIN).pot-update" or "make dist" will force an update.
$(srcdir)/$(DOMAIN).pot:
	$(MAKE) $(DOMAIN).pot-update

# This target rebuilds a PO file if $(DOMAIN).pot has changed.
# Note that a PO file is not touched if it doesn't need to be changed.
$(POFILES): $(srcdir)/$(DOMAIN).pot
	@lang=`echo $@ | sed -e 's,.*/,,' -e 's/\.po$$//'`; \
	if test -f "$(srcdir)/$${lang}.po"; then \
	  test "$(srcdir)" = . && cdcmd="" || cdcmd="cd $(srcdir) && "; \
	  echo "$${cdcmd}$(MSGMERGE_UPDATE) $(MSGMERGE_OPTIONS) --lang=$${lang} $${lang}.po $(DOMAIN).pot"; \
	  cd $(srcdir) \
	    && { case `$(MSGMERGE_UPDATE) --version | sed 1q | sed -e 's,^[^0-9]*,,'` in \
	           '' | 0.[0-9] | 0.[0-9].* | 0.1[0-7] | 0.1[0-7].*) \
	             $(MSGMERGE_UPDATE) $(MSGMERGE_OPTIONS) $${lang}.po $(DOMAIN).pot;; \
	           *) \
	             $(MSGMERGE_UPDATE) $(MSGMERGE_OPTIONS) --lang=$${lang} $${lang}.po $(DOMAIN).pot;; \
	         esac; \
	       }; \
	else \
	  $(MAKE) $${lang}.po-create; \
	fi


install: install-exec install-data
install-exec:
install-data: install-data-yes
	if test "$(PACKAGE)" = "gettext-tools"; then \
	  $(mkdir_p) $(DESTDIR)$(gettextsrcdir); \
	  for file in $(DISTFILES.common) Makevars.template; do \
	    $(INSTALL_DATA) $(srcdir)/$$file \
			    $(DESTDIR)$(gettextsrcdir)/$$file; \
	  done; \
	  for file in Makevars; do \
	    rm -f $(DESTDIR)$(gettextsrcdir)/$$file; \
	  done; \
	else \
	  : ; \
	fi
install-data-no: all
install-data-yes: all
	@catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed -e 's/\.gmo$$//'`; \
	  dir=$(localedir)/$$lang/LC_MESSAGES; \
	  $(mkdir_p) $(DESTDIR)$$dir; \
	  if test -r $$cat; then realcat=$$cat; else realcat=$(srcdir)/$$cat; fi; \
	  $(INSTALL_DATA) $$realcat $(DESTDIR)$$dir/$(DOMAIN).mo; \
	  echo "installing $$realcat as $(DESTDIR)$$dir/$(DOMAIN).mo"; \
	  for lc in '' $(EXTRA_LOCALE_CATEGORIES); do \
	    if test -n "$$lc"; then \
	      if (cd $(DESTDIR)$(localedir)/$$lang && LC_ALL=C ls -l -d $$lc 2>/dev/null) | grep ' -> ' >/dev/null; then \
	        link=`cd $(DESTDIR)$(localedir)/$$lang && LC_ALL=C ls -l -d $$lc | sed -e 's/^.* -> //'`; \
	        mv $(DESTDIR)$(localedir)/$$lang/$$lc $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	        mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	        (cd $(DESTDIR)$(localedir)/$$lang/$$lc.old && \
	         for file in *; do \
	           if test -f $$file; then \
	             ln -s ../$$link/$$file $(DESTDIR)$(localedir)/$$lang/$$lc/$$file; \
	           fi; \
	         done); \
	        rm -f $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	      else \
	        if test -d $(DESTDIR)$(localedir)/$$lang/$$lc; then \
	          :; \
	        else \
	          rm -f $(DESTDIR)$(localedir)/$$lang/$$lc; \
	          mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	        fi; \
	      fi; \
	      rm -f $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo; \
	      ln -s ../LC_MESSAGES/$(DOMAIN).mo $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo 2>/dev/null || \
	      ln $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$(DOMAIN).mo $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo 2>/dev/null || \
	      cp -p $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$(DOMAIN).mo $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo; \
	      echo "installing $$realcat link as $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo"; \
	    fi; \
	  done; \
	done

install-strip: install

installdirs: installdirs-exec installdirs-data
installdirs-exec:
installdirs-data: installdirs-data-yes
	if test "$(PACKAGE)" = "gettext-tools"; then \
	  $(mkdir_p) $(DESTDIR)$(gettextsrcdir); \
	else \
	  : ; \
	fi
installdirs-data-no:
installdirs-data-yes:
	@catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed -e 's/\.gmo$$//'`; \
	  dir=$(localedir)/$$lang/LC_MESSAGES; \
	  $(mkdir_p) $(DESTDIR)$$dir; \
	  for lc in '' $(EXTRA_LOCALE_CATEGORIES); do \
	    if test -n "$$lc"; then \
	      if (cd $(DESTDIR)$(localedir)/$$lang && LC_ALL=C ls -l -d $$lc 2>/dev/null) | grep ' -> ' >/dev/null; then \
	        link=`cd $(DESTDIR)$(localedir)/$$lang && LC_ALL=C ls -l -d $$lc | sed -e 's/^.* -> //'`; \
	        mv $(DESTDIR)$(localedir)/$$lang/$$lc $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	        mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	        (cd $(DESTDIR)$(localedir)/$$lang/$$lc.old && \
	         for file in *; do \
	           if test -f $$file; then \
	             ln -s ../$$link/$$file $(DESTDIR)$(localedir)/$$lang/$$lc/$$file; \
	           fi; \
	         done); \
	        rm -f $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	      else \
	        if test -d $(DESTDIR)$(localedir)/$$lang/$$lc; then \
	          :; \
	        else \
	          rm -f $(DESTDIR)$(localedir)/$$lang/$$lc; \
	          mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	        fi; \
	      fi; \
	    fi; \
	  done; \
	done

# Define this as empty until I found a useful application.
installcheck:

uninstall: uninstall-exec uninstall-data
uninstall-exec:
uninstall-data: uninstall-data-yes
	if test "$(PACKAGE)" = "gettext-tools"; then \
	  for file in $(DISTFILES.common) Makevars.template; do \
	    rm -f $(DESTDIR)$(gettextsrcdir)/$$file; \
	  done; \
	else \
	  : ; \
	fi
uninstall-data-no:
uninstall-data-yes:
	catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed -e 's/\.gmo$$//'`; \
	  for lc in LC_MESSAGES $(EXTRA_LOCALE_CATEGORIES); do \
	    rm -f $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo; \
	  done; \
	done

check: all

info dvi ps pdf html tags TAGS ctags CTAGS ID:

mostlyclean:
	rm -f remove-potcdate.sed
	rm -f stamp-poT
	rm -f core core.* $(DOMAIN).po $(DOMAIN).1po $(DOMAIN).2po *.new.po
	rm -fr *.o

clean: mostlyclean

distclean: clean
	rm -f Makefile Makefile.in POTFILES *.mo

maintainer-clean: distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."
	rm -f stamp-po $(GMOFILES)

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)
dist distdir:
	$(MAKE) update-po
	@$(MAKE) dist2
# This is a separate target because 'update-po' must be executed before.
dist2: stamp-po $(DISTFILES)
	dists="$(DISTFILES)"; \
	if test "$(PACKAGE)" = "gettext-tools"; then \
	  dists="$$dists Makevars.template"; \
	fi; \
	if test -f $(srcdir)/$(DOMAIN).pot; then \
	  dists="$$dists $(DOMAIN).pot stamp-po"; \
	fi; \
	if test -f $(srcdir)/ChangeLog; then \
	  dists="$$dists ChangeLog"; \
	fi; \
	for i in 0 1 2 3 4 5 6 7 8 9; do \
	  if test -f $(srcdir)/ChangeLog.$$i; then \
	    dists="$$dists ChangeLog.$$i"; \
	  fi; \
	done; \
	if test -f $(srcdir)/LINGUAS; then dists="$$dists LINGUAS"; fi; \
	for file in $$dists; do \
	  if test -f $$file; then \
	    cp -p $$file $(distdir) || exit 1; \
	  else \
	    cp -p $(srcdir)/$$file $(distdir) || exit 1; \
	  fi; \
	done

update-po: Makefile
	$(MAKE) $(DOMAIN).pot-update
	test -z "$(UPDATEPOFILES)" || $(MAKE) $(UPDATEPOFILES)
	$(MAKE) update-gmo

# General rule for creating PO files.

.nop.po-create:
	@lang=`echo $@ | sed -e 's/\.po-create$$//'`; \
	echo "File $$lang.po does not exist. If you are a translator, you can create it through 'msginit'." 1>&2; \
	exit 1

# General rule for updating PO files.

.nop.po-update:
	@lang=`echo $@ | sed -e 's/\.po-update$$//'`; \
	if test "$(PACKAGE)" = "gettext-tools"; then PATH=`pwd`/../src:$$PATH; fi; \
	tmpdir=`pwd`; \
	echo "$$lang:"; \
	test "$(srcdir)" = . && cdcmd="" || cdcmd="cd $(srcdir) && "; \
	echo "$${cdcmd}$(MSGMERGE) $(MSGMERGE_OPTIONS) --lang=$$lang $$lang.po $(DOMAIN).pot -o $$lang.new.po"; \
	cd $(srcdir); \
	if { case `$(MSGMERGE) --version | sed 1q | sed -e 's,^[^0-9]*,,'` in \
	       '' | 0.[0-9] | 0.[0-9].* | 0.1[0-7] | 0.1[0-7].*) \
	         $(MSGMERGE) $(MSGMERGE_OPTIONS) -o $$tmpdir/$$lang.new.po $$lang.po $(DOMAIN).pot;; \
	       *) \
	         $(MSGMERGE) $(MSGMERGE_OPTIONS) --lang=$$lang -o $$tmpdir/$$lang.new.po $$lang.po $(DOMAIN).pot;; \
	     esac; \
	   }; then \
	  if cmp $$lang.po $$tmpdir/$$lang.new.po >/dev/null 2>&1; then \
	    rm -f $$tmpdir/$$lang.new.po; \
	  else \
	    if mv -f $$tmpdir/$$lang.new.po $$lang.po; then \
	      :; \
	    else \
	      echo "msgmerge for $$lang.po failed: cannot move $$tmpdir/$$lang.new.po to $$lang.po" 1>&2; \
	      exit 1; \
	    fi; \
	  fi; \
	else \
	  echo "msgmerge for $$lang.po failed!" 1>&2; \
	  rm -f $$tmpdir/$$lang.new.po; \
	fi

$(DUMMYPOFILES):

update-gmo: Makefile $(GMOFILES)
	@:

# Recreate Makefile by invoking config.status. Explicitly invoke the shell,
# because execution permission bits may not work on the current file system.
# Use /bin/bash, which is the shell determined by autoconf for the use by its
# scripts, not $(SHELL) which is hardwired to /bin/sh and may be deficient.
Makefile: Makefile.in.in Makevars $(top_builddir)/config.status POTFILES.in
	cd $(top_builddir) \
	  && /bin/bash ./config.status $(subdir)/$@.in po-directories

force:

.NOEXPORT:
# Special Makefile rules for Pig Latin message catalogs with quotation marks.

DISTFILES.common.extra3 = piglatin.sed en@piglatin.header insert-header.sin Rules-piglatin

.SUFFIXES: .insert-header .po-update-en-piglatin

en@piglatin.po-create:
	$(MAKE) en@piglatin.po-update

en@piglatin.po-update: en@piglatin.po-update-en

.insert-header.po-update-en-piglatin:
	@lang=`echo $@ | sed -e 's/\.po-update-en-piglatin$$//'`; \
	if test "$(PACKAGE)" = "gettext"; then PATH=`pwd`/../src:$$PATH; GETTEXTLIBDIR=`cd $(top_srcdir)/src && pwd`; export GETTEXTLIBDIR; fi; \
	tmpdir=`pwd`; \
	echo "$$lang:"; \
	ll=`echo $$lang | sed -e 's/@.*//'`; \
	LC_ALL=C; export LC_ALL; \
	cd $(srcdir); \
	if $(MSGINIT) -i $(DOMAIN).pot --no-translator -l $$lang -o - 2>/dev/null | sed -f $$tmpdir/$$lang.insert-header | $(MSGCONV) -t UTF-8 | $(MSGFILTER) sed -f `echo $$lang | sed -e 's/.*@//'`.sed 2>/dev/null > $$tmpdir/$$lang.new.po; then \
	  if cmp $$lang.po $$tmpdir/$$lang.new.po >/dev/null 2>&1; then \
	    rm -f $$tmpdir/$$lang.new.po; \
	  else \
	    if mv -f $$tmpdir/$$lang.new.po $$lang.po; then \
	      :; \
	    else \
	      echo "creation of $$lang.po failed: cannot move $$tmpdir/$$lang.new.po to $$lang.po" 1>&2; \
	      exit 1; \
	    fi; \
	  fi; \
	else \
	  echo "creation of $$lang.po failed!" 1>&2; \
	  rm -f $$tmpdir/$$lang.new.po; \
	fi

en@piglatin.insert-header: insert-header.sin
	sed -e '/^#/d' -e 's/HEADER/en@piglatin.header/g' $(srcdir)/insert-header.sin > en@piglatin.insert-header

mostlyclean: mostlyclean-piglatin
mostlyclean-piglatin:
	rm -f *.insert-header
# Special Makefile rules for English message catalogs with quotation marks.

DISTFILES.common.extra1 = quot.sed boldquot.sed en@quot.header en@boldquot.header insert-header.sin Rules-quot

.SUFFIXES: .insert-header .po-update-en

en@quot.po-create:
	$(MAKE) en@quot.po-update
en@boldquot.po-create:
	$(MAKE) en@boldquot.po-update

en@quot.po-update: en@quot.po-update-en
en@boldquot.po-update: en@boldquot.po-update-en

.insert-header.po-update-en:
	@lang=`echo $@ | sed -e 's/\.po-update-en$$//'`; \
	if test "$(PACKAGE)" = "gettext"; then PATH=`pwd`/../src:$$PATH; GETTEXTLIBDIR=`cd $(top_srcdir)/src && pwd`; export GETTEXTLIBDIR; fi; \
	tmpdir=`pwd`; \
	echo "$$lang:"; \
	ll=`echo $$lang | sed -e 's/@.*//'`; \
	LC_ALL=C; export LC_ALL; \
	cd $(srcdir); \
	if $(MSGINIT) -i $(DOMAIN).pot --no-translator -l $$lang -o - 2>/dev/null | sed -f $$tmpdir/$$lang.insert-header | $(MSGCONV) -t UTF-8 | $(MSGFILTER) sed -f `echo $$lang | sed -e 's/.*@//'`.sed 2>/dev/null > $$tmpdir/$$lang.new.po; then \
	  if cmp $$lang.po $$tmpdir/$$lang.new.po >/dev/null 2>&1; then \
	    rm -f $$tmpdir/$$lang.new.po; \
	  else \
	    if mv -f $$tmpdir/$$lang.new.po $$lang.po; then \
	      :; \
	    else \
	      echo "creation of $$lang.po failed: cannot move $$tmpdir/$$lang.new.po to $$lang.po" 1>&2; \
	      exit 1; \
	    fi; \
	  fi; \
	else \
	  echo "creation of $$lang.po failed!" 1>&2; \
	  rm -f $$tmpdir/$$lang.new.po; \
	fi

en@quot.insert-header: insert-header.sin
	sed -e '/^#/d' -e 's/HEADER/en@quot.header/g' $(srcdir)/insert-header.sin > en@quot.insert-header

en@boldquot.insert-header: insert-header.sin
	sed -e '/^#/d' -e 's/HEADER/en@boldquot.header/g' $(srcdir)/insert-header.sin > en@boldquot.insert-header

mostlyclean: mostlyclean-quot
mostlyclean-quot:
	rm -f *.insert-header
# Special Makefile rules for transforming German typography into Swiss one.

DISTFILES.common.extra2 = swiss.sed Rules-swiss

de_CH.po-create: de.po swiss.sed
	LC_ALL=C.UTF-8 $(MSGFILTER) -i $< -o $(srcdir)/de_CH.po sed -f $(srcdir)/swiss.sed

DISTFILES.common.extra4 = hebrew.sed cyrillic.sed greek.sed arabic.sed Rules-translit

de@hebrew.po-create: de.po hebrew.sed
	LC_ALL=C $(MSGFILTER) -i $< -o $(srcdir)/de@hebrew.po sed -f $(srcdir)/hebrew.sed

en@hebrew.po-create: $(DOMAIN).pot hebrew.sed
	LC_ALL=C $(MSGINIT) -i $(srcdir)/$(DOMAIN).pot --no-translator -l en@hebrew -o - 2>/dev/null | LC_ALL=C $(MSGCONV) -t UTF-8 | LC_ALL=C $(MSGFILTER) -o $(srcdir)/en@hebrew.po -i - sed -f $(srcdir)/hebrew.sed

en@cyrillic.po-create: $(DOMAIN).pot cyrillic.sed
	LC_ALL=C $(MSGINIT) -i $(srcdir)/$(DOMAIN).pot --no-translator -l en@cyrillic -o - 2>/dev/null | LC_ALL=C $(MSGCONV) -t UTF-8 | LC_ALL=C $(MSGFILTER) -o $(srcdir)/en@cyrillic.po -i - sed -f $(srcdir)/cyrillic.sed

en@greek.po-create: $(DOMAIN).pot greek.sed
	LC_ALL=C $(MSGINIT) -i $(srcdir)/$(DOMAIN).pot --no-translator -l en@greek -o - 2>/dev/null | LC_ALL=C $(MSGCONV) -t UTF-8 | LC_ALL=C $(MSGFILTER) -o $(srcdir)/en@greek.po -i - sed -f $(srcdir)/greek.sed

en@arabic.po-create: $(DOMAIN).pot arabic.sed
	LC_ALL=C $(MSGINIT) -i $(srcdir)/$(DOMAIN).pot --no-translator -l en@arabic -o - 2>/dev/null | LC_ALL=C $(MSGCONV) -t UTF-8 | LC_ALL=C $(MSGFILTER) -o $(srcdir)/en@arabic.po -i - sed -f $(srcdir)/arabic.sed
# generate windowsdir

DISTFILES.common.extra5 = Rules-windowsdir

windowsdir="$(top_builddir)/$(PACKAGE)-$(VERSION)-for-windows"
windowsdir: $(GMOFILES)
	test -d "$(windowsdir)/locale" || mkdir "$(windowsdir)/locale"
	for x in $(CATALOGS); do \
		test -d "$(windowsdir)/locale/$${x%.gmo}" || mkdir "$(windowsdir)/locale/$${x%.gmo}"; \
		cp -fp "$(srcdir)/$$x" "$(windowsdir)/locale/$${x%.gmo}/holy.mo"; \
	done
