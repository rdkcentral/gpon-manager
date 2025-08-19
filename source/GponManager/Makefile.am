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

bin_PROGRAMS = GponManager

GponManager_DEPENDENCIES = \
        $(EXTRA_DEPENDENCIES) \
        ${top_builddir}/source/TR-181/middle_layer_src/libRdkGponManager_middle_layer_src.la

GponManager_CPPFLAGS = -I$(top_srcdir)/source/TR-181/middle_layer_src \
                       -I$(top_srcdir)/source/TR-181/include

GponManager_CFLAGS = -DFEATURE_SUPPORT_RDKLOG $(SYSTEMD_CFLAGS)
GponManager_SOURCES = ssp_action.c ssp_messagebus_interface.c ssp_main.c gponmgr_controller.c gponmgr_link_state_machine.c dm_pack_datamodel.c
GponManager_LDFLAGS = -lccsp_common -lrdkloggers -lsecure_wrapper $(SYSTEMD_LDFLAGS)
GponManager_LDADD = $(GponManager_DEPENDENCIES)
GponManager_CFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
