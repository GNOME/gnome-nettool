#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="gnome-netinfo"
REQUIRED_AUTOMAKE_VERSION=1.7

if [ ! -f "$srcdir/src/nettool.c" ]; then
	echo "$srcdir doesn't look like source directory for $PKG_NAME" >&2
	exit 1
fi

USE_GNOME2_MACROS=1 . gnome-autogen.sh
