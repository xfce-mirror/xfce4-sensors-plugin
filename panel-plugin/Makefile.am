INCLUDES =								\
	-DG_LOG_DOMAIN=\"xfce4-sensors-plugin\"  \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

plugindir = $(libdir)/xfce4/panel-plugins
plugin_LTLIBRARIES =							\
	libsensors.la

libsensors_la_LDFLAGS =							\
	-avoid-version							\
	-module

libsensors_la_SOURCES =							\
	sensors.h							\
	sensors.c

libsensors_la_CFLAGS =							\
	@XFCE4_PANEL_CFLAGS@

libsensors_la_LIBADD = 							\
	@XFCE4_PANEL_LIBS@						\
	-lsensors

