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

CSRCS += btservice/btservice/bts_service.c
CSRCS += btservice/btservice/bts_service_interface.c
CSRCS += btservice/gap/bts_gap.c
CSRCS += btservice/gap/bts_gap_service.c
CSRCS += btservice/gap/bts_gap_data.c
CSRCS += btmanager/btm_manager.c
CSRCS +=btmanager/btm_gap.c

ifeq ($(CONFIG_BLUETOOTH_A2DP_SRC),y)
	CSRCS +=btmanager/btm_a2dp_source.c
	CSRCS += btservice/a2dp_source/bts_a2dp_source.c
	CSRCS += btservice/a2dp_source/bts_a2dp_source_service.c
	CSRCS += btservice/a2dp_source/bts_a2dp_source_audio.c
	CSRCS += btservice/a2dp_source/bts_a2dp_control.c
	CSRCS += btservice/a2dp_source/bts_a2dp_event.c
	CSRCS += btservice/a2dp_source/bts_a2dp_state_machine.c
	CSRCS += btservice/a2dp_source/bts_a2dp_codec.c
	CSRCS += udrv/uv/a2dp_ipc.c
	CSRCS += btservice/a2dp_source/codec/a2dp_codec_sbc.c
	CSRCS += btservice/a2dp_source/codec/sbc_encoder.c
endif

ifeq ($(CONFIG_BLUETOOTH_AVRCP_TG),y)
	CSRCS +=btservice/avrcp_target/bts_avrcp_target.c
endif

ifeq ($(CONFIG_BLUETOOTH_HFP_HF),y)
	CSRCS +=btmanager/btm_hfp_hf.c
	CSRCS +=btservice/hfp_client/bts_hf_client.c
	CSRCS +=btservice/hfp_client/bts_hf_client_event.c
	CSRCS +=btservice/hfp_client/bts_hf_client_service.c
	CSRCS +=btservice/hfp_client/bts_hf_client_state_machine.c
endif

ifeq ($(CONFIG_BLUETOOTH_SPP),y)
	CSRCS +=udrv/uv/euv_pty.c
	CSRCS +=btmanager/btm_spp.c
	CSRCS +=btservice/spp/bts_spp.c
	CSRCS +=btservice/spp/bts_spp_service.c
endif

ifeq ($(CONFIG_BLUETOOTH_LE_SCAN),y)
	CSRCS +=btmanager/btm_le_scan.c
	CSRCS +=btservice/gatt/bts_le_scan.c
	CSRCS +=btservice/gatt/bts_gatt_service.c
endif

ifeq ($(CONFIG_BLUETOOTH_LE_ADVERTISE),y)
	CSRCS +=btmanager/btm_le_advertise.c
	CSRCS +=btservice/gatt/bts_le_advertise.c
	CSRCS +=btservice/gatt/bts_gatt_service.c
endif

ifeq ($(CONFIG_BLUETOOTH_GATT_CLIENT),y)
	CSRCS +=btmanager/btm_gatt_client.c
	CSRCS +=btservice/gatt/bts_gatt_client.c
	CSRCS +=btservice/gatt/bts_gatt_service.c
endif

ifeq ($(CONFIG_BLUETOOTH_GATT_SERVER),y)
	CSRCS +=btmanager/btm_gatt_server.c
	CSRCS +=btservice/gatt/bts_gatt_server.c
	CSRCS +=btservice/gatt/bts_gatt_service.c
endif

ifeq ($(CONFIG_BLUETOOTH_TOOL_CHAIN), y)
ifeq ($(CONFIG_BLUETOOTH_SPP),y)
	CSRCS +=tools/spp.c
endif
ifeq ($(CONFIG_BLUETOOTH_HFP_HF),y)
	CSRCS +=tools/hf_client.c
endif
ifeq ($(CONFIG_BLUETOOTH_HFP_HF),y)
	CSRCS +=tools/a2dp_source.c
endif
ifeq ($(CONFIG_BLUETOOTH_GATT_SERVER),y)
	CSRCS +=tools/gatt_server.c
endif
ifeq ($(CONFIG_BLUETOOTH_GATT_CLIENT),y)
	CSRCS +=tools/gatt_client.c
endif
endif

CSRCS +=utils/uuid.c
CSRCS +=utils/utils.c
CSRCS +=btservice/state_machine/state_machine.c

CFLAGS   += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/frameworks/bluetooth/}
CFLAGS   += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/frameworks/bluetooth/include}
CFLAGS   += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/frameworks/bluetooth/btservice/include}
CFLAGS   += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/frameworks/bluetooth/btservice/state_machine}
CFLAGS   += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/frameworks/bluetooth/utils}
CFLAGS   += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/frameworks/bluetooth/udrv/include}
CFLAGS   += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/frameworks/bluetooth/btservice/a2dp_source/codec}
ifeq ($(CONFIG_BLUETOOTH_TOOL_CHAIN), y)
	CFLAGS   += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/frameworks/bluetooth/tools}
endif

CFLAGS   += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/external/bluelet/src/samples/template/stack_adapter_template/inc}
CFLAGS += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/vendor/xiaomi/vela/bluelet/inc}
CFLAGS   += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/external/bluelet/src/stack/portings/btunix}
CFLAGS   += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/external/bluelet/src/stack/include}

CFLAGS   += ${shell $(INCDIR) $(INCDIROPT) "$(CC)" $(APPDIR)/system/libuv/libuv/include}
CFLAGS   += -I $(APPDIR)/external/bluelet/

PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = 40960
MODULE    = $(CONFIG_BLUETOOTH)

ifeq ($(CONFIG_BLUETOOTH_SAMPLE_GATTC), y)
	MAINSRC   += samples/test_gattc.c
	PROGNAME  += btsample_gattc
endif

ifeq ($(CONFIG_BLUETOOTH_SAMPLE_GATTS), y)
	MAINSRC   += samples/test_gatts.c
	PROGNAME  += btsample_gatts
endif

ifeq ($(CONFIG_BLUETOOTH_SAMPLE_GAP), y)
	MAINSRC   += samples/test_gap.c
	PROGNAME  += btsample_gap
endif

ifeq ($(CONFIG_BLUETOOTH_TOOL_CHAIN), y)
	PROGNAME 	+= bttool
	MAINSRC		+= tools/bt_tools.c
endif

include $(APPDIR)/Application.mk

