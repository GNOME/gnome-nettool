ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = pixmaps data src help po

MESON_FILES = \
	data/meson.build \
	help/meson.build \
	pixmaps/meson.build \
	po/meson.build \
	postinstall.py \
	src/meson.build \
	meson.build

EXTRA_DIST = \
	mkinstalldirs \
	$(MESON_FILES)
