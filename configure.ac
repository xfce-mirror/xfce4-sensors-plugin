dnl configure.ac
dnl
dnl xfce4-sensors-plugin - Sensors plugin for xfce4-panel
dnl
dnl 2003,2004 Benedikt Meurer <benny@xfce.org>
dnl

m4_define([sensors_version],[0.10.0])

AC_INIT([xfce4-sensors-plugin], [sensors_version()], [timystery@arcor.de])

AM_CONFIG_HEADER([config.h])

SENSORS_VERSION=sensors_version()
AM_INIT_AUTOMAKE([xfce4-sensors-plugin], [$SENSORS_VERSION])

AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl Check for basic programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_INTLTOOL

dnl Check for standard header files
AC_HEADER_STDC

dnl Check for libsensors
AC_CHECK_LIB([sensors], [sensors_get_detected_chips], [], [
        echo "Error! You need to have libsensors around."
	exit -1
])

dnl Substitute in Makefile
AC_SUBST(OS)

AC_CHECK_FUNCS([gethostname])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([strstr])
AC_CHECK_FUNCS([malloc])
AC_CHECK_FUNCS([bzero])
AC_CHECK_HEADERS([arpa/inet.h])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([sys/ioctl.h])
AC_CHECK_HEADERS([sys/param.h])
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([sys/sockio.h])
AC_HEADER_SYS_WAIT
AC_PROG_GCC_TRADITIONAL
AC_TYPE_SIZE_T

AC_CHECK_LIB(kstat, kstat_open, SOLLIBS="-lkstat -lsocket", SOLLIBS="")
AC_CHECK_LIB(nsl, kstat_open, SOLLIBS="$SOLLIBS -linet_ntop", SOLLIBS="$SOLLIBS")
AC_SUBST(SOLLIBS)

dnl configure the panel plugin
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-1.0], [4.3.20])

dnl I18n support
XDT_I18N([de el eu fr hu ja ko lt pl ru vi])

dnl Check for debugging support
XDT_FEATURE_DEBUG()

AC_OUTPUT([
Makefile
xfce4-sensors-plugin.spec
panel-plugin/Makefile
icons/Makefile
po/Makefile.in
])

