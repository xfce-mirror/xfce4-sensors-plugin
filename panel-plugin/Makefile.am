plugindir = $(libexecdir)/xfce4/panel-plugins
plugin_PROGRAMS = xfce4-sensors-plugin

LIBS = @LIBS@ @SOLLIBS@

xfce4_sensors_plugin_SOURCES =                              		 \
    sensors.c                                           		     \
	sensors.h                                           

xfce4_sensors_plugin_CFLAGS =  		                                 \
     -DPACKAGE_LOCALE_DIR=\"$(localedir)\"                           \ 
     @LIBXFCE4PANEL_CFLAGS@             

xfce4_sensors_plugin_LDFLAGS =                                       \
    @SOLLIBS@                                   			         \
	@LIBXFCE4PANEL_LIBS@                                             \
	-lsensors

# INCLUDES =								                             \
#	-DG_LOG_DOMAIN=\"xfce4-sensors-plugin\"                          \
#	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

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

DISTCLEANFILES =							                         \
	$(desktop_DATA) $(desktop_in_files)
	
# get full path into .desktop file
%.desktop.in: %.desktop.in.in
	sed -e "s^@PLUGIN_PATH@^$(libexecdir)/xfce4/panel-plugins^" 	 \
		$< > $@
