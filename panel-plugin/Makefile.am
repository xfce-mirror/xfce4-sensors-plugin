plugindir = $(libexecdir)/xfce4/panel-plugins
plugin_PROGRAMS = xfce4-sensors-plugin

AM_CPPFLAGS = 			\
	-I$(top_srcdir)/include 	\
	-I$(top_srcdir)/lib

xfce4_sensors_plugin_SOURCES =									   \
	sensors-plugin.c															\
	sensors-plugin.h

xfce4_sensors_plugin_CFLAGS = \
	@LIBXFCE4PANEL_CFLAGS@										   \
	@LIBXFCE4UI_CFLAGS@										   \
	@PLATFORM_CFLAGS@												\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"							\
	@LIBSENSORS_CFLAGS@

xfce4_sensors_plugin_LDADD = 							\
	$(top_builddir)/lib/libxfce4sensors.la		\
	@LIBXFCE4PANEL_LIBS@											\
	@LIBXFCE4UI_LIBS@												\
	@SOLLIBS@																	\
	@LIBSENSORS_LDFLAGS@											\
	@LIBM@ 

# .desktop file
#
# Some automake trickery here. Because we cannot use $(libexecdir) in the
# automake stage, we'll use sed to get the full path into the .desktop file.
# We also need to let intltool merge the translated fields, so we add an
# additional level of indirection: a <name>.desktop.in.in file.
#
desktop_in_in_files = xfce4-sensors-plugin.desktop.in.in
desktop_in_files = $(desktop_in_in_files:.desktop.in.in=.desktop.in)

desktopdir = $(datadir)/xfce4/panel-plugins
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =														 \
	$(desktop_in_in_files)

DISTCLEANFILES =													 \
	$(desktop_DATA) $(desktop_in_files)

# get full path into .desktop file
%.desktop.in: %.desktop.in.in
	sed -e "s^@PLUGIN_PATH@^$(libexecdir)/xfce4/panel-plugins^" \
		$< > $@
