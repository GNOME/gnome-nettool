dialogdir = $(datadir)/$(PACKAGE)/dialogs
dialog_DATA = gnome-nettool.glade

INCLUDES = 			\
	-DGNOME_NETTOOL_LOCALEDIR=\""$(datadir)/locale"\" \
	-DDATADIR=\""$(dialogdir)/"\" \
	@NETTOOL_CFLAGS@

LDADD =				\
	$(NETINFO_LIBS)	

bin_PROGRAMS = gnome-nettool

gnome_nettool_SOURCES = \
	callbacks.c callbacks.h \
	nettool.c nettool.h \
	ping.c ping.h \
	traceroute.c traceroute.h \
	netstat.c netstat.h \
	info.c info.h \
	scan.c scan.h \
	lookup.c lookup.h \
	finger.c finger.h \
	whois.c whois.h \
	utils.c utils.h \
	util-mii.c util-mii.h mii.h \
	util-mii-wireless.h \
	main.c

gnome_nettool_LDADD = @NETTOOL_LIBS@

pixmapdir   = $(datadir)/pixmaps
pixmap_DATA = gnome-nettool.png
EXTRA_DIST  = \
	$(pixmap_DATA) \
	gnome-nettool.desktop.in \
	$(dialog_DATA)

Applicationsdir = $(datadir)/applications
Applications_in_files = gnome-nettool.desktop.in
Applications_DATA = $(Applications_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

install-data-local:
	@$(NORMAL_INSTALL)
