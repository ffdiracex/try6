#! /bin/sh
set -e

#
# Example:
#
# gensyms.sh normal.module
#

module=$1
modname=`echo $module | sed -e 's@\.module.*$@@'`

# Print all symbols defined by module
if test x"--defined-only" = x && test x"-P" = x; then
    nm -g -p $module | \
	sed -n "s@^\([0-9a-fA-F]*\)  *[TBRDS]  *\([^ ]*\).*@defined $modname \2@p"
elif test x"--defined-only" = x; then
    nm -g -P -p $module | \
	sed -n "s@^\([^ ]*\)  *[TBRDS]  *\([0-9a-fA-F]*\).*@defined $modname \1@p"
else
    nm -g --defined-only -P -p $module | \
	sed "s@^\([^ ]*\).*@defined $modname \1@g"
fi

# Print all undefined symbols used by module
nm -u -P -p $module | sed "s@^\([^ ]*\).*@undefined $modname \1@g"
