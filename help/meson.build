gnome = import('gnome')

sources = [
  'index.page',
  'introduction.page',
  'ip-information.page',
  'network-connectivity.page',
]

media = [
  'figures/gnome-nettool-trail.png',
  'figures/gnome-nettool.png',
]

linguas = [
  'ca',
  'cs',
  'da',
  'de',
  'el',
  'en_GB',
  'es',
  'fr',
  'fur',
  'hu',
  'ko',
  'oc',
  'pa',
  'pl',
  'pt_BR',
  'ru',
  'sv',
  'uk',
  'vi',
  'zh_CN',
]

gnome.yelp(meson.project_name(),
           sources: sources,
           media: media,
           symlink_media: true,
           languages: linguas)
