nettool_sources = [
  'callbacks.c',
  'nettool.c',
  'gn-combo-history.c',
  'ping.c',
  'traceroute.c',
  'netstat.c',
  'info.c',
  'scan.c',
  'lookup.c',
  'finger.c',
  'whois.c',
  'utils.c',
  'util-mii.c',
  'main.c',
]

executable('gnome-nettool', nettool_sources,
           include_directories: config_inc,
           dependencies: nettool_deps,
           install: true)
