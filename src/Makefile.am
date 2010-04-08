INCLUDES = 							\
	-DGNOME_NETTOOL_LOCALEDIR=\""$(datadir)/locale"\" 	\
	-DUI_DIR=\""$(pkgdatadir)/ui/"\" 			\
	-DPIXMAPS_DIR=\""$(datadir)/gnome-nettool/pixmaps"\"	\
	@NETTOOL_CFLAGS@

LDADD =				\
	$(NETTOOL_LIBS)	

bin_PROGRAMS = gnome-nettool

gnome_nettool_SOURCES = \
	callbacks.c callbacks.h \
	nettool.c nettool.h \
	gn-combo-history.c gn-combo-history.h \
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
