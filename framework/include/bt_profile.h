/****************************************************************************
 *  Copyright (C) 2022 Xiaomi Corporation
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
#ifndef _BT_PROFILE_H__
#define _BT_PROFILE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define PROFILE_A2DP_NAME "A2DP-Src"
#define PROFILE_A2DP_SINK_NAME "A2DP-Sink"
#define PROFILE_AVRCP_CT_NAME "AVRCP-CT"
#define PROFILE_AVRCP_TG_NAME "AVRCP-TG"
#define PROFILE_HFP_HF_NAME "HFP-HF"
#define PROFILE_HFP_AG_NAME "HFP-AG"
#define PROFILE_SPP_NAME "SPP"
#define PROFILE_HID_DEV_NAME "HID-DEV"
#define PROFILE_PANU_NAME "PANU"
#define PROFILE_GATTC_NAME "GATTC"
#define PROFILE_GATTS_NAME "GATTS"
#define PROFILE_MCS_NAME "MCS"
#define PROFILE_MCP_NAME "MCP"
#define PROFILE_TBS_NAME "TBS"
#define PROFILE_CCP_NAME "CCP"
#define PROFILE_VMICS_NAME "VMICS"
#define PROFILE_VMICP_NAME "VMICP"
#define PROFILE_LEA_CLIENT_NAME "LEA-CLIENT"
#define PROFILE_LEA_SERVER_NAME "LEA-SERVER"

enum profile_id {
    PROFILE_A2DP,
    PROFILE_A2DP_SINK,
    PROFILE_AVRCP_CT,
    PROFILE_AVRCP_TG,
    PROFILE_HFP_HF,
    PROFILE_HFP_AG,
    PROFILE_SPP,
    PROFILE_HID_DEV,
    PROFILE_PANU,
    PROFILE_GATTC,
    PROFILE_GATTS,
    PROFILE_LEAUDIO_SERVER,
    PROFILE_LEAUDIO_MCP,
    PROFILE_LEAUDIO_CCP,
    PROFILE_LEAUDIO_VMICS,
    PROFILE_LEAUDIO_CLIENT,
    PROFILE_LEAUDIO_MCS,
    PROFILE_LEAUDIO_TBS,
    PROFILE_LEAUDIO_VMICP,
    PROFILE_UNKOWN,
    PROFILE_MAX
};

#ifdef __cplusplus
}
#endif

#endif /* _BT_PROFILE_H__ */
