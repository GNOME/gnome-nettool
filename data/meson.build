pkg = meson.project_name()
prefix = get_option('prefix')
datadir = get_option('datadir')
localedir = get_option('localedir')

ui_dir = join_paths(prefix, datadir, pkg, 'ui', '') # want trailing slash
pixmaps_dir = join_paths(prefix, datadir, pkg, 'pixmaps')
schemas_dir = join_paths(prefix, datadir, 'glib-2.0', 'schemas')

cdata.set_quoted('GNOME_NETTOOL_LOCALEDIR', join_paths(prefix, localedir))
cdata.set_quoted('PIXMAPS_DIR', pixmaps_dir)
cdata.set_quoted('UI_DIR', ui_dir)

install_data('gnome-nettool.ui', install_dir: ui_dir)

desktop_conf = configuration_data()
desktop_conf.set('PACKAGE_VERSION', meson.project_version())

desktop_in = configure_file(input: 'gnome-nettool.desktop.in.in',
                            output: 'gnome-nettool.desktop.in',
                            configuration: desktop_conf)

i18n.merge_file('desktop',
                type: 'desktop',
                input: desktop_in,
                output: 'gnome-nettool.desktop',
                po_dir: po_dir,
                install: true,
                install_dir: join_paths(prefix, datadir, 'applications'))

i18n.merge_file('appdata',
                type: 'xml',
                input: 'gnome-nettool.appdata.xml.in',
                output: 'gnome-nettool.appdata.xml',
                po_dir: po_dir,
                install: true,
                install_dir: join_paths(prefix, datadir, 'appdata'),
)

install_data('org.gnome.gnome-nettool.gschema.xml', install_dir: schemas_dir)
