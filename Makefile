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
CSRCS += framework/api/bluetooth.c
CSRCS += framework/api/bt_adapter.c
CSRCS += framework/api/bt_device.c
ifeq ($(CONFIG_BLUETOOTH_LE_CS), y)
CSRCS += framework/api/bt_cs.c
endif #CONFIG_BLUETOOTH_LE_CS
ifeq ($(CONFIG_BLUETOOTH_A2DP_SINK), y)
CSRCS += framework/api/bt_a2dp_sink.c
endif #CONFIG_BLUETOOTH_A2DP_SINK
ifeq ($(CONFIG_BLUETOOTH_A2DP_SOURCE), y)
CSRCS += framework/api/bt_a2dp_source.c
endif #CONFIG_BLUETOOTH_A2DP_SOURCE
ifeq ($(CONFIG_BLUETOOTH_AVRCP_TARGET), y)
CSRCS += framework/api/bt_avrcp_target.c
endif #CONFIG_BLUETOOTH_AVRCP_TARGET
ifeq ($(CONFIG_BLUETOOTH_AVRCP_CONTROL), y)
CSRCS += framework/api/bt_avrcp_control.c
endif #CONFIG_BLUETOOTH_AVRCP_CONTROL
ifeq ($(CONFIG_BLUETOOTH_HFP_HF), y)
CSRCS += framework/api/bt_hfp_hf.c
endif #CONFIG_BLUETOOTH_HFP_HF
ifeq ($(CONFIG_BLUETOOTH_HFP_AG), y)
CSRCS += framework/api/bt_hfp_ag.c
endif #CONFIG_BLUETOOTH_HFP_AG
ifeq ($(CONFIG_BLUETOOTH_SPP), y)
CSRCS += framework/api/bt_spp.c
endif #CONFIG_BLUETOOTH_SPP
ifeq ($(CONFIG_BLUETOOTH_HID_DEVICE), y)
CSRCS += framework/api/bt_hid_device.c
endif #CONFIG_BLUETOOTH_HID_DEVICE
ifeq ($(CONFIG_BLUETOOTH_GATT_CLIENT), y)
CSRCS += framework/api/bt_gattc.c
endif #CONFIG_BLUETOOTH_GATT_CLIENT
ifeq ($(CONFIG_BLUETOOTH_GATT_SERVER), y)
CSRCS += framework/api/bt_gatts.c
endif #CONFIG_BLUETOOTH_GATT_SERVER
ifeq ($(CONFIG_BLUETOOTH_L2CAP), y)
CSRCS += framework/api/bt_l2cap.c
endif #CONFIG_BLUETOOTH_L2CAP
ifeq ($(CONFIG_BLUETOOTH_BLE_ADV), y)
CSRCS += framework/api/bt_le_advertiser.c
endif #CONFIG_BLUETOOTH_BLE_ADV
ifeq ($(CONFIG_BLUETOOTH_BLE_SCAN), y)
CSRCS += framework/api/bt_le_scan.c
endif #CONFIG_BLUETOOTH_BLE_SCAN
ifeq ($(CONFIG_BLUETOOTH_LOG), y)
CSRCS += framework/api/bt_trace.c
endif
ifeq ($(CONFIG_BLUETOOTH_PAN), y)
CSRCS += framework/api/bt_pan.c
endif #CONFIG_BLUETOOTH_PAN
ifeq ($(CONFIG_BLUETOOTH_BLE_AUDIO), y)
CSRCS := framework/api/bt_lea*.c
endif #CONFIG_BLUETOOTH_BLE_AUDIO

ifeq ($(CONFIG_BLUETOOTH_FRAMEWORK_SOCKET_IPC), y)
CSRCS += service/ipc/bluetooth_ipc.c
CSRCS += framework/socket/bt_device.c
CSRCS += framework/socket/bt_adapter.c
CSRCS += framework/socket/bluetooth.c
CSRCS += service/ipc/socket/src/bt_socket.c
CSRCS += service/ipc/socket/src/bt_socket_manager.c
CSRCS += service/ipc/socket/src/bt_socket_client.c
CSRCS += service/ipc/socket/src/bt_socket_server.c
CSRCS += service/ipc/socket/src/bt_socket_device.c
CSRCS += service/ipc/socket/src/bt_socket_adapter.c
CSRCS += service/ipc/socket/src/bt_socket_bluetooth.c

ifeq ($(CONFIG_BLUETOOTH_A2DP_SINK), y)
CSRCS += framework/socket/bt_a2dp_sink.c
CSRCS += service/ipc/socket/src/bt_socket_a2dp_sink.c
endif #CONFIG_BLUETOOTH_A2DP_SINK

ifeq ($(CONFIG_BLUETOOTH_A2DP_SOURCE), y)
CSRCS += framework/socket/bt_a2dp_source.c
CSRCS += service/ipc/socket/src/bt_socket_a2dp_source.c
endif #CONFIG_BLUETOOTH_A2DP_SOURCE

ifeq ($(CONFIG_BLUETOOTH_AVRCP_TARGET), y)
CSRCS += framework/socket/bt_avrcp_target.c
CSRCS += service/ipc/socket/src/bt_socket_avrcp_target.c
endif #CONFIG_BLUETOOTH_AVRCP_TARGET

ifeq ($(CONFIG_BLUETOOTH_AVRCP_CONTROL), y)
CSRCS += framework/socket/bt_avrcp_control.c
CSRCS += service/ipc/socket/src/bt_socket_avrcp_control.c
endif #CONFIG_BLUETOOTH_AVRCP_CONTROL

ifeq ($(CONFIG_BLUETOOTH_HFP_HF), y)
CSRCS += framework/socket/bt_hfp_hf.c
CSRCS += service/ipc/socket/src/bt_socket_hfp_hf.c
endif #CONFIG_BLUETOOTH_HFP_HF

ifeq ($(CONFIG_BLUETOOTH_HFP_AG), y)
CSRCS += framework/socket/bt_hfp_ag.c
CSRCS += service/ipc/socket/src/bt_socket_hfp_ag.c
endif #CONFIG_BLUETOOTH_HFP_AG

ifeq ($(CONFIG_BLUETOOTH_SPP), y)
CSRCS += framework/socket/bt_spp.c
CSRCS += service/ipc/socket/src/bt_socket_spp.c
endif #CONFIG_BLUETOOTH_SPP

ifeq ($(CONFIG_BLUETOOTH_HID_DEVICE), y)
CSRCS += framework/socket/bt_hid_device.c
CSRCS += service/ipc/socket/src/bt_socket_hid_device.c
endif #CONFIG_BLUETOOTH_HID_DEVICE

ifeq ($(CONFIG_BLUETOOTH_GATT_CLIENT), y)
CSRCS += framework/socket/bt_gattc.c
CSRCS += service/ipc/socket/src/bt_socket_gattc.c
endif #CONFIG_BLUETOOTH_GATT_CLIENT

ifeq ($(CONFIG_BLUETOOTH_GATT_SERVER), y)
CSRCS += framework/socket/bt_gatts.c
CSRCS += service/ipc/socket/src/bt_socket_gatts.c
endif #CONFIG_BLUETOOTH_GATT_SERVER

ifeq ($(CONFIG_BLUETOOTH_L2CAP), y)
CSRCS += framework/socket/bt_l2cap.c
CSRCS += service/ipc/socket/src/bt_socket_l2cap.c
endif #CONFIG_BLUETOOTH_L2CAP

ifeq ($(CONFIG_BLUETOOTH_BLE_ADV), y)
CSRCS += framework/socket/bt_le_advertiser.c
CSRCS += service/ipc/socket/src/bt_socket_advertiser.c
endif #CONFIG_BLUETOOTH_BLE_ADV

ifeq ($(CONFIG_BLUETOOTH_BLE_SCAN), y)
CSRCS += framework/socket/bt_le_scan.c
CSRCS += service/ipc/socket/src/bt_socket_scan.c
endif #CONFIG_BLUETOOTH_BLE_SCAN

ifeq ($(CONFIG_BLUETOOTH_PAN), y)
CSRCS += framework/socket/bt_pan.c
CSRCS += service/ipc/socket/src/bt_socket_pan.c
endif #CONFIG_BLUETOOTH_PAN

ifeq ($(CONFIG_BLUETOOTH_LOG), y)
CSRCS += framework/socket/bt_trace.c
CSRCS += service/ipc/socket/src/bt_socket_log.c
endif #CONFIG_BLUETOOTH_BLE_AUDIO

ifeq ($(CONFIG_BLUETOOTH_LE_CS), y)
CSRCS += framework/socket/bt_cs.c
CSRCS += service/ipc/socket/src/bt_socket_cs.c
endif #CONFIG_BLUETOOTH_LE_CS

CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/ipc/socket/include
ifeq ($(CONFIG_BLUETOOTH_FRAMEWORK_ASYNC), y)
CSRCS += framework/socket/async/*.c
CSRCS += framework/btwrap/async/*.c
endif #CONFIG_BLUETOOTH_FRAMEWORK_ASYNC
endif #CONFIG_BLUETOOTH_FRAMEWORK_SOCKET_IPC

CSRCS += service/src/manager_service.c
CSRCS += service/common/index_allocator.c

ifeq ($(CONFIG_BLUETOOTH_CONNECTION_MANAGER), y)
CSRCS += service/src/connection_manager.c
endif #CONFIG_BLUETOOTH_CONNECTION_MANAGER

ifeq ($(CONFIG_BLUETOOTH_STORAGE_PROPERTY_SUPPORT), y)
CSRCS += service/common/storage_property.c
else
CSRCS += service/common/bt_storage.c
endif

ifeq ($(CONFIG_BLUETOOTH_STORAGE_UPDATE), y)
CSRCS += tools/storage_update/storage_version_4.c
CSRCS += tools/storage_update/storage_version_5.c
endif #CONFIG_BLUETOOTH_STORAGE_UPDATE

ifeq ($(CONFIG_BLUETOOTH_DEBUG_MEMORY),y)
CSRCS += debug/bt_memory.c
endif

ifeq ($(CONFIG_BLUETOOTH_DEBUG_TRACE), y)
CSRCS += service/debug/bt_trace.c
endif

ifeq ($(CONFIG_BLUETOOTH_SERVICE), y)
	CSRCS += service/common/service_loop.c
	CSRCS += service/src/adapter_service.c
	CSRCS += service/src/adapter_state.c
	CSRCS += service/src/btservice.c
	CSRCS += service/src/device.c
	ifeq ($(CONFIG_BLUETOOTH_BREDR_SUPPORT), y)
	CSRCS += service/src/power_manager.c
	endif #CONFIG_BLUETOOTH_BREDR_SUPPORT
	CSRCS += service/vendor/bt_vendor.c
	CSRCS += service/src/hci_parser.c
ifeq ($(CONFIG_BLUETOOTH_BLE_ADV), y)
	CSRCS += service/src/advertising.c
endif #CONFIG_BLUETOOTH_BLE_ADV
ifeq ($(CONFIG_BLUETOOTH_BLE_SCAN), y)
	CSRCS += service/src/scan_manager.c
	CSRCS += service/src/scan_record.c
	CSRCS += service/src/scan_filter.c
endif #CONFIG_BLUETOOTH_BLE_SCAN
ifeq ($(CONFIG_BLUETOOTH_L2CAP), y)
	CSRCS += service/src/l2cap_service.c
endif #CONFIG_BLUETOOTH_L2CAP
ifeq ($(CONFIG_LE_DLF_SUPPORT), y)
	CSRCS += service/src/connection_manager_dlf.c
endif #CONFIG_LE_DLF_SUPPORT
	CSRCS += service/stacks/*.c
ifneq ($(CONFIG_BLUETOOTH_STACK_BREDR_BLUELET)$(CONFIG_BLUETOOTH_STACK_LE_BLUELET),)
	CSRCS += service/stacks/bluelet/*.c
endif
ifneq ($(CONFIG_BLUETOOTH_STACK_BREDR_ZBLUE)$(CONFIG_BLUETOOTH_STACK_LE_ZBLUE),)
	CSRCS += service/stacks/zephyr/hci_h4.c
	CSRCS += service/stacks/zephyr/sal_debug_interface.c
	CSRCS += service/stacks/zephyr/sal_zblue.c
	CSRCS += service/stacks/zephyr/sal_adapter_interface.c
	ifeq ($(CONFIG_BLUETOOTH_CONNECTION_MANAGER), y)
	CSRCS += service/stacks/zephyr/sal_connection_manager.c
	endif
ifeq ($(CONFIG_BLUETOOTH_SPP), y)
	CSRCS += service/stacks/zephyr/sal_spp_interface.c
endif #CONFIG_BLUETOOTH_SPP
ifeq ($(CONFIG_BLUETOOTH_A2DP), y)
	CSRCS += service/stacks/zephyr/sal_a2dp_interface.c
endif #CONFIG_BLUETOOTH_A2DP
ifneq ($(CONFIG_BLUETOOTH_AVRCP_CONTROL)$(CONFIG_BLUETOOTH_AVRCP_TARGET),)
	CSRCS += service/stacks/zephyr/sal_avrcp_interface.c
endif #CONFIG_BLUETOOTH_AVRCP_CONTROL/CONFIG_BLUETOOTH_AVRCP_TARGET

ifeq ($(CONFIG_BLUETOOTH_HFP_HF), y)
	CSRCS += service/stacks/zephyr/sal_hfp_hf_interface.c
endif #CONFIG_BLUETOOTH_HFP_HF
ifeq ($(CONFIG_BLUETOOTH_HFP_AG), y)
	CSRCS += service/stacks/zephyr/sal_hfp_ag_interface.c
endif #CONFIG_BLUETOOTH_HFP_AG

ifeq ($(CONFIG_BLUETOOTH_HID_DEVICE), y)
	CSRCS += service/stacks/zephyr/sal_hid_device_interface.c
endif #CONFIG_BLUETOOTH_HID_DEVICE

ifeq ($(CONFIG_BLUETOOTH_STACK_LE_ZBLUE), y)
	CSRCS += service/stacks/zephyr/sal_adapter_le_interface.c
ifeq ($(CONFIG_BLUETOOTH_BLE_ADV), y)
	CSRCS += service/stacks/zephyr/sal_le_advertise_interface.c
endif #CONFIG_BLUETOOTH_BLE_ADV
ifeq ($(CONFIG_BLUETOOTH_BLE_SCAN), y)
	CSRCS += service/stacks/zephyr/sal_le_scan_interface.c
endif #CONFIG_BLUETOOTH_BLE_SCAN
ifeq ($(CONFIG_BLUETOOTH_GATT_CLIENT), y)
	CSRCS += service/stacks/zephyr/sal_gatt_client_interface.c
endif #CONFIG_BLUETOOTH_GATT_CLIENT
ifeq ($(CONFIG_BLUETOOTH_GATT_SERVER), y)
	CSRCS += service/stacks/zephyr/sal_gatt_server_interface.c
endif #CONFIG_BLUETOOTH_GATT_SERVER
ifeq ($(CONFIG_BLUETOOTH_LE_CS), y)
	CSRCS += service/stacks/zephyr/sal_le_cs_interface.c
endif #CONFIG_BLUETOOTH_LE_CS
endif #CONFIG_BLUETOOTH_STACK_LE_ZBLUE

endif
ifeq ($(CONFIG_BLUETOOTH_BLE_AUDIO),)
  CSRCS := $(filter-out $(wildcard service/stacks/bluelet/sal_lea_*),$(wildcard $(CSRCS)))
endif #CONFIG_BLUETOOTH_BLE_AUDIO
	CSRCS += service/profiles/*.c
	CSRCS += service/profiles/system/*.c
ifeq ($(CONFIG_BLUETOOTH_A2DP),)
	CSRCS := $(filter-out $(wildcard service/profiles/system/bt_player.c),$(wildcard $(CSRCS)))
endif #CONFIG_BLUETOOTH_A2DP
ifeq ($(findstring y, $(CONFIG_BLUETOOTH_A2DP)_$(CONFIG_BLUETOOTH_HFP_AG)_$(CONFIG_BLUETOOTH_HFP_HF)_$(CONFIG_BLUETOOTH_BLE_AUDIO)), )
	CSRCS := $(filter-out $(wildcard service/profiles/system/media_system.c),$(wildcard $(CSRCS)))
else
	CSRCS += service/profiles/audio_interface/audio_control.c
	ifeq ($(CONFIG_AUDIOUTILS_TINYCOMPRESS), y)
		CSRCS += service/profiles/audio_interface/audio_transport.c
		CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/service/profiles/audio_interface/include
	endif #CONFIG_AUDIOUTILS_TINYCOMPRESS
endif #CONFIG_BLUETOOTH_A2DP/CONFIG_BLUETOOTH_HFP_AG/CONFIG_BLUETOOTH_HFP_HF
ifeq ($(CONFIG_MICO_MEDIA_MAIN_PLAYER),y)
	CFLAGS += ${INCDIR_PREFIX}${TOPDIR}/../vendor/xiaomi/miai/mediaplayer/include
endif #CONFIG_MICO_MEDIA_MAIN_PLAYER
ifeq ($(CONFIG_BLUETOOTH_GATT_CLIENT), y)
	CSRCS += service/profiles/gatt/gattc_event.c
	CSRCS += service/profiles/gatt/gattc_service.c
endif #CONFIG_BLUETOOTH_GATT_CLIENT
ifeq ($(CONFIG_BLUETOOTH_GATT_SERVER), y)
	CSRCS += service/profiles/gatt/gatts_event.c
	CSRCS += service/profiles/gatt/gatts_service.c
endif #CONFIG_BLUETOOTH_GATT_SERVER

ifeq ($(CONFIG_BLUETOOTH_A2DP), y)
  CSRCS += service/profiles/a2dp/*.c
  CSRCS += service/profiles/a2dp/codec/*.c
  CSRCS += service/profiles/avrcp/*.c
  CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles/a2dp
  CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles/a2dp/codec
  CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles/avrcp
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
	ifeq ($(CONFIG_PHONE_SERVICE), y)
		CSRCS := $(filter-out $(wildcard service/profiles/system/telephony_interface.c) $(wildcard service/profiles/hfp_ag/hfp_ag_tele_service.c), $(wildcard $(CSRCS)))
	else
		CSRCS := $(filter-out $(wildcard service/profiles/hfp_ag/hfp_ag_tele_phone_service.c),$(wildcard $(CSRCS)))
	endif
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
	CSRCS += service/profiles/leaudio/codec/*.c
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

ifeq ($(CONFIG_BLUETOOTH_LOG), y)
CSRCS += service/utils/log_server.c
CSRCS += service/utils/btsnoop_log.c
CSRCS += service/utils/btsnoop_writer.c
CSRCS += service/utils/btsnoop_filter.c
endif #CONFIG_BLUETOOTH_LOG

ifeq ($(CONFIG_BLUETOOTH_LE_CS), y)
CSRCS += service/profiles/cs/*.c
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles/cs
endif #CONFIG_BLUETOOTH_LE_CS

ifeq ($(CONFIG_BLUETOOTH_HCI_FILTER), y)
CSRCS += service/vhal/bt_hci_filter.c
endif

CSRCS += service/vhal/bt_vhal.c
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/vhal
endif #CONFIG_BLUETOOTH_SERVICE

ifeq ($(CONFIG_APP_BT_SAMPLE_CODE), y)
ifeq ($(CONFIG_APP_BT_SAMPLE_CODE_BASIC), y)
	CSRCS += sample_code/basic/*.c
endif

ifeq ($(CONFIG_APP_BT_SAMPLE_CODE_ENABLE), y)
	CSRCS += sample_code/enable/*.c
endif

ifeq ($(CONFIG_APP_BT_SAMPLE_CODE_DISCOVERY), y)
	CSRCS += sample_code/discovery/*.c
endif

ifeq ($(CONFIG_APP_BT_SAMPLE_CODE_CREATEBOND), y)
	CSRCS += sample_code/createbond/*.c
endif

ifeq ($(CONFIG_APP_BT_SAMPLE_CODE_ACCEPTBOND), y)
	CSRCS += sample_code/acceptbond/*.c
endif
endif #CONFIG_APP_BT_SAMPLE_CODE

ifeq ($(CONFIG_BLUETOOTH_TOOLS), y)
	CSRCS += tools/utils.c
	CSRCS += tools/uv_thread_loop.c
ifeq ($(CONFIG_BLUETOOTH_FRAMEWORK_ASYNC), y)
	CSRCS += tools/async/gap.c
	CSRCS += tools/async/log.c
ifeq ($(CONFIG_BLUETOOTH_BLE_ADV), y)
	CSRCS += tools/async/adv.c
endif #CONFIG_BLUETOOTH_BLE_ADV
ifeq ($(CONFIG_BLUETOOTH_BLE_SCAN), y)
	CSRCS += tools/async/scan.c
endif
ifeq ($(CONFIG_BLUETOOTH_GATT), y)
	CSRCS += tools/async/gatt_client.c
endif #CONFIG_BLUETOOTH_GATT
endif
ifeq ($(CONFIG_BLUETOOTH_BLE_ADV), y)
	CSRCS += tools/adv.c
endif
ifeq ($(CONFIG_BLUETOOTH_BLE_SCAN), y)
	CSRCS += tools/scan.c
endif
ifeq ($(CONFIG_BLUETOOTH_L2CAP), y)
	CSRCS += tools/l2cap.c
endif #CONFIG_BLUETOOTH_L2CAP
ifeq ($(CONFIG_BLUETOOTH_A2DP_SINK), y)
	CSRCS += tools/a2dp_sink.c
endif #CONFIG_BLUETOOTH_A2DP_SINK
ifeq ($(CONFIG_BLUETOOTH_A2DP_SOURCE), y)
	CSRCS += tools/a2dp_source.c
endif #CONFIG_BLUETOOTH_A2DP_SOURCE
ifeq ($(CONFIG_BLUETOOTH_AVRCP_CONTROL), y)
	CSRCS += tools/avrcp_control.c
endif #CONFIG_BLUETOOTH_AVRCP_CONTROL
ifeq ($(CONFIG_BLUETOOTH_GATT_CLIENT), y)
	CSRCS += tools/gatt_client.c
endif #CONFIG_BLUETOOTH_GATT_CLIENT
ifeq ($(CONFIG_BLUETOOTH_GATT_SERVER), y)
	CSRCS += tools/gatt_server.c
endif #CONFIG_BLUETOOTH_GATT_SERVER
ifeq ($(CONFIG_BLUETOOTH_HFP_HF), y)
	CSRCS += tools/hfp_hf.c
endif #CONFIG_BLUETOOTH_HFP_HF

ifeq ($(CONFIG_BLUETOOTH_HFP_AG), y)
	CSRCS += tools/hfp_ag.c
endif #CONFIG_BLUETOOTH_HFP_AG
ifeq ($(CONFIG_BLUETOOTH_LOG), y)
CSRCS += tools/log.c
endif #CONFIG_BLUETOOTH_LOG
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

ifeq ($(CONFIG_BLUETOOTH_STORAGE_UPDATE), y)
	CSRCS += tools/storage_update/storage_tool.c
endif #CONFIG_BLUETOOTH_STORAGE_UPDATE

ifeq ($(CONFIG_BLUETOOTH_LE_CS), y)
	CSRCS += tools/le_cs.c
endif

endif

# framework/service/stack/tools dependence
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/framework/include

ifneq ($(CONFIG_LIB_DBUS_RPMSG_SERVER_CPUNAME)$(CONFIG_OFONO),)
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/dbus/dbus
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/glib/glib/glib
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/glib/glib
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/glib
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/system/utils/gdbus
endif

CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/src
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/common
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles/include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/profiles/system
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/stacks
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/stacks/include
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/vendor
CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/dfx

ifeq ($(CONFIG_BLUETOOTH_SERVICE), y)
ifneq ($(CONFIG_BLUETOOTH_STACK_BREDR_BLUELET)$(CONFIG_BLUETOOTH_STACK_LE_BLUELET),)
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/stacks/bluelet/include
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/bluelet/bluelet/src/samples/stack_adapter/inc
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/vendor/xiaomi/vela/bluelet/inc
endif
ifneq ($(CONFIG_BLUETOOTH_STACK_BREDR_ZBLUE)$(CONFIG_BLUETOOTH_STACK_LE_ZBLUE),)
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/stacks/zephyr/include
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/zblue/zblue/port/include/
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/zblue/zblue/subsys/bluetooth/host
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/zblue/zblue/subsys/settings/include/settings
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/zblue/zblue/subsys/bluetooth
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/external/zblue/zblue/port/include/kernel/include
endif
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/service/ipc
endif

ifeq ($(CONFIG_APP_BT_SAMPLE_CODE), y)
ifeq ($(CONFIG_APP_BT_SAMPLE_CODE_BASIC), y)
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/sample_code/basic
endif

ifeq ($(CONFIG_APP_BT_SAMPLE_CODE_ENABLE), y)
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/sample_code/enable
endif

ifeq ($(CONFIG_APP_BT_SAMPLE_CODE_DISCOVERY), y)
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/sample_code/discovery
endif

ifeq ($(CONFIG_APP_BT_SAMPLE_CODE_CREATEBOND), y)
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/sample_code/createbond
endif

ifeq ($(CONFIG_APP_BT_SAMPLE_CODE_ACCEPTBOND), y)
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/sample_code/acceptbond
endif
endif #CONFIG_APP_BT_SAMPLE_CODE

ifeq ($(CONFIG_BLUETOOTH_TOOLS), y)
	CFLAGS	+= ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/tools
endif

ifeq ($(CONFIG_BLUETOOTH_STORAGE_UPDATE), y)
	CFLAGS += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/tools/storage_update
endif #CONFIG_BLUETOOTH_STORAGE_UPDATE

ifeq ($(CONFIG_ARCH_SIM),y)
CFLAGS	 += -O0
endif
CFLAGS	 += -Wno-strict-prototypes #-fno-short-enums -Wl,-no-enum-size-warning #-Werror
PRIORITY  = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_BLUETOOTH_TASK_STACK_SIZE)
MODULE    = $(CONFIG_BLUETOOTH)

# if enabled bluetoothd
ifeq ($(CONFIG_BLUETOOTH_SERVER), y)
	PROGNAME += $(CONFIG_BLUETOOTH_SERVER_NAME)
	MAINSRC  += service/src/main.c
endif

ifeq ($(CONFIG_APP_BT_SAMPLE_CODE), y)
ifeq ($(CONFIG_APP_BT_SAMPLE_CODE_BASIC), y)
	PROGNAME  += bt_basic
	MAINSRC   += sample_code/basic/basic.c
endif

ifeq ($(CONFIG_APP_BT_SAMPLE_CODE_ENABLE), y)
	PROGNAME  += bt_enable
	MAINSRC   += sample_code/enable/enable.c
endif

ifeq ($(CONFIG_APP_BT_SAMPLE_CODE_DISCOVERY), y)
	PROGNAME  += bt_discovery
	MAINSRC   += sample_code/discovery/discovery.c
endif

ifeq ($(CONFIG_APP_BT_SAMPLE_CODE_CREATEBOND), y)
	PROGNAME  += bt_createbond
	MAINSRC   += sample_code/createbond/createbond.c
endif

ifeq ($(CONFIG_APP_BT_SAMPLE_CODE_ACCEPTBOND), y)
	PROGNAME  += bt_acceptbond
	MAINSRC   += sample_code/acceptbond/acceptbond.c
endif
endif #CONFIG_APP_BT_SAMPLE_CODE

# if enabled bttool
ifeq ($(CONFIG_BLUETOOTH_TOOLS), y)
	PROGNAME += bttool
	MAINSRC  += tools/bt_tools.c
	PROGNAME += adapter_test
	MAINSRC  += tests/adapter_test.c
endif

ifeq ($(CONFIG_BLUETOOTH_UPGRADE), y)
	PROGNAME += bt_upgrade
	MAINSRC  += tools/storage_transform.c
endif

ifeq ($(CONFIG_BLUETOOTH_STORAGE_UPDATE), y)
	PROGNAME += bt_storage_update
	MAINSRC  += tools/storage_update/storage_update.c
endif #CONFIG_BLUETOOTH_STORAGE_UPDATE
endif

ASRCS := $(wildcard $(ASRCS))
CSRCS := $(wildcard $(CSRCS))
CXXSRCS := $(wildcard $(CXXSRCS))
MAINSRC := $(wildcard $(MAINSRC))

NOEXPORTSRCS = $(ASRCS)$(CSRCS)$(CXXSRCS)$(MAINSRC)

ifeq ($(CONFIG_BLUETOOTH_FEATURE),y)
CFLAGS    += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/feature/include
CFLAGS    += ${INCDIR_PREFIX}$(APPDIR)/frameworks/runtimes/feature/include

CSRCS     += feature/src/system_bluetooth.c
CSRCS     += feature/src/system_bluetooth_impl.c
CSRCS     += feature/src/feature_bluetooth_util.c
CSRCS     += feature/src/system_bluetooth_bt.c
CSRCS     += feature/src/system_bluetooth_bt_impl.c
CSRCS     += feature/src/feature_bluetooth_callback.c

ifeq ($(CONFIG_BLUETOOTH_A2DP_SINK), y)
CSRCS     += feature/src/system_bluetooth_bt_a2dpsink.c
CSRCS     += feature/src/system_bluetooth_bt_a2dpsink_impl.c
endif
ifeq ($(CONFIG_BLUETOOTH_AVRCP_CONTROL), y)
CSRCS     += feature/src/system_bluetooth_bt_avrcpcontrol.c
CSRCS     += feature/src/system_bluetooth_bt_avrcpcontrol_impl.c
endif

ifneq ($(CONFIG_BLUETOOTH_DFX),)
  DFX_XML=$(APPDIR)/frameworks/system/dfx/src/onetrack/xml
  BLUETOOTH_XML=$(APPDIR)/frameworks/connectivity/bluetooth/dfx/event_bt.xml
context::
	@cp $(BLUETOOTH_XML) $(DFX_XML)
endif

depend::
	$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp \
		$(APPDIR)/frameworks/connectivity/bluetooth/feature/jidl/bluetooth.jidl --out-dir \
		$(APPDIR)/frameworks/connectivity/bluetooth/feature/src --header system_bluetooth.h --source system_bluetooth.c
	$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp \
		$(APPDIR)/frameworks/connectivity/bluetooth/feature/jidl/bluetooth_bt.jidl --out-dir \
		$(APPDIR)/frameworks/connectivity/bluetooth/feature/src --header system_bluetooth_bt.h --source system_bluetooth_bt.c
ifeq ($(CONFIG_BLUETOOTH_A2DP_SINK), y)
	$(APPDIR)/../prebuilts/tools/rust/bin/jidl/jidl_gen_cpp \
		$(APPDIR)/frameworks/connectivity/bluetooth/feature/jidl/bluetooth_bt_a2dpsink.jidl --out-dir \
		$(APPDIR)/frameworks/connectivity/bluetooth/feature/src --header system_bluetooth_bt_a2dpsink.h --source system_bluetooth_bt_a2dpsink.c
endif
ifeq ($(CONFIG_BLUETOOTH_AVRCP_CONTROL), y)
	@python3 $(APPDIR)/frameworks/runtimes/feature/tools/jidl/jsongensource.py \
		$(APPDIR)/frameworks/connectivity/bluetooth/feature/jidl/bluetooth_bt_avrcpcontrol.jidl -out-dir \
		$(APPDIR)/frameworks/connectivity/bluetooth/feature/src -header system_bluetooth_bt_avrcpcontrol.h -source system_bluetooth_bt_avrcpcontrol.c
endif
else ifeq ($(CONFIG_BLUETOOTH_FEATURE_ASYNC), y)
include $(APPDIR)/frameworks/runtimes/feature/Make.defs
CFLAGS    += ${INCDIR_PREFIX}$(APPDIR)/frameworks/connectivity/bluetooth/feature/feature_async/include

CSRCS     += feature/feature_async/src/bluetooth.c
CSRCS     += feature/feature_async/src/bluetooth_impl.c
CSRCS     += feature/feature_async/src/feature_bluetooth_util.c
CSRCS     += feature/feature_async/src/bluetooth_ble.c
CSRCS     += feature/feature_async/src/bluetooth_ble_impl.c

depend::
	@python3 $(APPDIR)/frameworks/runtimes/feature/tools/jidl/jsongensource.py \
		$(APPDIR)/frameworks/connectivity/bluetooth/feature/feature_async/jidl/bluetooth.jidl -out-dir \
		$(APPDIR)/frameworks/connectivity/bluetooth/feature/feature_async/src -header bluetooth.h -source bluetooth.c
	@python3 $(APPDIR)/frameworks/runtimes/feature/tools/jidl/jsongensource.py \
		$(APPDIR)/frameworks/connectivity/bluetooth/feature/feature_async/jidl/bluetooth_ble.jidl -out-dir \
		$(APPDIR)/frameworks/connectivity/bluetooth/feature/feature_async/src -header bluetooth_ble.h -source bluetooth_ble.c
else
endif

ifneq ($(NOEXPORTSRCS),)
BIN := $(APPDIR)/staging/libbluetooth.a
endif

include $(APPDIR)/Application.mk

