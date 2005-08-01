dnl configure.ac
dnl
dnl xfce4-sensors-plugin - Sensors plugin for xfce4-panel
dnl
dnl 2003,2004 Benedikt Meurer <benny@xfce.org>
dnl

AC_INIT([panel-plugin/sensors.c])

AM_CONFIG_HEADER([config.h])

AM_INIT_AUTOMAKE([xfce4-sensors-plugin], [0.6.2])

AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

dnl Check for standard header files
AC_HEADER_STDC

dnl Check for libsensors
AC_CHECK_LIB([sensors], [sensors_get_detected_chips], [], [
        echo "Error! You need to have libsensors around."
	exit -1
])

dnl configure the panel plugin
XFCE_PANEL_PLUGIN([XFCE4_PANEL], [4.0.0])

dnl I18n support
BM_I18N([xfce4-sensors-plugin], [de el fr ko lt])

dnl Check for debugging support
BM_DEBUG_SUPPORT

AC_OUTPUT([
Makefile
xfce4-sensors-plugin.spec
panel-plugin/Makefile
po/Makefile.in
])
