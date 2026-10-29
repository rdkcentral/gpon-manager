##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2019 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
dnl                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

dnl AC_PREREQ([2.69])
AC_INIT(RdkGponManager-rdkb, version-1.0)
AC_CONFIG_SRCDIR([])
AM_CONFIG_HEADER(cfg/config.h)
AC_CONFIG_MACRO_DIR([cfg])
AC_CONFIG_AUX_DIR([cfg])
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-bzip2 1.9])
LT_INIT

dnl subdirectories.
SUBDIRS=""

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_CPP
AC_PROG_CXXCPP

dnl use pretty build output with automake >= 1.11
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])],
  [AM_DEFAULT_VERBOSITY=1
   AC_SUBST(AM_DEFAULT_VERBOSITY)])



#Add git version
m4_define([GITVERSION],
  m4_esyscmd_s([git describe --tags --always --dirty 2>/dev/null || echo "undefined"])
)
AC_SUBST([GIT_VERSION], [GITVERSION])

dnl **********************************
dnl checks for dependencies
dnl **********************************
AC_HEADER_STDC

SYSTEMD_CFLAGS=" "
SYSTEMD_LDFLAGS=" "

AC_ARG_ENABLE([notify],
        AS_HELP_STRING([--enable-notify],[enable systemd notify]),
        [
          case "${enableval}" in
           yes) SYSTEMD_CFLAGS="-DENABLE_SD_NOTIFY"
                SYSTEMD_LDFLAGS="-lsystemd" ;;
           no) AC_MSG_ERROR([systemd notify is disabled]) ;;
          *) AC_MSG_ERROR([bad value ${enableval} for --enable-notify ]) ;;
           esac
           ],
         [echo "systemd notify is disabled."])

AC_SUBST(SYSTEMD_CFLAGS)
AC_SUBST(SYSTEMD_LDFLAGS)

AC_ARG_VAR([RBUS_CFLAGS], [Compiler flags for RBUS headers])
AC_ARG_VAR([RBUS_LIBS], [Linker flags for RBUS])
AS_IF([test -z "$RBUS_LIBS"], [RBUS_LIBS="-lrbus"])
saved_CPPFLAGS="$CPPFLAGS"
saved_LIBS="$LIBS"
CPPFLAGS="$CPPFLAGS $RBUS_CFLAGS"
LIBS="$RBUS_LIBS $LIBS"
AC_CHECK_HEADER([rbus.h], [],
  [AC_MSG_ERROR([RBUS headers are required; set RBUS_CFLAGS for the target sysroot])])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <rbus.h>]],
  [[rbus_registerDynamicTableSyncHandler((rbusHandle_t)0, "Device.XPON.ONU.", 0);]])], [],
  [AC_MSG_ERROR([RBUS with dynamic table synchronization is required; set RBUS_LIBS])])
CPPFLAGS="$saved_CPPFLAGS"
LIBS="$saved_LIBS"
AC_SUBST(RBUS_CFLAGS)
AC_SUBST(RBUS_LIBS)

dnl Option to enable/disable RDK Telemetry 2.0 support
AC_ARG_ENABLE([telemetry2-0],
    AS_HELP_STRING([--enable-telemetry2-0], [Enable RDK Telemetry 2.0 support (default=no)]),
    [enable_telemetry2_0=$enableval],
    [enable_telemetry2_0=no])

T2_LIBS=""
T2_CFLAGS=""
if test "x$enable_telemetry2_0" = "xyes"; then
    AC_CHECK_LIB([telemetry_msgsender], [t2_event_s], [
        T2_LIBS="-ltelemetry_msgsender"
    ], [
        AC_MSG_ERROR([libtelemetry_msgsender is required for Telemetry 2.0])
    ])
    AC_DEFINE([ENABLE_FEATURE_TELEMETRY2_0], [1],
              [Define to 1 to enable RDK Telemetry 2.0 support])
    dnl Sources do not include cfg/config.h, so also pass the define on the command line.
    T2_CFLAGS="-DENABLE_FEATURE_TELEMETRY2_0"
fi
AC_SUBST([T2_LIBS])
AC_SUBST([T2_CFLAGS])

dnl Checks for header files.
AC_CHECK_HEADERS([limits.h memory.h stdlib.h string.h sys/socket.h unistd.h])

dnl Checks for typedefs, structures, and compiler characteristics.
dnl AC_CHECK_HEADER_STDBOOL
AC_TYPE_UINT8_T

dnl Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset strdup strerror])

AC_CONFIG_FILES([Makefile
                source/Makefile
                source/TR-181/Makefile
                source/TR-181/middle_layer_src/Makefile
                source/TR-181/rbus/Makefile
                source/telemetry/Makefile
                source/GponManager/Makefile])
AC_OUTPUT
