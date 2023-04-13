#
# Copyright (C) 2020 Xiaomi Corporation
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

include $(APPDIR)/Make.defs

ifeq ($(CONFIG_BLUETOOTH), y)

CSRCS += framework/common/*.c
CSRCS += framework/common/utils/*.c

ifeq ($(CONFIG_BLUETOOTH_FRAMEWORK), y)
ifeq ($(CONFIG_BLUETOOTH_FRAMEWORK_LOCAL), y)
	CSRCS += framework/api/*.c
else ifeq ($(CONFIG_BLUETOOTH_FRAMEWORK_BINDER_IPC), y)
	CSRCS += framework/binder/*.c
	CSRCS += service/ipc/*.c
	CSRCS += service/ipc/binder/parcel/*.c
	CSRCS += service/ipc/binder/src/*.c
else
endif
endif

ifeq ($(CONFIG_BLUETOOTH_OBELISK), y)
	CSRCS += service/src/manager_service.c
	CSRCS += service/src/adapter_service.c
	CSRCS += service/src/adapter_state.c
	CSRCS += service/src/btservice.c
	CSRCS += service/src/device.c
ifeq ($(CONFIG_BLUETOOTH_BLE_ADV), y)
	CSRCS += service/src/advertising.c
endif
ifeq ($(CONFIG_BLUETOOTH_BLE_SCAN), y)
	CSRCS += service/src/scan_manager.c
endif
	CSRCS += service/stacks/*.c
	CSRCS += service/common/*.c
	CSRCS += service/stacks/bluelet/*.c
	CSRCS += service/profiles/*.c
	CSRCS += service/profiles/system/*.c
ifeq ($(CONFIG_BLUETOOTH_GATT), y)
	CSRCS += service/profiles/gatt/*.c
endif #CONFIG_BLUETOOTH_GATT

ifeq ($(CONFIG_BLUETOOTH_A2DP_SOURCE), y)
endif #CONFIG_BLUETOOTH_A2DP_SOURCE

ifeq ($(CONFIG_BLUETOOTH_A2DP_SINK), y)
endif #CONFIG_BLUETOOTH_A2DP_SINK

ifeq ($(CONFIG_BLUETOOTH_HFP_HF), y)
	CSRCS += service/profiles/hfp_hf/*.c
endif #CONFIG_BLUETOOTH_HFP_HF

ifeq ($(CONFIG_BLUETOOTH_HFP_AG), y)
	CSRCS += service/profiles/hfp_ag/*.c
endif #CONFIG_BLUETOOTH_HFP_AG

ifeq ($(CONFIG_BLUETOOTH_SPP), y)
	CSRCS += service/profiles/spp/*.c
endif

ifeq ($(CONFIG_BLUETOOTH_PAN), y)
	CSRCS += service/profiles/pan/*.c
endif

endif

ifeq ($(CONFIG_BLUETOOTH_TOOLS), y)
	CSRCS += tools/utils.c
ifeq ($(CONFIG_BLUETOOTH_BLE_ADV), y)
	CSRCS += tools/adv.c
endif
ifeq ($(CONFIG_BLUETOOTH_BLE_SCAN), y)
	CSRCS += tools/scan.c
endif
ifeq ($(CONFIG_BLUETOOTH_GATT), y)
	CSRCS += tools/gatt_client.c
	CSRCS += tools/gatt_server.c
endif #CONFIG_BLUETOOTH_GATT
ifeq ($(CONFIG_BLUETOOTH_HFP_HF), y)
	CSRCS += tools/hfp_hf.c
endif #CONFIG_BLUETOOTH_HFP_HF

ifeq ($(CONFIG_BLUETOOTH_HFP_AG), y)
	CSRCS += tools/hfp_ag.c
endif #CONFIG_BLUETOOTH_HFP_AG

ifeq ($(CONFIG_BLUETOOTH_SPP), y)
	CSRCS += tools/spp.c
endif
ifeq ($(CONFIG_BLUETOOTH_PAN), y)
	CSRCS += tools/panu.c
endif
endif

# framework/service/stack/tools dependence
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/framework/include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/framework/common

ifeq ($(CONFIG_OFONO), y)
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/dbus/dbus
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/glib/glib/glib
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/glib/glib
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/glib
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/utils/gdbus
endif

ifeq ($(CONFIG_BLUETOOTH_OBELISK), y)
ifneq ($(CONFIG_OBELISK_BREDR_BLUELET)$(CONFIG_OBELISK_LE_BLUELET),)
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/bluelet/bluelet/src/samples/stack_adapter/inc
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/vendor/xiaomi/vela/bluelet/inc
endif
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/src
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/common
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/profiles
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/profiles/include
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/profiles/system
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/stacks
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/stacks/include
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/ipc
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/ipc/binder/include
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/ipc/binder/parcel
endif

ifeq ($(CONFIG_BLUETOOTH_TOOLS), y)
	CFLAGS	+= ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/tools
endif

ifeq ($(CONFIG_ARCH_SIM),y)
CFLAGS	 += -O0
endif
CFLAGS	 += -Wno-strict-prototypes #-Werror
PRIORITY  = SCHED_PRIORITY_DEFAULT
STACKSIZE = 8192
MODULE    = $(CONFIG_BLUETOOTH)

# if enabled bluetoothd
ifeq ($(CONFIG_BLUETOOTH_SERVER), y)
	PROGNAME += $(CONFIG_BLUETOOTH_SERVER_NAME)
	MAINSRC  += service/src/main.c
endif

# if enabled bttool
ifeq ($(CONFIG_BLUETOOTH_TOOLS), y)
	PROGNAME += bttool
	MAINSRC  += tools/bt_tools.c
endif
endif

ASRCS := $(wildcard $(ASRCS))
CSRCS := $(wildcard $(CSRCS))
CXXSRCS := $(wildcard $(CXXSRCS))
MAINSRC := $(wildcard $(MAINSRC))

NOEXPORTSRCS = $(ASRCS)$(CSRCS)$(CXXSRCS)$(MAINSRC)

ifneq ($(NOEXPORTSRCS),)
BIN := $(APPDIR)/staging/libbluetooth.a
endif

include $(APPDIR)/Application.mk

