project(
  'gnome-nettool',
  'c',
  version: '42.0',
  default_options: [
    'buildtype=debugoptimized',
    'warning_level=1',
  ],
  meson_version: '>= 0.40.1',
  license: 'GPLv2.0+',
)

gio_req = '>= 2.26'
gtk_req = '>= 3.0.0'

cc = meson.get_compiler('c')

# config.h configuration data
cdata = configuration_data()
cdata.set_quoted('GETTEXT_PACKAGE', 'gnome-nettool')
cdata.set_quoted('PACKAGE_VERSION', meson.project_version())
cdata.set_quoted('PACKAGE_URL', 'http://projects.gnome.org/gnome-network/')

add_project_arguments('-DHAVE_CONFIG_H', language: 'c')

if cc.has_argument('-Wno-deprecated-declarations')
  add_project_arguments('-Wno-deprecated-declarations', language: 'c')
endif

config_inc = include_directories('.')

# Check headers
cdata.set('HAVE_SYS_SOCKIO_H', cc.has_header('sys/sockio.h'))

req_headers = [
  'sys/socket.h',
  'ifaddrs.h',
]
foreach req_header : req_headers
  if not cc.has_header(req_header)
    error('Required header not found: ' + req_header)
  endif
endforeach

gtk_dep = dependency('gtk+-3.0', version: gtk_req)
gio_dep = dependency('gio-2.0', version: gio_req)
gtop_dep = dependency('libgtop-2.0')
gmodule_dep = dependency('gmodule-export-2.0')

nettool_deps = [
  gtk_dep,
  gio_dep,
  gmodule_dep,
  gtop_dep,
]

po_dir = join_paths(meson.source_root(), 'po')

subdir('src')
subdir('po')
subdir('data')
subdir('help')
subdir('pixmaps')

# write config.h
configure_file(
  output: 'config.h',
  configuration: cdata,
)

meson.add_install_script('postinstall.py')
