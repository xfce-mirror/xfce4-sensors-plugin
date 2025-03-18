xfce4sensors_sources = [
  'configuration.cc',
  'middlelayer.cc',
  'sensors-interface.cc',
  'sensors-interface-common.cc',
  'tacho.cc',
]

if have_libsensors
  xfce4sensors_sources += 'lmsensors.cc'
endif
if have_hddtemp
  xfce4sensors_sources += 'hddtemp.cc'
endif
if have_procacpi
  xfce4sensors_sources += 'acpi.cc'
endif
if have_xnvctrl
  xfce4sensors_sources += 'nvidia.cc'
endif

libxfce4sensors = static_library(
  'xfce4sensors',
  xfce4sensors_sources,
  include_directories: [
    include_directories('..'),
    include_directories('..' / 'include'),
  ],
  dependencies: [
    glib,
    gtk,
    libnotify,
    libm,
    libsensors,
    libxfce4panel,
    libxfce4ui,
    libxfce4util,
    xnvctrl_deps,
  ],
  link_with: [
    libxfce4util_pp,
  ],
  install: false,
)
