dnl
dnl Copyright (c) 2003-2022 - The Xfce Development Team
dnl
dnl 2003,2004,2007 Benedikt Meurer <benny@xfce.org>
dnl 2007-2011,2014-2020 Fabian Nowak <timystery@arcor.de>
dnl

dnl Version information
m4_define([copyright_year], [2025])
XDT_VERSION_INIT([1.5.0], [git])

dnl Initialize autoconf
AC_COPYRIGHT([Copyright (c) 2004-copyright_year() The Xfce development team. All rights reserved.])
AC_INIT([xfce4-sensors-plugin], [xdt_version], [https://gitlab.xfce.org/panel-plugins/xfce4-sensors-plugin/], [xfce4-sensors-plugin])
AC_REVISION([xdt_version_build])
AC_PREREQ([2.69])
AC_CONFIG_MACRO_DIRS([m4])
AC_DEFINE([VERSION_FULL], [PACKAGE_VERSION], [Alias for VERSION and PACKAGE_VERSION for meson compatibility])
COPYRIGHT_YEAR=copyright_year()
AC_DEFINE_UNQUOTED([COPYRIGHT_YEAR], ["$COPYRIGHT_YEAR"], [Copyright year])
AC_SUBST([COPYRIGHT_YEAR])

dnl Initialize automake
AM_INIT_AUTOMAKE([1.11 dist-bzip2 tar-ustar no-dist-gzip])
AC_CONFIG_HEADERS([config.h])
AM_MAINTAINER_MODE()
AM_SILENT_RULES([yes])

dnl Check for basic programs
AC_PROG_CC()
AC_PROG_CXX()
LT_PATH_LD([])
AC_PROG_INSTALL
AX_CXX_COMPILE_STDCXX_11()
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl Check for libsensors depending on whether it is disabled upon user request
AC_ARG_ENABLE([libsensors],
            AS_HELP_STRING([--enable-libsensors=auto|yes|no],
                    [Use LM-sensors to read a vast amount of sensors on your mainboards @<:@default=auto@:>@]),
            [],
            [enable_libsensors=auto])

if test x"$enable_libsensors" = x"auto"; then
    AC_CHECK_LIB([sensors], [sensors_get_detected_chips], [enable_libsensors=yes], [enable_libsensors=no])
else
    if test x"$enable_libsensors" = x"yes"; then
        AC_CHECK_LIB([sensors], [sensors_get_detected_chips], [], [AC_MSG_ERROR(could not find libsensors)])
    fi
fi

if test x"$enable_libsensors" = x"yes"; then
    AC_DEFINE([HAVE_LIBSENSORS], [1], [Define to 1 if libsensors is found])
    dnl FIXME:  make that dynamic in case of auto
    LIBSENSORS_CFLAGS="-I/usr/include"
    LIBSENSORS_LDFLAGS="-lsensors"
fi

AC_SUBST([LIBSENSORS_CFLAGS])
AC_SUBST([LIBSENSORS_LDFLAGS])
AM_CONDITIONAL([HAVE_LIBSENSORS], [test x$enable_libsensors = x"yes"])


dnl Check for hddtemp depending on whether it is disabled upon user request
AC_ARG_ENABLE([hddtemp],
        [AS_HELP_STRING([--enable-hddtemp=auto|yes|no],
                [Use hddtemp to read your harddisk temperature @<:@default=auto@:>@])],
        [],
        [
        enable_hddtemp=auto
        ])

if test x"$enable_hddtemp" = x"auto"; then
    AC_CHECK_PROG([hddtemp], [hddtemp], [yes], [], [$PATH:/sbin:/usr/sbin:/usr/local/sbin])
    if test x"$hddtemp" = x"yes"; then
        enable_hddtemp=yes
    else
        enable_hddtemp=no
    fi
fi

if test x"$enable_hddtemp" = x"yes"; then

    AC_ARG_WITH([pathhddtemp], [AS_HELP_STRING([--with-pathhddtemp], [Specify path to hddtemp @<:@default=auto@:>@])],
    [
        HDDTEMP=$withval
    ], [
        AC_MSG_CHECKING([for hddtemp path])
        HDDTEMP=[`whereis -b hddtemp | sed 's/.*\ \(\/[a-zA-Z0-9\-\/.]*bin[A-Za-z0-9\-\/.]*hddtemp\).*/\1/'`]
        AC_MSG_RESULT([$HDDTEMP])
    ])

    if test x"$HDDTEMP" = x"hddtemp:"; then
        enable_hddtemp=no
    else
        AC_DEFINE([HAVE_HDDTEMP], [1], [Define to 1 if hddtemp is desired])
        AC_DEFINE_UNQUOTED([PATH_HDDTEMP], ["$HDDTEMP"], [Complete path to hddtemp])
        AC_SUBST([PATH_HDDTEMP])
    fi
fi

AM_CONDITIONAL([HAVE_HDDTEMP], [test x$enable_hddtemp = x"yes"])


dnl Check for netcat for hddtemp to be used
AC_ARG_ENABLE([netcat], [AS_HELP_STRING([--enable-netcat], [Use netcat to read your hddtemp temperature @<:@default=auto@:>@])],
[],
[
    enable_netcat=auto
])

if test x"$enable_netcat" = x"auto"; then
    AC_CHECK_PROG([netcat], [netcat], [yes], [], [$PATH:/sbin:/usr/sbin:/usr/local/sbin])
    AC_CHECK_PROG([nc], [nc], [yes], [], [$PATH:/sbin:/usr/sbin:/usr/local/sbin])
    if test x"$netcat" = x"yes" -o x"$nc" = x"yes"; then
        enable_netcat1=yes
    else
        enable_netcat1=no
    fi
else
    if test x"$enable_netcat" = x"yes"; then
        enable_netcat1=yes
    else
        enable_netcat1=no
    fi
fi

if test x"$enable_netcat1" = x"yes"; then
    AC_ARG_WITH([pathnetcat], [AS_HELP_STRING([--with-pathnetcat], [Specify path to netcat @<:@default=auto@:>@])],
    [
        NETCAT_PATH=$withval
    ], [
        AC_MSG_CHECKING([for netcat path])
        NETCAT_PATH=[`whereis -b netcat | sed 's/.*\ \(\/[a-zA-Z0-9\-\/.]*bin[A-Za-z0-9\-\/.]*netcat\).*/\1/'`]
        if test x"$NETCAT_PATH" = x"netcat:"; then
            NETCAT_PATH=[`whereis -b nc | sed 's/.*\ \(\/[a-zA-Z0-9\-\/.]*bin[A-Za-z0-9\-\/.]*nc\).*/\1/'`]
        fi;

        AC_MSG_RESULT([$NETCAT_PATH])
    ])

    NETCAT_HOST=localhost
    HDDTEMP_PORT=7634

    AC_ARG_ENABLE([pathchecks], [AS_HELP_STRING([--enable-pathchecks], [Enable checks for paths, access and execution @<:@default=yes@:>@])],
    [
    ], [
        enable_pathchecks=yes
    ])

    NETCAT_RUN=skip
    if test x"$enable_pathchecks" = x"yes"; then
        AC_MSG_CHECKING([for hddtemp being queryable via netcat])
        NETCAT_RUN=[`"$NETCAT_PATH" "$NETCAT_HOST" "$HDDTEMP_PORT" 2> /dev/null`]
        AC_MSG_RESULT([$NETCAT_RUN])
    fi

    if test x"$NETCAT_RUN" = x""; then
        enable_netcat1=no
        if test x"$enable_netcat" = x"yes"; then
            AC_MSG_ERROR([hddtemp isn't queryable via netcat (use --disable-pathchecks to disable this check)])
        fi
    else
        AC_DEFINE([HAVE_NETCAT], [1], [Define to 1 if hddtemp daemon is queried via netcat])
        AC_DEFINE_UNQUOTED([NETCAT_PATH], [$NETCAT_PATH], [Complete path to netcat])
        AC_SUBST([NETCAT_PATH])
        AC_DEFINE_UNQUOTED([NETCAT_HOST], [$NETCAT_HOST], [Host for netcat])
        AC_SUBST([NETCAT_HOST])
        AC_DEFINE_UNQUOTED([HDDTEMP_PORT], [$HDDTEMP_PORT], [Port for hddtemp with netcat])
        AC_SUBST([HDDTEMP_PORT])
    fi

fi

AM_CONDITIONAL([HAVE__NETCAT], [test x$enable_netcat1 = x"yes"])


dnl Check for interface for /proc/acpi to be used
AC_ARG_ENABLE([procacpi], [AS_HELP_STRING([--enable-procacpi], [Use /proc/acpi to read your CPU temperature @<:@default=auto@:>@])],
[],
[
    enable_procacpi=auto
])

if test x"$enable_procacpi" = x"auto"; then
    AC_CHECK_FILE([/proc/acpi/info],
    [
        enable_procacpi=yes
    ],
    [
        enable_procacpi=no
    ])
fi

dnl Check for interface for /sys/class/power_supply to be used
AC_ARG_ENABLE([sysfsacpi], [AS_HELP_STRING([--enable-sysfsacpi], [Use /sys/class/power_supply to read your battery value @<:@default=auto@:>@])],
[],
[
    enable_sysfsacpi=auto
])

if test x"$enable_sysfsacpi" = x"auto"; then
    AC_CHECK_FILE([/sys/class/power_supply],
    [
        enable_sysfsacpi=yes
    ],
    [
        enable_sysfsacpi=no
    ])
fi

if test x"$enable_sysfsacpi" = x"yes"; then
    AC_DEFINE([HAVE_SYSFS_ACPI], [1], [Define to 1 if /sys/class/power_supply is found])
    enable_procacpi=yes
fi

if test x"$enable_procacpi" = x"yes"; then
    AC_DEFINE([HAVE_ACPI], [1], [Define to 1 if /proc/acpi is found])
fi


dnl Check for nvidia blahblah to be used
AC_ARG_ENABLE([xnvctrl],
        AS_HELP_STRING([--enable-xnvctrl],
           [Use nvctrl to obtain Nvidia GPU temperature value @<:@default=auto@:>@]),
    [],
[
    enable_xnvctrl=auto
])

if test x"$enable_xnvctrl" = x"auto"; then
 AC_CHECK_FILE([/usr/include/NVCtrl],
    [
        enable_xnvctrl=yes
    ],
    [
        enable_xnvctrl=no
    ])
fi

if test x"$enable_xnvctrl" = x"yes"; then
    AC_DEFINE([HAVE_NVIDIA], [1], [Define to 1 if media-video/nvidia-drivers installed])
    NVIDIA_CFLAGS="-I/usr/include/NVCtrl"
    NVIDIA_LIBS="-lX11 -lXext -lXNVCtrl"
    enable_xnvctrl=yes
AC_SUBST([NVIDIA_CFLAGS])
AC_SUBST([NVIDIA_LIBS])
fi

AM_CONDITIONAL([HAVE_ACPI], [test x$enable_procacpi = x"yes"])
AM_CONDITIONAL([HAVE_SYSFS_ACPI], [test x$enable_sysfsacpi = x"yes"])
AM_CONDITIONAL([HAVE_NVIDIA], [test x$enable_xnvctrl = x"yes"])


dnl Get operating system information
KERNEL=`uname -s`
if test "$KERNEL" = "Linux"; then
    AC_DEFINE([HAVE_LINUX], [1], [Set this to 1 if using Linux or newer])
fi
dnl OS="$KERNEL"
dnl AC_DEFINE([OS], ["$OS"], [Set this to the name of the operating system.])

AC_CHECK_FUNCS_ONCE([gethostname malloc_trim socket])
AC_CHECK_HEADERS([arpa/inet.h])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([sys/ioctl.h])
AC_CHECK_HEADERS([sys/param.h])
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([sys/sockio.h])
AC_HEADER_SYS_WAIT

AC_CHECK_LIB([kstat], [kstat_open], SOLLIBS="-lkstat -lsocket", SOLLIBS="")
AC_CHECK_LIB([nsl], [kstat_open], SOLLIBS="$SOLLIBS -linet_ntop", SOLLIBS="$SOLLIBS")
AC_SUBST(SOLLIBS)

dnl ***********************************
dnl *** Check for Glib/Gtk packages ***
dnl ***********************************
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [2.50.0])
XDT_CHECK_PACKAGE([GTK], [gtk+-3.0], [3.22.0])


dnl **************************************
dnl *** Check for notification library ***
dnl **************************************
XDT_CHECK_OPTIONAL_PACKAGE([LIBNOTIFY],
                           [libnotify],
                           [0.7], [notification],
                           [notification message library])

dnl configure the panel plugin
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-2.0], [4.16.0])
XDT_CHECK_PACKAGE([LIBXFCE4UI], [libxfce4ui-2], [4.16.0])
XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0], [4.17.2])

dnl I18n support
GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

dnl Check for debugging support
XDT_FEATURE_DEBUG([xdt_debug_default])

dnl *********************************
dnl *** Substitute platform flags ***
dnl *********************************
AC_MSG_CHECKING([PLATFORM_LDFLAGS])
AC_MSG_RESULT([$PLATFORM_LDFLAGS])
AC_SUBST([PLATFORM_LDFLAGS])
AC_SUBST([PLATFORM_CFLAGS])


AC_CONFIG_FILES([
Makefile
icons/Makefile
include/Makefile
lib/Makefile
panel-plugin/Makefile
po/Makefile.in
src/Makefile
xfce4++/Makefile
xfce4++/util/Makefile
])
AC_OUTPUT


dnl ***************************
dnl *** Print configuration ***
dnl ***************************
echo
echo "==============================================================="
echo "Build Configuration"
echo "==============================================================="
dnl echo
dnl echo "Host system:  $OS"
echo
echo "The following sensor interfaces will be built into the plugin:"
if test x"$enable_hddtemp" = x"yes"; then
echo "* hddtemp: $HDDTEMP (Make sure it is user-executable with UID bit set and user member of the respective group)"
fi
if test x"$enable_netcat1" = x"yes"; then
echo "  netcat for hddtemp: $NETCAT_PATH on $NETCAT_HOST:$HDDTEMP_PORT"
fi
if test x"$LIBNOTIFY_FOUND" = x"yes"; then
echo "* libnotify"
fi
if test x"$enable_libsensors" = x"yes"; then
echo "* libsensors"
fi
if test x"$enable_procacpi" = x"yes"; then
echo "* /proc/acpi"
fi
if test x"$enable_sysfsacpi" = x"yes"; then
echo "* /sys/class/power_supply"
fi
if test x"$enable_xnvctrl" = x"yes"; then
echo "* NVIDIA X CTRL"
fi
echo
if test x"$enable_debug" = x"yes"; then
echo "Full debug is enabled"
else
echo "Full debug is disabled"
fi
echo
echo "==============================================================="
