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
AM_CFLAGS =

ACLOCAL_AMFLAGS = -I m4

noinst_LTLIBRARIES = libRdkGponManager_middle_layer_src.la

libRdkGponManager_middle_layer_src_la_CPPFLAGS = -I$(top_srcdir)/source/TR-181/middle_layer_src \
                                                 -I$(top_srcdir)/source/TR-181/rbus \
                                                 -I$(top_srcdir)/source/TR-181/include \
                                                 -I$(top_srcdir)/source/GponManager \
                                                 -I$(top_srcdir)/source/telemetry

libRdkGponManager_middle_layer_src_la_SOURCES = gponmgr_dml_backendmgr.c \
                                                gponmgr_dml_data.c \
                                                gponmgr_dml_eth_iface.c \
                                                gponmgr_dml_func.c \
                                                gponmgr_dml_hal.c \
                                                gponmgr_dml_hal_param.c \
                                                gponmgr_dml_internal.c \
                                                gponmgr_dml_obj.c \
                                                gponmgr_dml_plugin_main.c

libRdkGponManager_middle_layer_src_la_LDFLAGS = -lccsp_common -lhal_platform -ljson_hal_client -ljson-c -lpthread -lsyscfg
