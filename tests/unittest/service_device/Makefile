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

PROGNAME  = cmocka_service_device_test
PRIORITY  = $(CONFIG_CM_SERVICE_DEVICE_TEST_PRIORITY)
STACKSIZE = $(CONFIG_CM_SERVICE_DEVICE_TEST_STACKSIZE)
MODULE    = $(CONFIG_CM_SERVICE_DEVICE_TEST)

MAINSRC = $(CURDIR)/cm_service_device_entry.c

CSRCS  += src/test_device.c

CFLAGS += -I$(CURDIR)/include
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/framework/include
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/framework/common
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/common
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/src
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service

include $(APPDIR)/Application.mk
