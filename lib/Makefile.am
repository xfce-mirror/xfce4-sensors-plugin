AM_CPPFLAGS =										\
	-I$(top_srcdir)/include

plugindir = $(libdir)/xfce4/modules

plugin_LTLIBRARIES = libxfce4sensors.la

if HAVE_LIBSENSORS
SENSORS_SOURCES = lmsensors.c
endif

if HAVE_HDDTEMP
HDDTEMP_SOURCES = hddtemp.c
endif

if HAVE_ACPI
ACPI_SOURCES = acpi.c
endif

if HAVE_NVIDIA
NVIDIA_SOURCES = nvidia.c
endif

libxfce4sensors_la_SOURCES =						\
	middlelayer.c									\
	configuration.c									\
	sensors-interface-common.c						\
	sensors-interface.c								\
	cpu.c											\
	$(SENSORS_SOURCES)								\
	$(HDDTEMP_SOURCES)								\
	$(ACPI_SOURCES)									\
	$(NVIDIA_SOURCES)

libxfce4sensors_la_CFLAGS =							\
	@PLATFORM_CFLAGS@								\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"			\
	@LIBXFCE4UI_CFLAGS@								\
	@LIBXFCE4PANEL_CFLAGS@							\
	@LIBSENSORS_CFLAGS@

if HAVE_NVIDIA
libxfce4sensors_la_CFLAGS += @NVIDIA_CFLAGS@
endif

if HAVE_LIBNOTIFY4
libxfce4sensors_la_CFLAGS += @LIBNOTIFY4_CFLAGS@
endif
if HAVE_LIBNOTIFY7
libxfce4sensors_la_CFLAGS += @LIBNOTIFY7_CFLAGS@
endif

libxfce4sensors_la_LDFLAGS =						\
	-export-dynamic									\
	-export-symbols-regex "^[^_].*"					\
	-version-info $(LIBXFCE4SENSORS_VERINFO)

# @PLATFORM_LDFLAGS@

libxfce4sensors_la_LIBADD =							\
	@LIBXFCE4UI_LIBS@								\
	@LIBXFCE4PANEL_LIBS@							\
	@LIBSENSORS_LDFLAGS@							\
	@LIBM@

if HAVE_NVIDIA
libxfce4sensors_la_LIBADD += @NVIDIA_LIBS@
endif

if HAVE_LIBNOTIFY4
libxfce4sensors_la_LIBADD += @LIBNOTIFY4_LIBS@
endif
if HAVE_LIBNOTIFY7
libxfce4sensors_la_LIBADD += @LIBNOTIFY7_LIBS@
endif

EXTRA_DIST =										\
	helpers.c

DISTCLEANFILES =

##
## Pkg-config information
##
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxfce4sensors-1.0.pc
