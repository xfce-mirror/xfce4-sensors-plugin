# $Id:$

plugindir = $(libdir)/xfce4/modules

plugin_LTLIBRARIES = libxfce4sensors.la

if HAVE_LIBSENSORS
SENSORS_SOURCES = lmsensors.c
# lmsensors.h
endif

if HAVE_HDDTEMP
HDDTEMP_SOURCES = hddtemp.c
#hddtemp.h
endif

if HAVE_ACPI
ACPI_SOURCES = acpi.c
#acpi.h
endif

libxfce4sensors_la_SOURCES =									   \
	middlelayer.c													\
	configuration.c													\
	sensors-interface-common.c						\
	sensors-interface.c										\
	$(SENSORS_SOURCES)												\
	$(HDDTEMP_SOURCES)												\
	$(ACPI_SOURCES)

# configuration.h													\
# sensors-plugin.h
# types.h																	\
# middlelayer.h

INCLUDES =														  \
	-I$(top_srcdir)/include

libxfce4sensors_la_CFLAGS = \
	@GTK_CFLAGS@ \
	@GLIB_CFLAGS@ \
	@PLATFORM_CFLAGS@												\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"							\
	@LIBSENSORS_CFLAGS@										\
	@LIBXFCE4UTIL_CFLAGS@							\
	@LIBXFCEGUI4_CFLAGS@								\
	@LIBXFCE4PANEL_CFLAGS@


libxfce4sensors_la_LDFLAGS = \
	-export-dynamic							\
	-export-symbols-regex "^[^_].*"					\
	-version-info $(LIBXFCE4SENSORS_VERINFO)

#	@PLATFORM_LDFLAGS@

#	-export-dynamic \
#	-avoid-version \
#	-module

libxfce4sensors_la_LIBADD = \
	@GTK_LIBS@ \
	@GLIB_LIBS@ \
	@LIBSENSORS_LDFLAGS@									\
	@LIBXFCE4UTIL_LIBS@								\
	@LIBXFCEGUI4_LIBS@								\
	@LIBXFCE4PANEL_LIBS@

#@SOLLIBS@

# -fPIC -shared

#	-DG_LOG_DOMAIN=\"xfce4-sensors-plugin\"						 \
#	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

DISTCLEANFILES =

##
## Pkg-config information
##
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxfce4sensors-1.0.pc
