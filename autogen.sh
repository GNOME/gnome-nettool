#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="gnome-netinfo"

(test -f $srcdir/configure.in \
  && test -d $srcdir/src \
  && test -d $srcdir/src/ping.c) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

gnome_autogen=`which gnome-autogen.sh`
test -z "$gnome_autogen"

USE_GNOME2_MACROS=1 . $gnome_autogen
