noinst_LTLIBRARIES = libRdkGponManager_rbus.la

AM_CPPFLAGS = $(RBUS_CFLAGS) \
    -I$(top_srcdir)/source/TR-181/middle_layer_src \
    -I$(top_srcdir)/source/TR-181/include \
    -I$(top_srcdir)/source/GponManager
AM_CFLAGS = -std=gnu11

libRdkGponManager_rbus_la_SOURCES = gponmgr_xpon_rbus.c gponmgr_xpon_rbus.h

EXTRA_DIST = gponmgr_xpon_params.def