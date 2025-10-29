/****************************************************************************
 *
 *   Copyright (C) 2025 Xiaomi InC. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#define LOG_TAG "bt_ipc_code"

#include "bt_utils.h"
#include "bt_ipc_code.h"
#include "bt_message.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

static const char* bt_legacy_ipc_code_to_string(uint32_t type)
{
    if (
        (type >= BT_A2DP_SINK_MESSAGE_START && type <= BT_A2DP_SINK_MESSAGE_END) ||
        (type >= BT_A2DP_SINK_CALLBACK_START && type <= BT_A2DP_SINK_CALLBACK_END)
    ){
        return bt_a2dp_sink_ipc_code_to_string(type);
    } else if (
        (type >= BT_A2DP_SOURCE_MESSAGE_START && type <= BT_A2DP_SOURCE_MESSAGE_END) ||
        (type >= BT_A2DP_SOURCE_CALLBACK_START && type <= BT_A2DP_SOURCE_CALLBACK_END)
    ){
        return bt_a2dp_source_ipc_code_to_string(type);
    } else {
        return NULL;
    }
}

char* bt_ipc_code_to_string(uint32_t code)
{
    char* result = NULL;

    switch (BT_IPC_GET_GROUP(code)) {
    case BT_IPC_CODE_GROUP_LEGACY:
        result = bt_legacy_ipc_code_to_string(code);
        break;
    case BT_IPC_CODE_GROUP_MANAGER:
        result = bt_manager_ipc_code_to_string(code);
        break;
    case BT_IPC_CODE_GROUP_ADAPTER:
        result = bt_adapter_ipc_code_to_string(code);
        break;
    case BT_IPC_CODE_GROUP_DEVICE:
        result = bt_device_ipc_code_to_string(code);
        break;
    case BT_IPC_CODE_GROUP_LOG:
        result = bt_log_ipc_code_to_string(code);
        break;
    case BT_IPC_CODE_GROUP_BLE_ADVERTISER:
        result = bt_advertiser_ipc_code_to_string(code);
        break;
    case BT_IPC_CODE_GROUP_BLE_SCAN:
        result = bt_scan_ipc_code_to_string(code);
        break;
    case BT_IPC_CODE_GROUP_L2CAP:
        result = bt_l2cap_ipc_code_to_string(code);
        break;
    case BT_IPC_CODE_GROUP_A2DP_SRC:
        result = bt_a2dp_source_ipc_code_to_string(code);
        break;
    case BT_IPC_CODE_GROUP_A2DP_SINK:
        result = bt_a2dp_sink_ipc_code_to_string(code);
        break;
    case BT_IPC_CODE_GROUP_AVRCP_CT:
        result = bt_avrcp_control_ipc_code_to_string(code);
        break;
    case BT_IPC_CODE_GROUP_AVRCP_TG:
        result = bt_avrcp_target_ipc_code_to_string(code);
        break;
    case BT_IPC_CODE_GROUP_HFP_HF:
        result = bt_hfp_hf_ipc_code_to_string(code);
        break;
    case BT_IPC_CODE_GROUP_HFP_AG:
        result = bt_hfp_ag_ipc_code_to_string(code);
        break;
    case BT_IPC_CODE_GROUP_SPP:
        result = bt_spp_ipc_code_to_string(code);
        break;
    case BT_IPC_CODE_GROUP_HID_DEV:
        result = bt_hid_device_ipc_code_to_string(code);
        break;
    case BT_IPC_CODE_GROUP_PANU:
        result = bt_pan_ipc_code_to_string(code);
        break;
    case BT_IPC_CODE_GROUP_GATTC:
        result = bt_gattc_ipc_code_to_string(code);
        break;
    case BT_IPC_CODE_GROUP_GATTS:
        result = bt_gatts_ipc_code_to_string(code);
        break;
    default:
        (void)snprintf(result, sizeof(result), "UNKNOWN_CODE(0x%" PRIx32 ")", code);
        break;
    }

    return result;
}
