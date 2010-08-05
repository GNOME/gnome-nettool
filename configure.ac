AC_PREREQ([2.64])
AC_INIT([GNOME Nettool],
        [2.31.6],
        [http://bugzilla.gnome.org/enter_bug.cgi?product=gnome-nettool],
        [gnome-nettool],
        [http://projects.gnome.org/gnome-network/]) #FIXME: website should be updated, it is even yet named gnome-network

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/ping.c])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11.1 foreign])
AM_MAINTAINER_MODE([enable])
# Support silent build rules, requires at least automake-1.11. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT

# Check headers
AC_CHECK_HEADERS(sys/sockio.h)
AC_CHECK_HEADERS(sys/socket.h ifaddrs.h,,AC_MSG_ERROR(Required header not found: $ac_header))

dnl ******************************************
dnl ** pkg-config dependacy checks          **
dnl ******************************************
GTK_REQUIRED=2.90.0
GCONF_REQUIRED=1.1.11

AC_MSG_CHECKING([which gtk+ version to compile against])
AC_ARG_WITH([gtk],
  [AS_HELP_STRING([--with-gtk=2.0|3.0],[which gtk+ version to compile against (default: 2.0)])],
  [case "$with_gtk" in
     2.0|3.0) ;;
     *) AC_MSG_ERROR([invalid gtk version specified]) ;;
   esac],
  [with_gtk=2.0])
AC_MSG_RESULT([$with_gtk])

case "$with_gtk" in
  2.0) GTK_API_VERSION=2.0
       GTK_REQUIRED=2.19.7
       ;;
  3.0) GTK_API_VERSION=3.0
       GTK_REQUIRED=2.90.4
       ;;
esac

PKG_CHECK_MODULES(NETTOOL,
		  gtk+-$GTK_API_VERSION >= $GTK_REQUIRED
		  gconf-2.0      >= $GCONF_REQUIRED
		  gmodule-export-2.0
		  gnome-doc-utils
		  libgtop-2.0)
AC_SUBST(NETTOOL_CFLAGS)
AC_SUBST(NETTOOL_LIBS)

dnl
dnl Translations
dnl
IT_PROG_INTLTOOL([0.40.0])

GETTEXT_PACKAGE=gnome-nettool
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[The gettext package])
AM_GLIB_GNU_GETTEXT

GNOME_DOC_INIT

dnl
dnl GConf stuff
dnl
AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
if test x"$GCONFTOOL" = xno; then
        AC_MSG_ERROR([gconftool-2 executable not found in your path - should be installed with GConf])
fi
AM_GCONF_SOURCE_2


dnl
dnl Debugging flags
dnl
GNOME_COMPILE_WARNINGS([maximum])
CFLAGS="$CFLAGS $WARN_CFLAGS"

set_debug=no
AC_ARG_ENABLE(debug,
[  --enable-debug       Lots of noisy debug messages],
set_debug="$enableval")

AC_MSG_CHECKING(if debug messages are enabled)
if test "$set_debug" != "no"; then
        AC_MSG_RESULT(yes)
        CFLAGS="-DGRD_DEBUG $CFLAGS"
else
        AC_MSG_RESULT(no)
fi

AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)

AC_CONFIG_FILES([
Makefile
data/Makefile
data/gnome-nettool.desktop.in
help/Makefile
pixmaps/Makefile
pixmaps/icons/Makefile
pixmaps/icons/16x16/Makefile
pixmaps/icons/16x16/apps/Makefile
pixmaps/icons/22x22/Makefile
pixmaps/icons/22x22/apps/Makefile
pixmaps/icons/24x24/Makefile
pixmaps/icons/24x24/apps/Makefile
pixmaps/icons/32x32/Makefile
pixmaps/icons/32x32/apps/Makefile
pixmaps/icons/scalable/Makefile
pixmaps/icons/scalable/apps/Makefile
po/Makefile.in
src/Makefile])
AC_OUTPUT
