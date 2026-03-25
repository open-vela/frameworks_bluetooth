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

PROGNAME  = cmocka_framework_common_test
PRIORITY  = $(CONFIG_CM_FRAMEWORK_COMMON_TEST_PRIORITY)
STACKSIZE = $(CONFIG_CM_FRAMEWORK_COMMON_TEST_STACKSIZE)
MODULE    = $(CONFIG_CM_FRAMEWORK_COMMON_TEST)

BT_DIR := $(CURDIR)/../../..
MAINSRC = $(CURDIR)/cm_framework_common_entry.c

CSRCS  += src/test_bt_addr.c
CSRCS  += src/test_bt_uuid.c
CSRCS  += src/test_bt_list.c
CSRCS  += src/test_bt_hash.c
CSRCS  += src/test_bt_time.c
CSRCS  += src/test_callbacks_list.c
CSRCS  += src/test_state_machine.c
CSRCS  += src/test_index_allocator.c
CSRCS  += src/test_hci_parser.c
CSRCS  += src/test_scan_record.c
CSRCS  += src/test_scan_filter.c
CSRCS  += src/test_advertiser_data.c

CSRCS  += $(BT_DIR)/framework/common/advertiser_data.c
CSRCS  += $(BT_DIR)/framework/common/callbacks_list.c
CSRCS  += $(BT_DIR)/service/src/scan_record.c
CSRCS  += $(BT_DIR)/service/src/hci_parser.c

CFLAGS += -I$(CURDIR)/include
CFLAGS += -I$(BT_DIR)/framework/include
CFLAGS += -I$(BT_DIR)/framework/common
CFLAGS += -I$(BT_DIR)/service/common
CFLAGS += -I$(BT_DIR)/service/src
CFLAGS += -I$(BT_DIR)/service

include $(APPDIR)/Application.mk
