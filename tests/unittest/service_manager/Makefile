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

PROGNAME  = cmocka_service_manager_test
PRIORITY  = $(CONFIG_CM_SERVICE_MANAGER_TEST_PRIORITY)
STACKSIZE = $(CONFIG_CM_SERVICE_MANAGER_TEST_STACKSIZE)
MODULE    = $(CONFIG_CM_SERVICE_MANAGER_TEST)

MAINSRC = $(CURDIR)/cm_service_manager_entry.c

CSRCS  += src/test_service_manager.c

CFLAGS += -I$(CURDIR)/include
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/framework/include
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/framework/common
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/common
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/src
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service
CFLAGS += -I$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles
CFLAGS += -Wno-implicit-function-declaration
CFLAGS += -Wno-unused-variable

include $(APPDIR)/Application.mk
