dialogdir = $(datadir)/$(PACKAGE)/dialogs
dialog_DATA = gnome-netinfo.glade

INCLUDES = 			\
	-DGNOME_NETINFO_LOCALEDIR=\""$(datadir)/locale"\" \
	-DDATADIR=\""$(dialogdir)/"\" \
	@NETINFO_CFLAGS@

LDADD =				\
	$(NETINFO_LIBS)	

bin_PROGRAMS = gnome-netinfo

gnome_netinfo_SOURCES = \
	callbacks.c callbacks.h \
	netinfo.c netinfo.h \
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

gnome_netinfo_LDADD = @NETINFO_LIBS@

pixmapdir   = $(datadir)/pixmaps
pixmap_DATA = gnome-netinfo.png
EXTRA_DIST  = \
	$(pixmap_DATA) \
	gnome-netinfo.desktop.in \
	$(dialog_DATA) \
	gnome-netinfo.gladep

Applicationsdir = $(datadir)/applications
Applications_in_files = gnome-netinfo.desktop.in
Applications_DATA = $(Applications_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

install-data-local:
	@$(NORMAL_INSTALL)
