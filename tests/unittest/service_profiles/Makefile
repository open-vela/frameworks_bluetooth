############################################################################
#  Copyright (C) 2026 Xiaomi Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
############################################################################

include $(APPDIR)/Make.defs

PROGNAME  = cmocka_service_profiles_test
PRIORITY  = $(CONFIG_CM_SERVICE_PROFILES_TEST_PRIORITY)
STACKSIZE = $(CONFIG_CM_SERVICE_PROFILES_TEST_STACKSIZE)
MODULE    = $(CONFIG_CM_SERVICE_PROFILES_TEST)

MAINSRC = $(CURDIR)/cm_service_profiles_entry.c

CSRCS  += src/test_a2dp_codec_sbc.c
CSRCS  += src/test_a2dp_codec_aac.c
CSRCS  += src/test_a2dp_event.c
CSRCS  += src/test_avrcp_msg.c
CSRCS  += src/test_hfp_hf_event.c
CSRCS  += src/test_hfp_ag_event.c
CSRCS  += src/test_gatt_event.c

CFLAGS += -I$(CURDIR)/include
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/framework/include
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/framework/common
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/common
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/src
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles/a2dp
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles/a2dp/codec
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/vendor
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles/avrcp
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles/hfp_hf
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles/hfp_ag
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles/include
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles/gatt
CFLAGS += -Wno-implicit-function-declaration
CFLAGS += -Wno-unused-variable
CFLAGS += -Wno-strict-prototypes

include $(APPDIR)/Application.mk
