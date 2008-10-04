# $Id$

INCLUDES = -I../panel-plugin/

bin_PROGRAMS = xfce4-sensors

# LIBS = @LIBS@ @SOLLIBS@
# CFLAGS = @CFLAGS@

xfce4_sensors_SOURCES =									   \
	main.c \
	interface.c \
	callbacks.c \
	actions.c \
	../panel-plugin/middlelayer.c \
	../panel-plugin/acpi.c \
	../panel-plugin/hddtemp.c \
	../panel-plugin/lmsensors.c

xfce4_sensors_CFLAGS = \
	$(GTK_CFLAGS) \
	@PLATFORM_CFLAGS@												\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"							\
	@LIBSENSORS_CFLAGS@
#	@LIBXFCE4PANEL_CFLAGS@

xfce4_sensors_LDFLAGS = \
	$(GTK_LIBS) \
	@SOLLIBS@														\
	@LIBSENSORS_LDFLAGS@
#	@LIBXFCE4PANEL_LIBS@

# INCLUDES =														 \
#	-DG_LOG_DOMAIN=\"xfce4-sensors-plugin\"						 \
#	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

# .desktop file
#
# Some automake trickery here. Because we cannot use $(libexecdir) in the
# automake stage, we'll use sed to get the full path into the .desktop file.
# We also need to let intltool merge the translated fields, so we add an
# additional level of indirection: a <name>.desktop.in.in file.
#
desktop_in_in_files = xfce4-sensors.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)

desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =														 \
	$(desktop_in_in_files)


DISTCLEANFILES =													 \
	$(desktop_DATA) $(desktop_in_files)
