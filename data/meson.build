pkg = meson.project_name()
prefix = get_option('prefix')
datadir = get_option('datadir')
localedir = get_option('localedir')

ui_dir = join_paths(prefix, datadir, pkg, 'ui', '') # want trailing slash
pixmaps_dir = join_paths(prefix, datadir, pkg, 'pixmaps')

cdata.set_quoted('GNOME_NETTOOL_LOCALEDIR', join_paths(prefix, localedir))
cdata.set_quoted('PIXMAPS_DIR', pixmaps_dir)
cdata.set_quoted('UI_DIR', ui_dir)

install_data('gnome-nettool.ui', install_dir: ui_dir)

desktop_conf = configuration_data()
desktop_conf.set('PACKAGE_VERSION', meson.project_version())

# gnome-nettool.desktop.in.meson has leading _ removed for Name, Comment, Keywords
desktop_in = configure_file(input: 'gnome-nettool.desktop.in.meson',
                            output: 'gnome-nettool.desktop.in',
                            configuration: desktop_conf)

i18n.merge_file('desktop',
                type: 'desktop',
                input: desktop_in,
                output: 'gnome-nettool.desktop',
                po_dir: join_paths(meson.current_source_dir(), '../po'),
                install: true,
                install_dir: join_paths(prefix, datadir, 'applications'))

# FIXME: not sure how to do things with the orignal .in file with translatable xml tags
# Apparently very very recent gettext (git) has gschema support.
# Also: https://github.com/mesonbuild/meson/pull/1757
configure_file(input: 'org.gnome.gnome-nettool.gschema.xml.meson',
               output: 'org.gnome.gnome-nettool.gschema.xml',
               configuration: configuration_data(),
               install: true,
               install_dir: join_paths(prefix, datadir, 'glib-2.0', 'schemas'))
