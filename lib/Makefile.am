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
	@LIBXFCE4UTIL_CFLAGS@


libxfce4sensors_la_LDFLAGS = \
	-export-dynamic \
	-avoid-version \
	-module

libxfce4sensors_la_LIBADD = \
	@GTK_LIBS@ \
	@GLIB_LIBS@ \
	@SOLLIBS@														\
	@LIBSENSORS_LDFLAGS@									\
	@LIBXFCE4UTIL_LIBS@

# -fPIC -shared

#	-DG_LOG_DOMAIN=\"xfce4-sensors-plugin\"						 \
#	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

DISTCLEANFILES =

