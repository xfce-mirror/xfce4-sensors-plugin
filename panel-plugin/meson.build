plugin_sources = [
  'plugin.c',
  'plugin.h',
  'sensors-plugin.cc',
  'sensors-plugin.h',
  xfce_revision_h,
]

plugin_install_subdir = 'xfce4' / 'panel' / 'plugins'

plugin_lib = shared_module(
  'xfce4-sensors-plugin',
  plugin_sources,
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('xfce4-sensors-plugin'),
  ],
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
  install_dir: get_option('prefix') / get_option('libdir') / plugin_install_subdir,
)

i18n.merge_file(
  input: 'xfce4-sensors-plugin.desktop.in',
  output: 'xfce4-sensors-plugin.desktop',
  po_dir: '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)

install_data(
  'xfce4-sensors-plugin.css',
  install_dir: get_option('prefix') / get_option('datadir') / plugin_install_subdir,
)
