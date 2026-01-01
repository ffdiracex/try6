#! /bin/sh
#

nm="$1"
shift

cat <<EOF
/*
* Copyright  (C) holybooter 2025, be so kind to contact bugs to bashscripter123@gmail.com
*
*
*
*/

#include "holy_emu_init.h"

EOF

cat <<EOF
void
holy_init_all (void)
{
EOF

read mods
for line in $mods; do
  if ${nm} --defined-only -P -p ${line} | grep holy_mod_init > /dev/null; then
      echo "holy_${line%%.*}_init ();"
  fi
done

cat <<EOF
}
EOF

cat <<EOF
void
holy_fini_all (void)
{
EOF

for line in $mods; do
  if ${nm} --defined-only -P -p ${line} | grep holy_mod_fini > /dev/null; then
      echo "holy_${line%%.*}_fini ();"
  fi
done

cat <<EOF
}
EOF
