AC_PREREQ([2.64])
AC_INIT([GNOME Nettool],
        [3.8.1],
        [https://gitlab.gnome.org/GNOME/gnome-nettool/-/issues/],
        [gnome-nettool],
        [https://gitlab.gnome.org/GNOME/gnome-nettool])

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
GIO_REQUIRED=2.26.0
GTK_REQUIRED=3.0.0

PKG_CHECK_MODULES(NETTOOL,
		  gtk+-3.0 >= $GTK_REQUIRED
		  gio-2.0 >= $GIO_REQUIRED
		  gmodule-export-2.0
		  libgtop-2.0)
AC_SUBST(NETTOOL_CFLAGS)
AC_SUBST(NETTOOL_LIBS)

dnl
dnl Translations
dnl
GETTEXT_PACKAGE=gnome-nettool
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE",[The gettext package])

AX_REQUIRE_DEFINED([AM_GNU_GETTEXT_VERSION])
AM_GNU_GETTEXT_VERSION([0.19.8])

AX_REQUIRE_DEFINED([AM_GNU_GETTEXT])
AM_GNU_GETTEXT([external])

#User Documentation
YELP_HELP_INIT

dnl
dnl GSettings stuff
dnl
GLIB_GSETTINGS

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
data/gnome-nettool.appdata.xml
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
