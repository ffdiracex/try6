        AUTOMAKE_OPTIONS = subdir-objects -Wno-portability

DEPDIR=.deps-core

include $(top_srcdir)/conf/Makefile.common

CC=$(TARGET_CC)
CPP=$(TARGET_CC)
CCAS=$(TARGET_CC)
RANLIB=$(TARGET_RANLIB)
STRIP=$(TARGET_STRIP)

MACHO2IMG=$(top_builddir)/holy-macho2img

AM_CFLAGS = $(TARGET_CFLAGS)
AM_LDFLAGS = $(TARGET_LDFLAGS)
AM_CPPFLAGS = $(TARGET_CPPFLAGS) $(CPPFLAGS_DEFAULT)
AM_CCASFLAGS = $(TARGET_CCASFLAGS) $(CCASFLAGS_DEFAULT)

CFLAGS_PROGRAM += $(CFLAGS_PLATFORM)
LDFLAGS_PROGRAM += $(LDFLAGS_PLATFORM)
CPPFLAGS_PROGRAM += $(CPPFLAGS_PLATFORM)
CCASFLAGS_PROGRAM += $(CCASFLAGS_PLATFORM)

CFLAGS_LIBRARY += $(CFLAGS_PLATFORM) -fno-builtin
CPPFLAGS_LIBRARY += $(CPPFLAGS_PLATFORM)
CCASFLAGS_LIBRARY += $(CCASFLAGS_PLATFORM)

build-holy-pep2elf$(BUILD_EXEEXT): $(top_srcdir)/util/holy-pe2elf.c $(top_srcdir)/holy-core/kern/emu/misc.c $(top_srcdir)/util/misc.c
	$(BUILD_CC) -o $@ -I$(top_srcdir)/include $(BUILD_CFLAGS) $(BUILD_CPPFLAGS) $(BUILD_LDFLAGS) -Dholy_BUILD=1 -Dholy_TARGET_WORDSIZE=64 -Dholy_UTIL=1 -Dholy_BUILD_PROGRAM_NAME=\"build-holy-pep2elf\" $^
CLEANFILES += build-holy-pep2elf$(BUILD_EXEEXT)

build-holy-pe2elf$(BUILD_EXEEXT): $(top_srcdir)/util/holy-pe2elf.c $(top_srcdir)/holy-core/kern/emu/misc.c $(top_srcdir)/util/misc.c
	$(BUILD_CC) -o $@ -I$(top_srcdir)/include $(BUILD_CFLAGS) $(BUILD_CPPFLAGS) $(BUILD_LDFLAGS) -Dholy_BUILD=1 -Dholy_TARGET_WORDSIZE=32 -Dholy_UTIL=1 -Dholy_BUILD_PROGRAM_NAME=\"build-holy-pe2elf\" $^
CLEANFILES += build-holy-pe2elf$(BUILD_EXEEXT)

# gentrigtables
gentrigtables$(BUILD_EXEEXT): gentrigtables.c
	$(BUILD_CC) -o $@ -I$(top_srcdir)/include $(BUILD_CFLAGS) $(BUILD_CPPFLAGS) $(BUILD_LDFLAGS) $< $(BUILD_LIBM)
CLEANFILES += gentrigtables$(BUILD_EXEEXT)

build-holy-module-verifier$(BUILD_EXEEXT): $(top_srcdir)/util/holy-module-verifier.c $(top_srcdir)/util/holy-module-verifier32.c $(top_srcdir)/util/holy-module-verifier64.c $(top_srcdir)/holy-core/kern/emu/misc.c $(top_srcdir)/util/misc.c
	$(BUILD_CC) -o $@ -I$(top_srcdir)/include $(BUILD_CFLAGS) $(BUILD_CPPFLAGS) $(BUILD_LDFLAGS) -Dholy_BUILD=1 -Dholy_UTIL=1 -Dholy_BUILD_PROGRAM_NAME=\"build-holy-module-verifier\" $^
CLEANFILES += build-holy-module-verifier$(BUILD_EXEEXT)

# trigtables.c
trigtables.c: gentrigtables$(BUILD_EXEEXT) gentrigtables.c $(top_srcdir)/configure.ac
	./gentrigtables$(BUILD_EXEEXT) > $@
CLEANFILES += trigtables.c

# XXX Use Automake's LEX & YACC support
holy_script.tab.h: script/parser.y
	$(YACC) -d -p holy_script_yy -b holy_script $<
holy_script.tab.c: holy_script.tab.h
CLEANFILES += holy_script.tab.c holy_script.tab.h

# For the lexer.
holy_script.yy.h: script/yylex.l
	$(LEX) -o holy_script.yy.c --header-file=holy_script.yy.h $<
holy_script.yy.c: holy_script.yy.h

rs_decoder.h: $(srcdir)/lib/reed_solomon.c
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) -Os -I$(top_builddir) -S -DSTANDALONE -o $@ $< -g0 -mregparm=3 -ffreestanding

CLEANFILES += holy_script.yy.c holy_script.yy.h

include $(srcdir)/Makefile.core.am

KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/cache.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/command.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/device.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/disk.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/dl.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/env.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/env_private.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/err.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/file.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/fs.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/i18n.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/kernel.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/list.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/misc.h
if COND_emu
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/compiler-rt-emu.h
else
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/compiler-rt.h
endif
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/mm.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/parser.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/partition.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/term.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/time.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/mm_private.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/net.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/tpm.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/memory.h

if COND_i386_pc
KERNEL_HEADER_FILES += $(top_builddir)/include/holy/machine/kernel.h
KERNEL_HEADER_FILES += $(top_builddir)/include/holy/machine/pxe.h
KERNEL_HEADER_FILES += $(top_builddir)/include/holy/machine/int.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/i386/tsc.h
endif

if COND_i386_efi
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/efi/efi.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/efi/disk.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/i386/tsc.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/acpi.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/pci.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/i386/pmtimer.h
endif

if COND_i386_coreboot
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/i386/tsc.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/i386/coreboot/lbio.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/video.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/video_fb.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/gfxterm.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/font.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/bufio.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/acpi.h
endif

if COND_i386_multiboot
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/i386/tsc.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/acpi.h
endif

if COND_i386_qemu
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/pci.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/i386/tsc.h
endif

if COND_i386_ieee1275
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/ieee1275/ieee1275.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/terminfo.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/extcmd.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/lib/arg.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/i386/tsc.h
endif

if COND_i386_xen
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/xen.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/i386/xen/hypercall.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/terminfo.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/extcmd.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/lib/arg.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/loader.h
endif

if COND_x86_64_xen
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/xen.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/x86_64/xen/hypercall.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/terminfo.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/extcmd.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/lib/arg.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/loader.h
endif

if COND_x86_64_efi
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/efi/efi.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/efi/disk.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/i386/tsc.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/pci.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/acpi.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/i386/pmtimer.h
endif

if COND_ia64_efi
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/efi/efi.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/efi/disk.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/acpi.h
endif

if COND_mips
KERNEL_HEADER_FILES += $(top_builddir)/include/holy/cpu/kernel.h
endif

if COND_mips_arc
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/extcmd.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/arc/arc.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/terminfo.h
endif

if COND_mips_qemu_mips
KERNEL_HEADER_FILES += $(top_builddir)/include/holy/keyboard_layouts.h
KERNEL_HEADER_FILES += $(top_builddir)/include/holy/machine/kernel.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/serial.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/loader.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/terminfo.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/extcmd.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/lib/arg.h
endif

if COND_mips_loongson
KERNEL_HEADER_FILES += $(top_builddir)/include/holy/keyboard_layouts.h
KERNEL_HEADER_FILES += $(top_builddir)/include/holy/machine/kernel.h
KERNEL_HEADER_FILES += $(top_builddir)/include/holy/machine/time.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/video.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/video_fb.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/gfxterm.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/font.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/bufio.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/pci.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/cs5536.h
KERNEL_HEADER_FILES += $(top_builddir)/include/holy/machine/pci.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/serial.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/loader.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/terminfo.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/extcmd.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/lib/arg.h
endif

if COND_mips_qemu_mips
KERNEL_HEADER_FILES += $(top_builddir)/include/holy/machine/memory.h
KERNEL_HEADER_FILES += $(top_builddir)/include/holy/machine/kernel.h
endif

if COND_powerpc_ieee1275
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/ieee1275/ieee1275.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/terminfo.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/extcmd.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/lib/arg.h
endif

if COND_sparc64_ieee1275
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/ieee1275/ieee1275.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/sparc64/ieee1275/ieee1275.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/terminfo.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/extcmd.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/lib/arg.h
endif

if COND_arm_uboot
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/uboot/uboot.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/uboot/disk.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/extcmd.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/lib/arg.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/arm/system.h
endif

if COND_arm_efi
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/arm/efi/loader.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/efi/efi.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/efi/disk.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/arm/system.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/acpi.h
endif

if COND_arm64_efi
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/efi/efi.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/efi/disk.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/acpi.h
endif

if COND_emu
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/datetime.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/emu/misc.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/emu/net.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/emu/hostdisk.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/emu/hostfile.h
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/extcmd.h
if COND_holy_EMU_SDL
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/sdl.h
endif
if COND_holy_EMU_PCI
KERNEL_HEADER_FILES += $(top_srcdir)/include/holy/libpciaccess.h
endif
endif

symlist.h: $(top_builddir)/config.h $(KERNEL_HEADER_FILES)
	@list='$^'; \
	for p in $$list; do \
	  echo "#include <$$p>" >> $@ || (rm -f $@; exit 1); \
	done
CLEANFILES += symlist.h
BUILT_SOURCES += symlist.h

symlist.c: symlist.h gensymlist.sh
	$(TARGET_CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS_KERNEL) $(CPPFLAGS) -Dholy_SYMBOL_GENERATOR=1 symlist.h > symlist.p || (rm -f symlist.p; exit 1)
	cat symlist.p | /bin/sh $(srcdir)/gensymlist.sh $(top_builddir)/config.h $(KERNEL_HEADER_FILES) >$@ || (rm -f $@; exit 1)
	rm -f symlist.p
CLEANFILES += symlist.c
BUILT_SOURCES += symlist.c

if COND_HAVE_ASM_USCORE
ASM_PREFIX=_
else
ASM_PREFIX=
endif

noinst_DATA += kernel_syms.lst

kernel_syms.lst: $(KERNEL_HEADER_FILES) $(top_builddir)/config.h
	$(TARGET_CPP) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS_KERNEL) $(CPPFLAGS) $(CFLAGS) -Dholy_SYMBOL_GENERATOR=1 $^ >kernel_syms.input
	cat kernel_syms.input | grep -v '^#' | sed -n \
	  -e '/EXPORT_FUNC *([a-zA-Z0-9_]*)/{s/.*EXPORT_FUNC *(\([a-zA-Z0-9_]*\)).*/defined kernel '"$(ASM_PREFIX)"'\1/;p;}' \
	  -e '/EXPORT_VAR *([a-zA-Z0-9_]*)/{s/.*EXPORT_VAR *(\([a-zA-Z0-9_]*\)).*/defined kernel '"$(ASM_PREFIX)"'\1/;p;}' \
	  | sort -u >$@
	rm -f kernel_syms.input
CLEANFILES += kernel_syms.lst

if COND_emu
kern/emu/holy_emu-main.$(OBJEXT):holy_emu_init.h
holy_emu-holy_emu_init.$(OBJEXT):holy_emu_init.h
kern/emu/holy_emu_dyn-main.$(OBJEXT):holy_emu_init.h
holy_emu_dyn-holy_emu_init.$(OBJEXT):holy_emu_init.h

holy_emu_init.h: genemuinitheader.sh $(MODULE_FILES)
	rm -f $@; echo $(MODULE_FILES) | sh $(srcdir)/genemuinitheader.sh $(TARGET_NM) > $@
CLEANFILES += holy_emu_init.h

holy_emu_init.c: holy_emu_init.h genemuinit.sh $(MODULE_FILES)
	rm -f $@; echo $(MODULE_FILES) | sh $(srcdir)/genemuinit.sh $(TARGET_NM) > $@
CLEANFILES += holy_emu_init.c
endif

# List files

fs.lst: $(MARKER_FILES)
	(for pp in $^; do \
	  b=`basename $$pp .marker`; \
	  if grep 'FS_LIST_MARKER' $$pp >/dev/null 2>&1; then \
	    echo $$b; \
	  fi; \
	done) | sort -u > $@
platform_DATA += fs.lst
CLEANFILES += fs.lst

command.lst: $(MARKER_FILES)
	(for pp in $^; do \
	  b=`basename $$pp .marker`; \
	  sed -n \
	    -e "/EXTCOMMAND_LIST_MARKER *( *\"/{s/.*( *\"\([^\"]*\)\".*/*\1: $$b/;p;}" \
	    -e "/P1COMMAND_LIST_MARKER *( *\"/{s/.*( *\"\([^\"]*\)\".*/*\1: $$b/;p;}" \
	    -e "/COMMAND_LIST_MARKER *( *\"/{s/.*( *\"\([^\"]*\)\".*/\1: $$b/;p;}" $$pp; \
	done) | sort -u > $@
platform_DATA += command.lst
CLEANFILES += command.lst

partmap.lst: $(MARKER_FILES)
	(for pp in $^; do \
	  b=`basename $$pp .marker`; \
	  if grep 'PARTMAP_LIST_MARKER' $$pp >/dev/null 2>&1; then \
	    echo $$b; \
	  fi; \
	done) | sort -u > $@
platform_DATA += partmap.lst
CLEANFILES += partmap.lst

terminal.lst: $(MARKER_FILES)
	(for pp in $^; do \
	  b=`basename $$pp .marker`; \
	  sed -n \
	    -e "/INPUT_TERMINAL_LIST_MARKER *( *\"/{s/.*( *\"\([^\"]*\)\".*/i\1: $$b/;p;}" \
	    -e "/OUTPUT_TERMINAL_LIST_MARKER *( *\"/{s/.*( *\"\([^\"]*\)\".*/o\1: $$b/;p;}" $$pp; \
	done) | sort -u > $@
platform_DATA += terminal.lst
CLEANFILES += terminal.lst

parttool.lst: $(MARKER_FILES)
	(for pp in $^; do \
	  b=`basename $$pp .marker`; \
	  sed -n \
	    -e "/PARTTOOL_LIST_MARKER *( *\"/{s/.*( *\"\([^\"]*\)\".*/\1: $$b/;p;}" $$pp; \
	done) | sort -u > $@
platform_DATA += parttool.lst
CLEANFILES += parttool.lst

video.lst: $(MARKER_FILES)
	(for pp in $^; do \
	  b=`basename $$pp .marker`; \
	  if grep 'VIDEO_LIST_MARKER' $$pp >/dev/null 2>&1; then \
	    echo $$b; \
	  fi; \
	done) | sort -u > $@
platform_DATA += video.lst
CLEANFILES += video.lst

# but, crypto.lst is simply copied
crypto.lst: $(srcdir)/lib/libgcrypt-holy/cipher/crypto.lst
	cp $^ $@
platform_DATA += crypto.lst
CLEANFILES += crypto.lst

syminfo.lst: gensyminfo.sh kernel_syms.lst $(MODULE_FILES)
	cat kernel_syms.lst > $@.new
	for m in $(MODULE_FILES); do \
	  sh $< $$m >> $@.new || exit 1; \
	done
	mv $@.new $@

# generate global module dependencies list
moddep.lst: syminfo.lst genmoddep.awk video.lst
	cat $< | sort | $(AWK) -f $(srcdir)/genmoddep.awk > $@ || (rm -f $@; exit 1)
platform_DATA += moddep.lst
CLEANFILES += config.log syminfo.lst moddep.lst

$(MOD_FILES): %.mod : genmod.sh moddep.lst %.module$(EXEEXT) build-holy-module-verifier$(BUILD_EXEEXT)
	TARGET_OBJ2ELF=@TARGET_OBJ2ELF@ sh $^ $@
platform_DATA += $(MOD_FILES)
platform_DATA += modinfo.sh
CLEANFILES += $(MOD_FILES)

if COND_ENABLE_EFIEMU
efiemu32.o: efiemu/runtime/efiemu.c $(TARGET_OBJ2ELF)
	-rm -f $@
	-rm -f $@.bin
	$(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_DEFAULT) -m32 -Wall -Werror -nostdlib -static -O2 -c -o $@.bin $<
	if test "x$(TARGET_APPLE_LINKER)" = x1; then \
	  $(TARGET_OBJCONV) -felf32 -nu -nd $@.bin $@ || exit 1; \
	  rm -f $@.bin ; \
	elif test ! -z "$(TARGET_OBJ2ELF)"; then \
	  $(TARGET_OBJ2ELF) $@.bin || (rm -f $@.bin; exit 1); \
	  mv $@.bin $@ ; \
	else \
	  mv $@.bin $@ ; \
	fi

# Link format -arch,x86_64 means Apple linker
efiemu64_c.o: efiemu/runtime/efiemu.c
	$(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_DEFAULT) -m64 -nostdlib -Wall -Werror -O2 -mcmodel=large -mno-red-zone -c -o $@ $<

efiemu64_s.o: efiemu/runtime/efiemu.S
	$(TARGET_CC) $(DEFS) $(INCLUDES) $(CPPFLAGS_EFIEMU) $(CPPFLAGS_DEFAULT) -m64 -Wall -Werror -nostdlib -O2 -mcmodel=large -mno-red-zone -c -o $@ $<

efiemu64.o: efiemu64_c.o efiemu64_s.o $(TARGET_OBJ2ELEF)
	-rm -f $@
	-rm -f $@.bin
	$(TARGET_CC) -m64 $(EFIEMU64_LINK_FORMAT) -nostdlib -static -Wl,-r -o $@.bin $^
	if test "x$(EFIEMU64_LINK_FORMAT)" = x-arch,x86_64; then \
	  $(TARGET_OBJCONV) -felf64 -nu -nd $@.bin $@ || exit 1; \
	  rm -f $@.bin; \
	else \
	  mv $@.bin $@ ; \
	fi

platform_DATA += efiemu32.o efiemu64.o
CLEANFILES += efiemu32.o efiemu64.o efiemu64_c.o efiemu64_s.o
endif

windowsdir=$(top_builddir)/$(PACKAGE)-$(VERSION)-for-windows
windowsdir: $(PROGRAMS) $(starfield_DATA) $(platform_DATA)
	test -d $(windowsdir)/$(target_cpu)-$(platform) || mkdir $(windowsdir)/$(target_cpu)-$(platform)
	for x in $(platform_DATA); do \
		cp -fp $$x $(windowsdir)/$(target_cpu)-$(platform)/$$x; \
	done
