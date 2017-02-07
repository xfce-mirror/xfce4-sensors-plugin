# $Id$

AM_CPPFLAGS = \
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/lib

bin_PROGRAMS = \
	xfce4-sensors

xfce4_sensors_SOURCES = \
	main.c \
	interface.c \
	interface.h \
	callbacks.c \
	callbacks.h \
	actions.c \
	actions.h

xfce4_sensors_CFLAGS = \
	@PLATFORM_CFLAGS@ \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	@LIBSENSORS_CFLAGS@	\
	@LIBXFCE4UI_CFLAGS@ \
	@LIBXFCE4PANEL_CFLAGS@

xfce4_sensors_LDFLAGS = \
	@PLATFORM_LDFLAGS@

xfce4_sensors_LDADD = \
	$(top_builddir)/lib/libxfce4sensors.la \
	@LIBSENSORS_LDFLAGS@ \
	@LIBXFCE4UI_LIBS@	\
	@LIBM@

xfce4_sensors_DEPENDENCIES = \
	$(top_builddir)/lib/libxfce4sensors.la

# .desktop file
desktop_in_files = xfce4-sensors.desktop.in

desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	$(desktop_in_files)

DISTCLEANFILES = \
	$(desktop_DATA)
