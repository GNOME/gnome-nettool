pixmapsdir = $(datadir)/gnome-nettool/pixmaps
pixmaps_DATA = 			\
	16_ethernet.xpm	 	\
	16_loopback.xpm	 	\
	16_plip.xpm	 	\
	16_ppp.xpm	 	\
	irda-16.png	 	\
	network.png	 	\
	wavelan-16.png

icondir = $(datadir)/icons/hicolor/48x48/apps
icon_DATA = gnome-nettool.png

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

EXTRA_DIST = $(pixmaps_DATA)	 \
	     $(icon_DATA)
