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

ifeq ($(CONFIG_BLUETOOTH_FRAMEWORK), y)
ifeq ($(CONFIG_BLUETOOTH_FRAMEWORK_LOCAL), y)
	CSRCS += framework/api/*.c
ifeq ($(CONFIG_BLUETOOTH_BLE_AUDIO),)
  CSRCS := $(filter-out $(wildcard framework/api/bt_lea*),$(wildcard $(CSRCS)))
endif
else ifeq ($(CONFIG_BLUETOOTH_FRAMEWORK_SOCKET_IPC), y)
	CSRCS += framework/api/*.c
ifeq ($(CONFIG_BLUETOOTH_BLE_AUDIO),)
  CSRCS := $(filter-out $(wildcard framework/api/bt_lea*),$(wildcard $(CSRCS)))
endif
	CSRCS += service/ipc/*.c
	CSRCS += service/ipc/socket/src/*.c
	CSRCS += framework/socket/*.c
  CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/ipc/socket/include
else
endif
endif

CSRCS += service/src/manager_service.c
CSRCS += service/common/*.c

ifeq ($(CONFIG_BLUETOOTH_OBELISK), y)
	CSRCS += service/src/adapter_service.c
	CSRCS += service/src/adapter_state.c
	CSRCS += service/src/btservice.c
	CSRCS += service/src/device.c
ifeq ($(CONFIG_BLUETOOTH_BLE_ADV), y)
	CSRCS += service/src/advertising.c
endif #CONFIG_BLUETOOTH_BLE_ADV
ifeq ($(CONFIG_BLUETOOTH_BLE_SCAN), y)
	CSRCS += service/src/scan_manager.c
endif #CONFIG_BLUETOOTH_BLE_SCAN
	CSRCS += service/stacks/*.c
	CSRCS += service/stacks/bluelet/*.c
ifeq ($(CONFIG_BLUETOOTH_BLE_AUDIO),)
  CSRCS := $(filter-out $(wildcard service/stacks/bluelet/sal_lea_*),$(wildcard $(CSRCS)))
endif #CONFIG_BLUETOOTH_BLE_AUDIO
	CSRCS += service/profiles/*.c
	CSRCS += service/profiles/system/*.c
ifeq ($(CONFIG_BLUETOOTH_A2DP),)
	CSRCS := $(filter-out $(wildcard service/profiles/system/bt_player.c),$(wildcard $(CSRCS)))
endif #CONFIG_BLUETOOTH_A2DP
ifeq ($(findstring y, $(CONFIG_BLUETOOTH_A2DP)_$(CONFIG_BLUETOOTH_HFP_AG)_$(CONFIG_BLUETOOTH_HFP_HF)), )
	CSRCS := $(filter-out $(wildcard service/profiles/system/media_system.c),$(wildcard $(CSRCS)))
endif #CONFIG_BLUETOOTH_A2DP/CONFIG_BLUETOOTH_HFP_AG/CONFIG_BLUETOOTH_HFP_HF
	CSRCS += service/profiles/audio_interface/*.c
ifeq ($(CONFIG_BLUETOOTH_GATT), y)
	CSRCS += service/profiles/gatt/*.c
endif #CONFIG_BLUETOOTH_GATT

ifeq ($(CONFIG_BLUETOOTH_A2DP), y)
  CSRCS += service/profiles/a2dp/*.c
  CSRCS += service/profiles/a2dp/codec/*.c
  CSRCS += service/profiles/avrcp/*.c
  CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/profiles/a2dp
  CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/profiles/a2dp/codec
  CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/profiles/avrcp
endif #CONFIG_BLUETOOTH_A2DP

ifeq ($(CONFIG_BLUETOOTH_A2DP_SOURCE), y)
  CSRCS += service/profiles/a2dp/source/*.c
endif #CONFIG_BLUETOOTH_A2DP_SOURCE

ifeq ($(CONFIG_BLUETOOTH_A2DP_SINK), y)
  CSRCS += service/profiles/a2dp/sink/*.c
endif #CONFIG_BLUETOOTH_A2DP_SINK

ifeq ($(CONFIG_BLUETOOTH_AVRCP_TARGET), y)
  CSRCS += service/profiles/avrcp/target/*.c
endif #CONFIG_BLUETOOTH_A2DP_SOURCE

ifeq ($(CONFIG_BLUETOOTH_AVRCP_CONTROL), y)
  CSRCS += service/profiles/avrcp/control/*.c
endif #CONFIG_BLUETOOTH_A2DP_SINK

ifeq ($(CONFIG_BLUETOOTH_HFP_HF), y)
	CSRCS += service/profiles/hfp_hf/*.c
endif #CONFIG_BLUETOOTH_HFP_HF

ifeq ($(CONFIG_BLUETOOTH_HFP_AG), y)
	CSRCS += service/profiles/hfp_ag/*.c
endif #CONFIG_BLUETOOTH_HFP_AG

ifeq ($(CONFIG_BLUETOOTH_SPP), y)
	CSRCS += service/profiles/spp/*.c
endif #CONFIG_BLUETOOTH_SPP

ifeq ($(CONFIG_BLUETOOTH_HID_DEVICE), y)
	CSRCS += service/profiles/hid/*.c
endif #CONFIG_BLUETOOTH_HID_DEVICE

ifeq ($(CONFIG_BLUETOOTH_PAN), y)
	CSRCS += service/profiles/pan/*.c
endif #CONFIG_BLUETOOTH_PAN

ifneq ($(findstring y, $(CONFIG_BLUETOOTH_LEAUDIO_CLIENT)_$(CONFIG_BLUETOOTH_LEAUDIO_SERVER)), )
	CSRCS += service/profiles/leaudio/audio_ipc/*.c
	CSRCS += service/profiles/leaudio/*.c
endif #CONFIG_BLUETOOTH_LEAUDIO_CLIENT/CONFIG_BLUETOOTH_LEAUDIO_SERVER

ifeq ($(CONFIG_BLUETOOTH_LEAUDIO_SERVER), y)
	CSRCS += service/profiles/leaudio/server/*.c
endif #CONFIG_BLUETOOTH_LEAUDIO_SERVER

ifeq ($(CONFIG_BLUETOOTH_LEAUDIO_CCP), y)
	CSRCS += service/profiles/leaudio/ccp/*.c
endif #CONFIG_BLUETOOTH_LEAUDIO_CCP

ifeq ($(CONFIG_BLUETOOTH_LEAUDIO_MCP), y)
	CSRCS += service/profiles/leaudio/mcp/*.c
endif #CONFIG_BLUETOOTH_LEAUDIO_MCP

ifeq ($(CONFIG_BLUETOOTH_LEAUDIO_VMICS), y)
	CSRCS += service/profiles/leaudio/vmics/*.c
endif #CONFIG_BLUETOOTH_LEAUDIO_VMICS

ifeq ($(CONFIG_BLUETOOTH_LEAUDIO_CLIENT), y)
	CSRCS += service/profiles/leaudio/client/*.c
endif #CONFIG_BLUETOOTH_LEAUDIO_CLIENT

ifeq ($(CONFIG_BLUETOOTH_LEAUDIO_MCS), y)
	CSRCS += service/profiles/leaudio/mcs/*.c
endif #CONFIG_BLUETOOTH_LEAUDIO_MCS

ifeq ($(CONFIG_BLUETOOTH_LEAUDIO_TBS), y)
	CSRCS += service/profiles/leaudio/tbs/*.c
endif #CONFIG_BLUETOOTH_LEAUDIO_TBS

ifeq ($(CONFIG_BLUETOOTH_LEAUDIO_VMICP), y)
	CSRCS += service/profiles/leaudio/vmicp/*.c
endif #CONFIG_BLUETOOTH_LEAUDIO_VMICP

CSRCS += service/utils/*.c
endif #CONFIG_BLUETOOTH_OBELISK

ifeq ($(CONFIG_BLUETOOTH_TOOLS), y)
	CSRCS += tools/utils.c
	CSRCS += tools/uv_thread_loop.c
ifeq ($(CONFIG_BLUETOOTH_BLE_ADV), y)
	CSRCS += tools/adv.c
endif
ifeq ($(CONFIG_BLUETOOTH_BLE_SCAN), y)
	CSRCS += tools/scan.c
endif
ifeq ($(CONFIG_BLUETOOTH_A2DP_SINK), y)
	CSRCS += tools/a2dp_sink.c
endif #CONFIG_BLUETOOTH_A2DP_SINK
ifeq ($(CONFIG_BLUETOOTH_A2DP_SOURCE), y)
	CSRCS += tools/a2dp_source.c
endif #CONFIG_BLUETOOTH_A2DP_SOURCE
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
ifeq ($(CONFIG_BLUETOOTH_HID_DEVICE), y)
	CSRCS += tools/hid_device.c
endif
ifeq ($(CONFIG_BLUETOOTH_PAN), y)
	CSRCS += tools/panu.c
endif

ifeq ($(CONFIG_BLUETOOTH_LEAUDIO_SERVER), y)
	CSRCS += tools/lea_server.c
endif

ifeq ($(CONFIG_BLUETOOTH_LEAUDIO_MCP), y)
	CSRCS += tools/lea_mcp.c
endif

ifeq ($(CONFIG_BLUETOOTH_LEAUDIO_CCP), y)
	CSRCS += tools/lea_ccp.c
endif

ifeq ($(CONFIG_BLUETOOTH_LEAUDIO_VMICS), y)
	CSRCS += tools/lea_vmics.c
endif

ifeq ($(CONFIG_BLUETOOTH_LEAUDIO_CLIENT), y)
	CSRCS += tools/lea_client.c
endif
ifeq ($(CONFIG_BLUETOOTH_LEAUDIO_VMICP), y)
	CSRCS += tools/lea_vmicp.c
endif

ifeq ($(CONFIG_BLUETOOTH_LEAUDIO_MCS), y)
    CSRCS += tools/lea_mcs.c
endif

ifeq ($(CONFIG_BLUETOOTH_LEAUDIO_TBS), y)
	CSRCS += tools/lea_tbs.c
endif

endif

# framework/service/stack/tools dependence
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/framework/include

ifneq ($(CONFIG_LIB_DBUS_RPMSG_SERVER_CPUNAME)$(CONFIG_OFONO),)
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/dbus/dbus
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/glib/glib/glib
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/glib/glib
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/glib
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/utils/gdbus
endif

CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/src
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/common
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/profiles
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/profiles/include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/profiles/system
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/stacks
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/stacks/include

ifeq ($(CONFIG_BLUETOOTH_OBELISK), y)
ifneq ($(CONFIG_OBELISK_BREDR_BLUELET)$(CONFIG_OBELISK_LE_BLUELET),)
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/bluelet/bluelet/src/samples/stack_adapter/inc
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/vendor/xiaomi/vela/bluelet/inc
endif
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/service/ipc
endif

ifeq ($(CONFIG_BLUETOOTH_TOOLS), y)
	CFLAGS	+= ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/tools
endif

ifeq ($(CONFIG_ARCH_SIM),y)
CFLAGS	 += -O0
endif

CFLAGS	  += -Wno-strict-prototypes #-fno-short-enums -Wl,-no-enum-size-warning #-Werror
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
	PROGNAME += adapter_test
	MAINSRC  += tests/adapter_test.c
endif
endif

ASRCS := $(wildcard $(ASRCS))
CSRCS := $(wildcard $(CSRCS))
CXXSRCS := $(wildcard $(CXXSRCS))
MAINSRC := $(wildcard $(MAINSRC))

NOEXPORTSRCS = $(ASRCS)$(CSRCS)$(CXXSRCS)$(MAINSRC)

ifeq ($(CONFIG_BLUETOOTH_FEATURE),y)
include $(APPDIR)/frameworks/base/feature/Make.defs
ifeq ($(CONFIG_ARCH), arm)
TARGETDIR := arm
else ifeq ($(CONFIG_ARCH), arm64)
TARGETDIR := aarch64
else ifeq ($(CONFIG_ARCH), xtensa)
TARGETDIR := xtensa
else
TARGETDIR := x86
endif

CXXFLAGS    += $(CFLAGS)

CXXFLAGS    := $(filter-out -Wno-strict-prototypes, $(CXXFLAGS))
CXXFLAGS    := $(filter-out -Wstrict-prototypes, $(CXXFLAGS))
CXXFLAGS    += ${INCDIR_PREFIX}$(APPDIR)/external/libffi/libffi/src/$(TARGETDIR)
CXXFLAGS    += ${INCDIR_PREFIX}$(APPDIR)/external/libffi
CXXFLAGS    += ${INCDIR_PREFIX}$(APPDIR)/frameworks/bluetooth/feature/include

CXXSRCS     += feature/src/system_bluetooth.cpp
CXXSRCS     += feature/src/system_bluetooth_impl.cpp

CXXSRCS     += feature/src/system_bluetooth_bt.cpp
CXXSRCS     += feature/src/system_bluetooth_bt_impl.cpp

ifeq ($(CONFIG_BLUETOOTH_A2DP_SINK), y)
CXXSRCS     += feature/src/system_bluetooth_bt_a2dpsink.cpp
CXXSRCS     += feature/src/system_bluetooth_bt_a2dpsink_impl.cpp
endif
include $(APPDIR)/frameworks/quickapp/Module.mk
endif

ifneq ($(NOEXPORTSRCS),)
BIN := $(APPDIR)/staging/libbluetooth.a
endif



include $(APPDIR)/Application.mk

