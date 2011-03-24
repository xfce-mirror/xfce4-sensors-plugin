# $Id$

INCLUDES =														  	\
	-I$(top_srcdir)/include

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

if HAVE_NVIDIA
NVIDIA_SOURCES = nvidia.c
#nvidia.c
endif

libxfce4sensors_la_SOURCES =										\
	middlelayer.c													\
	configuration.c													\
	sensors-interface-common.c										\
	sensors-interface.c												\
	cpu.c																			\
	$(SENSORS_SOURCES)												\
	$(HDDTEMP_SOURCES)												\
	$(ACPI_SOURCES) \
	$(NVIDIA_SOURCES)

# configuration.h													\
# sensors-plugin.h													\
# types.h															\
# middlelayer.h

libxfce4sensors_la_CFLAGS = 										\
	@PLATFORM_CFLAGS@												\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"							\
	@GTK_CFLAGS@													\
	@GLIB_CFLAGS@													\
	@LIBXFCEGUI4_CFLAGS@											\
	@LIBXFCE4UTIL_CFLAGS@											\
	@LIBXFCE4PANEL_CFLAGS@										\
	@LIBSENSORS_CFLAGS@

if HAVE_NVIDIA
libxfce4sensors_la_CFLAGS += @NVIDIA_CFLAGS@
endif

if HAVE_LIBNOTIFY
libxfce4sensors_la_CFLAGS += @LIBNOTIFY_CFLAGS@
endif

# @GTK_CFLAGS@ 														\
# @GLIB_CFLAGS@ 													\
#	@LIBXFCE4UTIL_CFLAGS@											\
#	@LIBXFCEGUI4_CFLAGS@

libxfce4sensors_la_LDFLAGS = 										\
	-export-dynamic													\
	-export-symbols-regex "^[^_].*"									\
	-version-info $(LIBXFCE4SENSORS_VERINFO)

#	@PLATFORM_LDFLAGS@

#	-export-dynamic 												\
#	-avoid-version 													\
#	-module

libxfce4sensors_la_LIBADD = 										\
	@GTK_LIBS@														\
	@GLIB_LIBS@														\
	@LIBXFCE4UTIL_LIBS@												\
	@LIBXFCEGUI4_LIBS@												\
	@LIBXFCE4PANEL_LIBS@											\
	@LIBSENSORS_LDFLAGS@											\
	@LIBM@

if HAVE_NVIDIA
libxfce4sensors_la_LIBADD += @NVIDIA_LIBS@
endif

if HAVE_LIBNOTIFY
libxfce4sensors_la_LIBADD += @LIBNOTIFY_LIBS@
endif

# @GTK_LIBS@ 														\
#	@GLIB_LIBS@ 													\
#	@LIBXFCE4UTIL_LIBS@												\
#	@LIBXFCEGUI4_LIBS@

#@SOLLIBS@

# -fPIC -shared

#	-DG_LOG_DOMAIN=\"xfce4-sensors-plugin\"						 	\
#	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

EXTRA_DIST =														\
	helpers.c

DISTCLEANFILES =

##
## Pkg-config information
##
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libxfce4sensors-1.0.pc
