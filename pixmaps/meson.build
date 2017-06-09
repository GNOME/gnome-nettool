pixmaps = [
  '16_ethernet.xpm',
  '16_loopback.xpm',
  '16_plip.xpm',
  '16_ppp.xpm',
  'irda-16.png',
  'network.png',
  'gnome-nettool.png',
  'wavelan-16.png',
]
pixmaps_dir = join_paths(prefix, datadir, pkg, 'pixmaps')
install_data(pixmaps, install_dir: pixmaps_dir)

icons = [
  ['16x16', files('icons/16x16/apps/gnome-nettool.png')],
  ['22x22', files('icons/22x22/apps/gnome-nettool.png')],
  ['24x24', files('icons/24x24/apps/gnome-nettool.png')],
  ['32x32', files('icons/32x32/apps/gnome-nettool.png')],
  ['scalable', files('icons/scalable/apps/gnome-nettool.svg')],
  ['scalable', files('icons/scalable/apps/gnome-nettool-symbolic.svg')],
]

foreach icon : icons
  size = icon[0]
  file = icon[1]
  iconsdir = join_paths(prefix, datadir, 'icons', 'hicolor', size, 'apps')
  install_data(file, install_dir: iconsdir)
endforeach
