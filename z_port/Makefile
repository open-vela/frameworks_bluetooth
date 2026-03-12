############################################################################
#
# Copyright 2026 XiaoMi All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific
# language governing permissions and limitations under the License.
#
############################################################################

include $(APPDIR)/Make.defs

# z_port own include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/z_port/include

# Framework bluetooth include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/framework/include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/framework/common
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/src
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/common
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles/include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/stacks
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/stacks/include

# Core z_api modules for autopts testing
CSRCS += src/z_api_manager.c
CSRCS += src/z_api_gap.c
CSRCS += src/z_api_gatt.c
CSRCS += src/z_api_ext_adv.c
CSRCS += src/z_api_l2cap.c
CSRCS += src/z_api_spp.c
CSRCS += src/z_api_a2dp.c
CSRCS += src/z_api_avrcp.c
CSRCS += src/z_api_hfp.c
CSRCS += src/z_api_pan.c
CSRCS += src/z_api_hid.c

MODULE = y

include $(APPDIR)/Application.mk
