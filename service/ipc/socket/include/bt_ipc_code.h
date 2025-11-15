/****************************************************************************
 *  Copyright (C) 2025 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/
#ifndef _BT_IPC_CODE_H__
#define _BT_IPC_CODE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "bt_profile.h"

// BT_IPC_CODE_TYPE
#define BT_IPC_CODE_TYPE_COMMAND (0)
#define BT_IPC_CODE_TYPE_CALLBACK (1)

// BT_IPC_CODE_GROUP
#define BT_IPC_CODE_GROUP_LEGACY (0)
#define BT_IPC_CODE_GROUP_ADAPTER (1)
#define BT_IPC_CODE_GROUP_DEVICE (2)
#define BT_IPC_CODE_GROUP_MANAGER (3)
#define BT_IPC_CODE_GROUP_LOG (4)
#define BT_IPC_CODE_GROUP_BLE_ADVERTISER (5)
#define BT_IPC_CODE_GROUP_BLE_SCAN (6)
#define BT_IPC_CODE_GROUP_L2CAP (7)

#define BT_IPC_CODE_GROUP_PROFILES (16)
#define BT_IPC_CODE_GROUP_A2DP_SRC (BT_IPC_CODE_GROUP_PROFILES + PROFILE_A2DP)
#define BT_IPC_CODE_GROUP_A2DP_SINK (BT_IPC_CODE_GROUP_PROFILES + PROFILE_A2DP_SINK)
#define BT_IPC_CODE_GROUP_AVRCP_CT (BT_IPC_CODE_GROUP_PROFILES + PROFILE_AVRCP_CT)
#define BT_IPC_CODE_GROUP_AVRCP_TG (BT_IPC_CODE_GROUP_PROFILES + PROFILE_AVRCP_TG)
#define BT_IPC_CODE_GROUP_HFP_HF (BT_IPC_CODE_GROUP_PROFILES + PROFILE_HFP_HF)
#define BT_IPC_CODE_GROUP_HFP_AG (BT_IPC_CODE_GROUP_PROFILES + PROFILE_HFP_AG)
#define BT_IPC_CODE_GROUP_CS (BT_IPC_CODE_GROUP_PROFILES + PROFILE_CS)
#define BT_IPC_CODE_GROUP_SPP (BT_IPC_CODE_GROUP_PROFILES + PROFILE_SPP)
#define BT_IPC_CODE_GROUP_HID_DEV (BT_IPC_CODE_GROUP_PROFILES + PROFILE_HID_DEV)
#define BT_IPC_CODE_GROUP_PANU (BT_IPC_CODE_GROUP_PROFILES + PROFILE_PANU)
#define BT_IPC_CODE_GROUP_GATTC (BT_IPC_CODE_GROUP_PROFILES + PROFILE_GATTC)
#define BT_IPC_CODE_GROUP_GATTS (BT_IPC_CODE_GROUP_PROFILES + PROFILE_GATTS)
#define BT_IPC_CODE_GROUP_LEAUDIO_SERVER (BT_IPC_CODE_GROUP_PROFILES + PROFILE_LEAUDIO_SERVER)
#define BT_IPC_CODE_GROUP_LEAUDIO_MCP (BT_IPC_CODE_GROUP_PROFILES + PROFILE_LEAUDIO_MCP)
#define BT_IPC_CODE_GROUP_LEAUDIO_CCP (BT_IPC_CODE_GROUP_PROFILES + PROFILE_LEAUDIO_CCP)
#define BT_IPC_CODE_GROUP_LEAUDIO_VMICS (BT_IPC_CODE_GROUP_PROFILES + PROFILE_LEAUDIO_VMICS)
#define BT_IPC_CODE_GROUP_LEAUDIO_CLIENT (BT_IPC_CODE_GROUP_PROFILES + PROFILE_LEAUDIO_CLIENT)
#define BT_IPC_CODE_GROUP_LEAUDIO_MCS (BT_IPC_CODE_GROUP_PROFILES + PROFILE_LEAUDIO_MCS)
#define BT_IPC_CODE_GROUP_LEAUDIO_TBS (BT_IPC_CODE_GROUP_PROFILES + PROFILE_LEAUDIO_TBS)
#define BT_IPC_CODE_GROUP_LEAUDIO_VMICP (BT_IPC_CODE_GROUP_PROFILES + PROFILE_LEAUDIO_VMICP)
#define BT_IPC_CODE_GROUP_LEAUDIO_VMICP (BT_IPC_CODE_GROUP_PROFILES + PROFILE_LEAUDIO_VMICP)

#define BT_IPC_CODE(type, group, subcode) (((uint32_t)(type) << 24) | ((uint32_t)(group) << 16) | ((uint32_t)(subcode)))

#define BT_IPC_GET_TYPE(code) (((uint32_t)(code) >> 24) & 0x01)
#define BT_IPC_GET_GROUP(code) (((uint32_t)(code) >> 16) & 0xFF)
#define BT_IPC_GET_SUBCODE(code) ((uint32_t)(code)&0xFFFF)

#define BT_IPC_CODE_SUBCODE_MAX_NUM (0xFFFF)

#define BT_IPC_CODE_CHECK_RANGE(code, begin, end) (((uint32_t)(code) > (uint32_t)(begin)) && ((uint32_t)(code) < (uint32_t)(end)))
#define BT_IPC_CODE_CHECK_TYPE(code, type) ((uint32_t)(BT_IPC_GET_TYPE(code)) == (uint32_t)(type))
#define BT_IPC_CODE_CHECK_GROUP(code, group) ((uint32_t)(BT_IPC_GET_GROUP(code)) == (uint32_t)(group))
#define BT_IPC_CODE_CHECK_SUBCODE(code, subcode) ((uint32_t)(BT_IPC_GET_SUBCODE(code)) == (uint32_t)(subcode))

#ifdef __cplusplus
}
#endif

#endif /* _BT_IPC_CODE_H__ */
