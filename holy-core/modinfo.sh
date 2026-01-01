#!/bin/sh

# User-controllable options
holy_modinfo_target_cpu=i386
holy_modinfo_platform=pc
holy_disk_cache_stats=0
holy_boot_time_stats=0
holy_have_font_source=0

# Autodetected config
holy_have_asm_uscore=0
holy_bss_start_symbol="__bss_start"
holy_end_symbol="end"

# Build environment
holy_target_cc='gcc'
holy_target_cc_version='gcc (GCC) 15.2.1 20251112'
holy_target_cflags=' -Os -Wall -W -Wshadow -Wpointer-arith -Wundef -Wchar-subscripts -Wcomment -Wdeprecated-declarations -Wdisabled-optimization -Wdiv-by-zero -Wfloat-equal -Wformat-extra-args -Wformat-security -Wformat-y2k -Wimplicit -Wimplicit-function-declaration -Wimplicit-int -Wmain -Wmissing-braces -Wmissing-format-attribute -Wmultichar -Wparentheses -Wreturn-type -Wsequence-point -Wshadow -Wsign-compare -Wswitch -Wtrigraphs -Wunknown-pragmas -Wunused -Wunused-function -Wunused-label -Wunused-parameter -Wunused-value  -Wunused-variable -Wwrite-strings -Wnested-externs -Wstrict-prototypes -g -Wredundant-decls -Wmissing-prototypes -Wmissing-declarations  -Wextra -Wattributes -Wendif-labels -Winit-self -Wint-to-pointer-cast -Winvalid-pch -Wmissing-field-initializers -Wnonnull -Woverflow -Wvla -Wpointer-to-int-cast -Wstrict-aliasing -Wvariadic-macros -Wvolatile-register-var -Wpointer-sign -Wmissing-include-dirs -Wmissing-prototypes -Wmissing-declarations -Wformat=2 -march=i386 -m32 -mrtd -mregparm=3 -falign-jumps=1 -falign-loops=1 -falign-functions=1 -freg-struct-return -mno-mmx -mno-sse -mno-sse2 -mno-sse3 -mno-3dnow -msoft-float -fno-dwarf2-cfi-asm -mno-stack-arg-probe -fno-asynchronous-unwind-tables -fno-unwind-tables -Qn -fno-PIE -fno-pie -fno-stack-protector -Wtrampolines -Werror'
holy_target_cppflags=' -Wall -W  -Dholy_MACHINE_PCBIOS=1 -Dholy_MACHINE=I386_PC -m32 -nostdinc -isystem /usr/lib/gcc/x86_64-pc-linux-gnu/15.2.1/include -I$(top_srcdir)/include -I$(top_builddir)/include'
holy_target_ccasflags=' -g  -m32 -msoft-float'
holy_target_ldflags=' -m32 -Wl,-melf_i386 -no-pie -Wl,--build-id=none'
holy_cflags='-O2 -march=native -pipe -fno-semantic-interposition -fno-common -fstack-protector-strong -fstack-clash-protection -fcf-protection -D_FORTIFY_SOURCE=2 -fasynchronous-unwind-tables -fpic -fpie'
holy_cppflags='-DDEBUG -DGRUB_DEBUG -DGRUB_UTIL=1 -D_FILE_OFFSET_BITS=64'
holy_ccasflags='-O2 -march=native -pipe -fno-semantic-interposition -fno-common -fstack-protector-strong -fstack-clash-protection -fcf-protection -D_FORTIFY_SOURCE=2 -fasynchronous-unwind-tables -fpic -fpie'
holy_ldflags='-Wl,-O1 -Wl,--as-needed -Wl,-z,relro -Wl,-z,now -Wl,-z,defs -Wl,-z,noexecstack -Wl,--hash-style=gnu -Wl,--sort-common -Wl,--build-id -pie'
holy_target_strip='strip'
holy_target_nm='nm'
holy_target_ranlib='ranlib'
holy_target_objconf=''
holy_target_obj2elf=''
holy_target_img_base_ldopt='-Wl,-Ttext'
holy_target_img_ldflags='@TARGET_IMG_BASE_LDFLAGS@'

# Version
holy_version="2.02"
holy_package="holy"
holy_package_string="holy 2.02"
holy_package_version="2.02"
holy_package_name="holy"
holy_package_bugreport="bashscripter123@gmail.com"
