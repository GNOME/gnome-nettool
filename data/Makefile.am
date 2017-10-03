uidir = $(pkgdatadir)/ui
ui_DATA = gnome-nettool.ui

gsettings_SCHEMAS = org.gnome.gnome-nettool.gschema.xml
@GSETTINGS_RULES@

desktopdir = $(datadir)/applications
desktop_in_in_files = gnome-nettool.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

appdatadir = $(datadir)/appdata
appdata_in_files = gnome-nettool.appdata.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)

$(appdata_DATA): $(appdata_in_files)
	$(AM_V_GEN) $(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

gnome-nettool.desktop: gnome-nettool.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST =			\
	$(desktop_in_in_files)	\
	$(appdata_in_files)	\
	$(gsettings_SCHEMAS)	\
	$(ui_DATA)

DISTCLEANFILES =                \
	$(desktop_in_files)         \
	$(data_appdata_DATA)

CLEANFILES =                    \
	$(desktop_DATA)             \
	$(appdata_DATA)
