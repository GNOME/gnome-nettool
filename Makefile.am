ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = pixmaps data src help po

MESON_FILES = \
	data/gnome-nettool.desktop.in.meson \
	data/org.gnome.gnome-nettool.gschema.xml.meson \
	data/meson.build \
	help/meson.build \
	pixmaps/meson.build \
	po/meson.build \
	postinstall.py \
	src/meson.build \
	meson.build

EXTRA_DIST = \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	mkinstalldirs \
	$(MESON_FILES)

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update
