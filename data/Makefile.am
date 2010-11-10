uidir = $(pkgdatadir)/ui
ui_DATA = gnome-nettool.ui

gsettings_SCHEMAS = org.gnome.gnome-nettool.gschema.xml
@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

desktopdir = $(datadir)/applications
desktop_in_in_files = gnome-nettool.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =                    		\
	$(desktop_in_in_files)  		\
	org.gnome.gnome-nettool.gschema.xml.in	\
	$(ui_DATA)

DISTCLEANFILES =                \
	$(desktop_in_files)	\
	$(gsettings_SCHEMAS)

CLEANFILES =                    \
	$(desktop_DATA)
