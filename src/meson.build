xfce4_sensors_sources = [
  'actions.cc',
  'actions.h',
  'callbacks.cc',
  'callbacks.h',
  'interface.cc',
  'interface.h',
  'main.cc',
  xfce_revision_h,
]

xfce4_sensors = executable(
  'xfce4-sensors',
  xfce4_sensors_sources,
  include_directories: [
    include_directories('..'),
    include_directories('..' / 'include'),
  ],
  dependencies: [
    glib,
    gtk,
    libsensors,
    libxfce4panel,
    libxfce4ui,
    libxfce4util,
  ],
  link_with: [
    libxfce4sensors,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
)

install_man('xfce4-sensors.1')

i18n.merge_file(
  input: 'xfce4-sensors.desktop.in',
  output: 'xfce4-sensors.desktop',
  po_dir: '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / 'applications',
)
