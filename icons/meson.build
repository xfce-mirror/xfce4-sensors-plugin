sizes = [24, 32, 48, 64]

foreach size : sizes
  install_data(
    '@0@x@0@'.format(size) / 'xfce-sensors.png',
    install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / '@0@x@0@'.format(size) / 'apps',
  )
endforeach

install_data(
  'scalable' / 'xfce-sensors.svg',
  install_dir: get_option('prefix') / get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
)
