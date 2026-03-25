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

PROGNAME  = cmocka_framework_api_test
PRIORITY  = $(CONFIG_CM_FRAMEWORK_API_TEST_PRIORITY)
STACKSIZE = $(CONFIG_CM_FRAMEWORK_API_TEST_STACKSIZE)
MODULE    = $(CONFIG_CM_FRAMEWORK_API_TEST)

MAINSRC = $(CURDIR)/cm_framework_api_entry.c

CSRCS  += src/test_bt_adapter.c
CSRCS  += src/test_bt_device.c
CSRCS  += src/test_bt_le_scan.c
CSRCS  += src/test_bt_le_advertiser.c
CSRCS  += src/test_bluetooth.c
CSRCS  += src/test_bt_gattc.c
CSRCS  += src/test_bt_gatts.c
CSRCS  += src/test_bt_a2dp_sink.c
CSRCS  += src/test_bt_a2dp_source.c
CSRCS  += src/test_bt_hfp_hf.c
CSRCS  += src/test_bt_hfp_ag.c
CSRCS  += src/test_bt_avrcp_control.c
CSRCS  += src/test_bt_avrcp_target.c
CSRCS  += src/test_bt_spp.c
CSRCS  += src/test_bt_pan.c
CSRCS  += src/test_bt_hid_device.c
CSRCS  += src/test_bt_l2cap.c
CSRCS  += src/mock_service_manager.c
CSRCS  += src/api_stubs.c

CFLAGS += -I$(CURDIR)/include
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/framework/include
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/framework/common
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/src
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/common
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles/include

include $(APPDIR)/Application.mk
