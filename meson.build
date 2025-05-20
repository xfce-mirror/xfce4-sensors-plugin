project(
  'xfce4-sensors-plugin',
  'c',
  'cpp',
version : '1.5.0-dev',
  license : 'GPL-2.0-or-later',
  meson_version : '>= 0.54.0',
  default_options : ['c_std=gnu11', 'cpp_std=gnu++11', 'buildtype=debugoptimized', 'warning_level=2']
)

project_namespace = 'panel-plugins'
pkgdatadir = get_option('prefix') / get_option('datadir') / meson.project_name()
copyright_year = '2025'

cc = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')
pkgconfig = import('pkgconfig')
gnome = import('gnome')
i18n = import('i18n')
fs = import('fs')

dependency_versions = {
  'glib': '>= 2.50.0',
  'gtk': '>= 3.22.0',
  'xfce4': '>= 4.16.0',
  'libxfce4util': '>= 4.17.2',
  'libnotify': '>= 0.7',
}

glib = dependency('glib-2.0', version: dependency_versions['glib'])
gtk = dependency('gtk+-3.0', version: dependency_versions['gtk'])
libxfce4panel = dependency('libxfce4panel-2.0', version: dependency_versions['xfce4'])
libxfce4ui = dependency('libxfce4ui-2', version: dependency_versions['xfce4'])
libxfce4util = dependency('libxfce4util-1.0', version: dependency_versions['libxfce4util'])
libnotify = dependency('libnotify', version: dependency_versions['libnotify'], required: get_option('libnotify'))
libsensors = cc.find_library('sensors', required: get_option('libsensors'))
libm = cc.find_library('m')
libsocket = cc.find_library('socket', required: false)

xnvctrl_deps = []
xnvctrl_deps += cc.find_library('XNVCtrl', required: get_option('xnvctrl'))
xnvctrl_deps += dependency('x11', required: get_option('xnvctrl'))
xnvctrl_deps += dependency('xext', required: get_option('xnvctrl'))

hddtemp = get_option('hddtemp-path')
if hddtemp == ''
  hddtemp_exe = find_program('hddtemp', required: get_option('hddtemp'))
  if hddtemp_exe.found()
    hddtemp = hddtemp_exe.path()
  endif
endif
netcat = get_option('netcat-path')
if netcat == ''
  netcat_exe = find_program(['netcat', 'nc'], required: get_option('netcat'))
  if netcat_exe.found()
    netcat = netcat_exe.path()
  endif
endif

feature_cflags = []

if cc.has_function('malloc_trim', prefix: '#include <malloc.h>')
  feature_cflags += '-DHAVE_MALLOC_TRIM=1'
endif

have_libnotify = libnotify.found()
if have_libnotify
  feature_cflags += '-DHAVE_LIBNOTIFY=1'
endif

have_libsensors = libsensors.found()
if have_libsensors
  feature_cflags += '-DHAVE_LIBSENSORS=1'
endif

have_hddtemp = hddtemp != '' and not get_option('hddtemp').disabled()
if have_hddtemp
  feature_cflags += [
    '-DHAVE_HDDTEMP=1',
    '-DPATH_HDDTEMP="@0@"'.format(hddtemp),
  ]
endif

have_netcat = netcat != '' and not get_option('netcat').disabled()
if have_netcat
  feature_cflags += [
    '-DHAVE_NETCAT=1',
    '-DNETCAT_PATH="@0@"'.format(netcat),
    '-DHDDTEMP_PORT=7634',
  ]
endif

have_procacpi = get_option('procacpi').enabled()
if get_option('procacpi').auto()
  have_procacpi = fs.exists('/proc/acpi/info')
endif
if have_procacpi
  feature_cflags += '-DHAVE_ACPI=1'
endif

have_sysfsacpi = get_option('sysfsacpi').enabled()
if get_option('sysfsacpi').auto()
  have_sysfsacpi = fs.exists('/sys/class/power_supply')
endif
if have_sysfsacpi
  feature_cflags += '-DHAVE_SYSFS_ACPI=1'
endif

have_xnvctrl = not get_option('xnvctrl').disabled()
foreach dep : xnvctrl_deps
  have_xnvctrl = have_xnvctrl and dep.found()
endforeach
if have_xnvctrl
  feature_cflags += '-DHAVE_NVIDIA=1'
endif

extra_cflags = []
extra_cxxflags_check = [
  '-Wmissing-declarations',
  '-Wmissing-noreturn',
  '-Wredundant-decls',
  '-Wpointer-arith',
  '-Wcast-align',
  '-Winit-self',
  '-Wshadow',
  '-Wmissing-include-dirs',
  '-Wundef',
  '-Wformat',
  '-Wformat-security',
  '-Wformat-y2k',
  '-Wno-unused-parameter',
  '-Wno-missing-field-initializers',
  '-Wno-error=deprecated-declarations',
]

optimization = get_option('optimization')
if get_option('debug') and optimization in ['0', 'g']
  extra_cxxflags_check += '-fstack-protector-strong'
  extra_cflags += [
    '-DDEBUG=1',
    '-DDEBUG_TRACE=1',
    '-DG_ENABLE_DEBUG',
  ]
elif optimization in ['3', 'minsize']
  extra_cflags += [
    '-DNDEBUG',
    '-DG_DISABLE_CAST_CHECKS',
    '-DG_DISABLE_ASSERT',
  ]
endif

if dependency_versions.has_key('glib')
  glib_version_parts = dependency_versions['glib'].split(' ')
  glib_min_version_parts = glib_version_parts[1].split('.')
  glib_min_version_define = 'GLIB_VERSION_@0@_@1@'.format(glib_min_version_parts[0], glib_min_version_parts[1])
  extra_cflags += [
    '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_min_version_define),
    '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_min_version_define),
    '-DG_LOG_DOMAIN="@0@"'.format(meson.project_name()),
    '-DG_LOG_USE_STRUCTURED=1',
  ]
endif

extra_cflags_check = extra_cxxflags_check + [
  '-Wold-style-definition',
  '-Wnested-externs',
  '-Wno-declaration-after-statement',
  '-Werror=implicit-function-declaration'
]

version_parts = meson.project_version().split('-dev')[0].split('.')
version_short = '@0@.@1@'.format(version_parts[0], version_parts[1])

extra_cflags += [
  '-DPACKAGE="@0@"'.format(meson.project_name()),
  '-DPACKAGE_NAME="@0@"'.format(meson.project_name()),
  '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
  '-DVERSION="@0@"'.format(meson.project_version()),
  '-DVERSION_SHORT="@0@"'.format(version_short),
  '-DPACKAGE_STRING="@0@ @1@"'.format(meson.project_name(), meson.project_version()),
  '-DPACKAGE_DATADIR="@0@"'.format(pkgdatadir),
  '-DCOPYRIGHT_YEAR="@0@"'.format(copyright_year),
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DPACKAGE_BUGREPORT="https://gitlab.xfce.org/@0@/@1@/-/issues"'.format(project_namespace, meson.project_name()),
  '-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name()),
  '-DPREFIX="@0@"'.format(get_option('prefix')),
  '-DBINDIR="@0@"'.format(get_option('prefix') / get_option('bindir')),
  '-DDATADIR="@0@"'.format(get_option('prefix') / get_option('datadir')),
  '-DINCLUDEDIR="@0@"'.format(get_option('prefix') / get_option('includedir')),
  '-DLIBDIR="@0@"'.format(get_option('prefix') / get_option('libdir')),
  '-DLIBEXECDIR="@0@"'.format(get_option('prefix') / get_option('libexecdir')),
  '-DLOCALEDIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DLOCALSTATEDIR="@0@"'.format(get_option('prefix') / get_option('localstatedir')),
  '-DSBINDIR="@0@"'.format(get_option('prefix') / get_option('sbindir')),
  '-DSYSCONFDIR="@0@"'.format(get_option('prefix') / get_option('sysconfdir')),
  '-DHAVE_XFCE_REVISION_H=1',
]

add_project_arguments(cc.get_supported_arguments(extra_cflags_check), language: 'c')
add_project_arguments(cxx.get_supported_arguments(extra_cxxflags_check), language: 'cpp')
add_project_arguments(extra_cflags, language: ['c', 'cpp'])
add_project_arguments(feature_cflags, language: ['c', 'cpp'])

xfce_revision_h = vcs_tag(
  command: ['git', 'rev-parse', '--short', 'HEAD'],
  fallback: 'UNKNOWN',
  input: 'xfce-revision.h.in',
  output: 'xfce-revision.h',
  replace_string: '@REVISION@',
)

subdir('xfce4++')
subdir('lib')
subdir('src')
subdir('panel-plugin')
subdir('icons')
subdir('po')
