AM_CPPFLAGS = \
	-I${top_srcdir} \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/lib

bin_PROGRAMS = \
	xfce4-sensors

xfce4_sensors_SOURCES = \
	actions.cc \
	actions.h \
	callbacks.cc \
	callbacks.h \
	interface.cc \
	interface.h \
	main.cc

xfce4_sensors_CFLAGS = \
	@PLATFORM_CFLAGS@ \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	@LIBSENSORS_CFLAGS@ \
	@LIBXFCE4UI_CFLAGS@ \
	@LIBXFCE4PANEL_CFLAGS@

xfce4_sensors_CXXFLAGS = $(xfce4_sensors_CFLAGS)

xfce4_sensors_LDFLAGS = \
	@PLATFORM_LDFLAGS@

xfce4_sensors_LDADD = \
	$(top_builddir)/lib/libxfce4sensors.la \
	@LIBSENSORS_LDFLAGS@ \
	@LIBXFCE4UI_LIBS@ \
	-lm

xfce4_sensors_DEPENDENCIES = \
	$(top_builddir)/lib/libxfce4sensors.la

# .desktop file
desktop_in_files = xfce4-sensors.desktop.in

man_MANS = xfce4-sensors.1

desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
$(desktop_DATA): $(desktop_in_files) Makefile
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST = \
	$(desktop_in_files) \
	xfce4-sensors.1 \
	meson.build \
	$(NULL)

DISTCLEANFILES = \
	$(desktop_DATA)
