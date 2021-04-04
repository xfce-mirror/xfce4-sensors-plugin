plugindir = $(libdir)/xfce4/panel/plugins
plugin_LTLIBRARIES = libxfce4-sensors-plugin.la

AM_CPPFLAGS =			 											\
	-I$(top_srcdir)/include											\
	-I$(top_srcdir)/lib

libxfce4_sensors_plugin_la_SOURCES =								\
	sensors-plugin.c												\
	sensors-plugin.h

libxfce4_sensors_plugin_la_CFLAGS = 								\
	@LIBXFCE4PANEL_CFLAGS@										 	\
	@LIBXFCE4UI_CFLAGS@												\
	@PLATFORM_CFLAGS@												\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"							\
	-DDATADIR=\"$(datadir)\"                                        \
	@LIBSENSORS_CFLAGS@

libxfce4_sensors_plugin_la_LIBADD =								 	\
	$(top_builddir)/lib/libxfce4sensors.la							\
	@LIBXFCE4PANEL_LIBS@											\
	@LIBXFCE4UI_LIBS@												\
	@SOLLIBS@														\
	@LIBSENSORS_LDFLAGS@

libxfce4_sensors_plugin_la_LDFLAGS = 								\
	-avoid-version 													\
	-module 														\
	-no-undefined 													\
	-export-dynamic													\
	-export-symbols-regex "^[^_].*"									\
	$(PLATFORM_LDFLAGS)

# .desktop file
desktop_in_files = xfce4-sensors-plugin.desktop.in

desktopdir = $(datadir)/xfce4/panel/plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

cssfiledir = $(desktopdir)
cssfile_DATA = xfce4-sensors-plugin.css

EXTRA_DIST =														\
	$(desktop_in_files)												\
	$(cssfile_DATA)

DISTCLEANFILES =													\
	$(desktop_DATA)
