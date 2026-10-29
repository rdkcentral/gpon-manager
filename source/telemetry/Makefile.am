##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2025 RDK Management
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

noinst_LTLIBRARIES = libRdkGponManager_telemetry.la

libRdkGponManager_telemetry_la_CPPFLAGS = $(T2_CFLAGS) \
                                          -I$(top_srcdir)/source/telemetry \
                                          -I$(top_srcdir)/source/TR-181/include \
                                          -I$(top_srcdir)/source/TR-181/middle_layer_src \
                                          -I$(top_srcdir)/source/GponManager

libRdkGponManager_telemetry_la_SOURCES = gponmgr_telemetry.c \
                                         gponmgr_telemetry.h

libRdkGponManager_telemetry_la_LIBADD = $(T2_LIBS)
