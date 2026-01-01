#! /usr/bin/env bash
# Copyright 2025 Felix P. A. Gillberg HolyBooter
# SPDX-License-Identifier: GPL-2.0


set -e

# Set ${PYTHON} to plain 'python' if not set already
: ${PYTHON:=python}

export LC_COLLATE=C
unset LC_ALL

find . -iname '*.[ch]' ! -ipath './holy-core/lib/libgcrypt-holy/*' ! -ipath './build-aux/*' ! -ipath './holy-core/lib/libgcrypt/src/misc.c' ! -ipath './holy-core/lib/libgcrypt/src/global.c' ! -ipath './holy-core/lib/libgcrypt/src/secmem.c'  ! -ipath './util/holy-gen-widthspec.c' ! -ipath './util/holy-gen-asciih.c' |sort > po/POTFILES.in
find util -iname '*.in' ! -name Makefile.in  |sort > po/POTFILES-shell.in

echo "Importing unicode..."
${PYTHON} util/import_unicode.py unicode/UnicodeData.txt unicode/BidiMirroring.txt unicode/ArabicShaping.txt holy-core/unidata.c

echo "Importing libgcrypt..."
${PYTHON} util/import_gcry.py holy-core/lib/libgcrypt/ holy-core
sed -n -f util/import_gcrypth.sed < holy-core/lib/libgcrypt/src/gcrypt.h.in > include/holy/gcrypt/gcrypt.h
if [ -f include/holy/gcrypt/g10lib.h ]; then
    rm include/holy/gcrypt/g10lib.h
fi
if [ -d holy-core/lib/libgcrypt-holy/mpi/generic ]; then
    rm -rf holy-core/lib/libgcrypt-holy/mpi/generic
fi
cp holy-core/lib/libgcrypt-holy/src/g10lib.h include/holy/gcrypt/g10lib.h
cp -R holy-core/lib/libgcrypt/mpi/generic holy-core/lib/libgcrypt-holy/mpi/generic

for x in mpi-asm-defs.h mpih-add1.c mpih-sub1.c mpih-mul1.c mpih-mul2.c mpih-mul3.c mpih-lshift.c mpih-rshift.c; do
    if [ -h holy-core/lib/libgcrypt-holy/mpi/"$x" ] || [ -f holy-core/lib/libgcrypt-holy/mpi/"$x" ]; then
	rm holy-core/lib/libgcrypt-holy/mpi/"$x"
    fi
    cp holy-core/lib/libgcrypt-holy/mpi/generic/"$x" holy-core/lib/libgcrypt-holy/mpi/"$x"
done

echo "Generating Automake input..."

# Automake doesn't like including files from a path outside the project.
rm -f contrib holy-core/contrib
if [ "x${holy_CONTRIB}" != x ]; then
  [ "${holy_CONTRIB}" = contrib ] || ln -s "${holy_CONTRIB}" contrib
  [ "${holy_CONTRIB}" = holy-core/contrib ] || ln -s ../contrib holy-core/contrib
fi

UTIL_DEFS='Makefile.util.def Makefile.utilgcry.def'
CORE_DEFS='holy-core/Makefile.core.def holy-core/Makefile.gcry.def'

for extra in contrib/*/Makefile.util.def; do
  if test -e "$extra"; then
    UTIL_DEFS="$UTIL_DEFS $extra"
  fi
done

for extra in contrib/*/Makefile.core.def; do
  if test -e "$extra"; then
    CORE_DEFS="$CORE_DEFS $extra"
  fi
done

${PYTHON} gentpl.py $UTIL_DEFS > Makefile.util.am
${PYTHON} gentpl.py $CORE_DEFS > holy-core/Makefile.core.am

for extra in contrib/*/Makefile.common; do
  if test -e "$extra"; then
    echo "include $extra" >> Makefile.util.am
    echo "include $extra" >> holy-core/Makefile.core.am
  fi
done

for extra in contrib/*/Makefile.util.common; do
  if test -e "$extra"; then
    echo "include $extra" >> Makefile.util.am
  fi
done

for extra in contrib/*/Makefile.core.common; do
  if test -e "$extra"; then
    echo "include $extra" >> holy-core/Makefile.core.am
  fi
done

echo "Saving timestamps..."
echo timestamp > stamp-h.in

echo "Running autoreconf..."
autoreconf -vi
exit 0
