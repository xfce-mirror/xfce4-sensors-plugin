AM_CPPFLAGS = \
	-I${top_srcdir} \
	-I$(top_srcdir)/include

plugindir = $(libdir)/xfce4/modules

plugin_LTLIBRARIES = libxfce4sensors.la

if HAVE_LIBSENSORS
SENSORS_SOURCES = lmsensors.cc
endif

if HAVE_HDDTEMP
HDDTEMP_SOURCES = hddtemp.cc
endif

if HAVE_ACPI
ACPI_SOURCES = acpi.cc
endif

if HAVE_NVIDIA
NVIDIA_SOURCES = nvidia.cc
endif

libxfce4sensors_la_SOURCES = \
	configuration.cc \
	middlelayer.cc \
	sensors-interface.cc \
	sensors-interface-common.cc \
	tacho.cc \
	$(ACPI_SOURCES) \
	$(HDDTEMP_SOURCES) \
	$(NVIDIA_SOURCES) \
	$(SENSORS_SOURCES)

libxfce4sensors_la_CFLAGS = \
	@PLATFORM_CFLAGS@ \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	@LIBXFCE4UI_CFLAGS@ \
	@LIBXFCE4PANEL_CFLAGS@ \
	@LIBSENSORS_CFLAGS@

if HAVE_NVIDIA
libxfce4sensors_la_CFLAGS += @NVIDIA_CFLAGS@
endif

if HAVE_LIBNOTIFY
libxfce4sensors_la_CFLAGS += @LIBNOTIFY_CFLAGS@
endif

libxfce4sensors_la_CXXFLAGS = $(libxfce4sensors_la_CFLAGS)

libxfce4sensors_la_LDFLAGS = \
	-version-info $(LIBXFCE4SENSORS_VERINFO)

# @PLATFORM_LDFLAGS@

libxfce4sensors_la_LIBADD = \
	$(top_builddir)/xfce4++/util/libxfce4util_pp.la \
	@LIBXFCE4UI_LIBS@ \
	@LIBXFCE4PANEL_LIBS@ \
	@LIBSENSORS_LDFLAGS@ \
	-lm

if HAVE_NVIDIA
libxfce4sensors_la_LIBADD += @NVIDIA_LIBS@
endif

if HAVE_LIBNOTIFY
libxfce4sensors_la_LIBADD += @LIBNOTIFY_LIBS@
endif

DISTCLEANFILES =

##
## Pkg-config information
##
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxfce4sensors-1.0.pc
