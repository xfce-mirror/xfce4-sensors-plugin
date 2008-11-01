# $Id:$

INCLUDES = 			\
	-I$(top_srcdir)/include	\
	-I$(top_srcdir)/lib

bin_PROGRAMS = \
	xfce4-sensors

#	-DG_LOG_DOMAIN=\"xfce4-sensors-plugin\"						 \
#	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"


# LIBS = @LIBS@ @SOLLIBS@
# CFLAGS = @CFLAGS@

xfce4_sensors_SOURCES =									   \
	main.c \
	interface.c \
	interface.h \
	callbacks.c \
	callbacks.h \
	actions.c	\
	actions.h

xfce4_sensors_CFLAGS = \
	@PLATFORM_CFLAGS@											\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"						\
	@LIBSENSORS_CFLAGS@											\
	@LIBXFCE4UTIL_CFLAGS@ \
	@LIBXFCEGUI4_CFLAGS@

# @GTK_CFLAGS@ 												\
# @GLIB_CFLAGS@ 												\
#	@LIBXFCE4PANEL_CFLAGS@

xfce4_sensors_LDFLAGS = 								\
	-no-undefined								\
	@PLATFORM_LDFLAGS@

xfce4_sensors_LDADD = 								\
	$(top_builddir)/lib/libxfce4sensors.la \
	@LIBSENSORS_LDFLAGS@ 								\
	@LIBXFCE4UTIL_LIBS@									\
	@LIBXFCEGUI4_LIBS@

# @GTK_LIBS@ 													\
# @GLIB_LIBS@ 													\
#-L/usr/local/lib/xfce4/modules/ -L$(top_builddir)/lib -lxfce4sensors
#	@LIBXFCE4PANEL_LIBS@

xfce4_sensors_DEPENDENCIES = \
	$(top_builddir)/lib/libxfce4sensors.la

# .desktop file
desktop_in_files = xfce4-sensors.desktop.in
#desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)

desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =														 \
	$(desktop_in_files)

DISTCLEANFILES =													 \
	$(desktop_DATA)

# get full path into .desktop file
#%.desktop.in: %.desktop.in.in
#	sed -e "s^@PLUGIN_PATH@^$(libexecdir)/xfce4/panel-plugins^" \
#		$< > $@
