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

#ifndef _BT_DFX_H_
#define _BT_DFX_H_

#if defined(CONFIG_DFX) && defined(CONFIG_DFX_EVENT)
#include <dfx_debug.h>
#include <dfx_event.h>
#endif

#include "bt_dfx_event.h"
#include "bt_dfx_reason.h"

// br
#if defined(CONFIG_BLUETOOTH_DFX) && defined(CONFIG_BLUETOOTH_BREDR_SUPPORT)
#define BT_DFX_SEND_BR_EVENT(...) sendEventMisightF(__VA_ARGS__)
#else
#define BT_DFX_SEND_BR_EVENT(...)
#endif

#define BT_DFX_BR_GAP_INQUIRY_ERROR(reason)                                             \
    do {                                                                                \
        BT_LOGE("BT_DFX: brInquiryError: %s", reason);                                  \
        BT_DFX_SEND_BR_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_BR_GAP, BT_DFXC_BR_GAP_INQUIRY), \
            "%s:%s", "brInquiryError", reason);                                         \
    } while (0)

#define BT_DFX_BR_GAP_CONN_ERROR(reason)                                             \
    do {                                                                             \
        BT_LOGE("BT_DFX: brConnectError: %s", reason);                               \
        BT_DFX_SEND_BR_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_BR_GAP, BT_DFXC_BR_GAP_CONN), \
            "%s:%s", "brConnectError", reason);                                      \
    } while (0)

#define BT_DFX_BR_GAP_DISCONN_ERROR(reason)                                             \
    do {                                                                                \
        BT_LOGE("BT_DFX: brDisconnectError: %s", reason);                               \
        BT_DFX_SEND_BR_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_BR_GAP, BT_DFXC_BR_GAP_DISCONN), \
            "%s:%s", "brDisconnectError", reason);                                      \
    } while (0)

#define BT_DFX_BR_GAP_PAIR_ERROR(reason)                                             \
    do {                                                                             \
        BT_LOGE("BT_DFX: brPairError: %s", reason);                                  \
        BT_DFX_SEND_BR_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_BR_GAP, BT_DFXC_BR_GAP_PAIR), \
            "%s:%s", "brPairError", reason);                                         \
    } while (0)

// ble
#if defined(CONFIG_BLUETOOTH_DFX) && defined(CONFIG_BLUETOOTH_BLE_SUPPORT)
#define BT_DFX_SEND_LE_GAP_EVENT(...) sendEventMisightF(__VA_ARGS__)
#else
#define BT_DFX_SEND_LE_GAP_EVENT(...)
#endif

#ifdef CONFIG_BLUETOOTH_BLE_SCAN
#define BT_DFX_LE_GAP_SCAN_ERROR(reason)                                                 \
    do {                                                                                 \
        BT_LOGE("BT_DFX: bleScanError: %s", reason);                                     \
        BT_DFX_SEND_LE_GAP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_LE_GAP, BT_DFXC_LE_GAP_SCAN), \
            "%s:%s", "bleScanError", reason);                                            \
    } while (0)
#endif

#define BT_DFX_LE_GAP_CONN_ERROR(reason)                                                 \
    do {                                                                                 \
        BT_LOGE("BT_DFX: bleConnectError: %s", reason);                                  \
        BT_DFX_SEND_LE_GAP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_LE_GAP, BT_DFXC_LE_GAP_CONN), \
            "%s:%s", "bleConnectError", reason);                                         \
    } while (0)

#define BT_DFX_LE_GAP_DISCONN_ERROR(reason)                                                 \
    do {                                                                                    \
        BT_LOGE("BT_DFX: bleDisconnectError: %s", reason);                                  \
        BT_DFX_SEND_LE_GAP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_LE_GAP, BT_DFXC_LE_GAP_DISCONN), \
            "%s:%s", "bleDisconnectError", reason);                                         \
    } while (0)

#define BT_DFX_LE_GAP_PAIR_ERROR(reason)                                                 \
    do {                                                                                 \
        BT_LOGE("BT_DFX: blePairError: %s", reason);                                     \
        BT_DFX_SEND_LE_GAP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_LE_GAP, BT_DFXC_LE_GAP_PAIR), \
            "%s:%s", "blePairError", reason);                                            \
    } while (0)

// spp
#if defined(CONFIG_BLUETOOTH_DFX) && defined(CONFIG_BLUETOOTH_SPP)
#define BT_DFX_SEND_SPP_EVENT(...) sendEventMisightF(__VA_ARGS__)
#else
#define BT_DFX_SEND_SPP_EVENT(...)
#endif

#define BT_DFX_SPP_CONN_ERROR(reason)                                              \
    do {                                                                           \
        BT_LOGE("BT_DFX: btSppConnectError: %s", reason);                          \
        BT_DFX_SEND_SPP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_RFCOMM, BT_DFXC_SPP_CONN), \
            "%s:%s", "btSppConnectError", reason);                                 \
    } while (0)

#define BT_DFX_SPP_DISCONN_ERROR(reason, scn, port, role)                             \
    do {                                                                              \
        BT_LOGE("BT_DFX: btSppDisconnected: %s, scn: %d, port: %d, role: %d",         \
            reason, scn, port, role);                                                 \
        BT_DFX_SEND_SPP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_RFCOMM, BT_DFXC_SPP_DISCONN), \
            "%s:%s,%s:%d,%s:%d,%s:%s", "btSppDisconnected", reason, "scn", scn,       \
            "port", port, "role", role);                                              \
    } while (0)

// a2dp
#if defined(CONFIG_BLUETOOTH_DFX) && defined(CONFIG_BLUETOOTH_A2DP)
#define BT_DFX_SEND_A2DP_EVENT(...) sendEventMisightF(__VA_ARGS__)
#else
#define BT_DFX_SEND_A2DP_EVENT(...)
#endif

#define BT_DFX_A2DP_CONN_ERROR(reason)                                             \
    do {                                                                           \
        BT_LOGE("BT_DFX: btA2dpConnectError: %s", reason);                         \
        BT_DFX_SEND_A2DP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_A2DP, BT_DFXC_A2DP_CONN), \
            "%s:%s", "btA2dpConnectError", reason);                                \
    } while (0)

#define BT_DFX_A2DP_MEDIA_ERROR(reason)                                             \
    do {                                                                            \
        BT_LOGE("BT_DFX: btA2dpMediaError: %s", reason);                            \
        BT_DFX_SEND_A2DP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_A2DP, BT_DFXC_A2DP_MEDIA), \
            "%s:%s", "btA2dpMediaError", reason);                                   \
    } while (0)

#define BT_DFX_A2DP_OFFLOAD_ERROR(reason)                                             \
    do {                                                                              \
        BT_LOGE("BT_DFX: btA2dpOffloadError: %s", reason);                            \
        BT_DFX_SEND_A2DP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_A2DP, BT_DFXC_A2DP_OFFLOAD), \
            "%s:%s", "btA2dpOffloadError", reason);                                   \
    } while (0)

// avrcp
#if defined(CONFIG_BLUETOOTH_DFX) && (defined(CONFIG_BLUETOOTH_AVRCP_CONTROL) || defined(CONFIG_BLUETOOTH_AVRCP_TARGET))
#define BT_DFX_SEND_AVRCP_EVENT(...) sendEventMisightF(__VA_ARGS__)
#else
#define BT_DFX_SEND_AVRCP_EVENT(...)
#endif

#define BT_DFX_AVRCP_CONN_ERROR(reason)                                               \
    do {                                                                              \
        BT_LOGE("BT_DFX: btAvrcpConnectError: %s", reason);                           \
        BT_DFX_SEND_AVRCP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_AVRCP, BT_DFXC_AVRCP_CONN), \
            "%s:%s", "btAvrcpConnectError", reason);                                  \
    } while (0)

#define BT_DFX_AVRCP_CTRL_ERROR(reason)                                               \
    do {                                                                              \
        BT_LOGE("BT_DFX: btAvrcpCtrlError: %s", reason);                              \
        BT_DFX_SEND_AVRCP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_AVRCP, BT_DFXC_AVRCP_CTRL), \
            "%s:%s", "btAvrcpCtrlError", reason);                                     \
    } while (0)

#define BT_DFX_AVRCP_VOL_ERROR(reason)                                               \
    do {                                                                             \
        BT_LOGE("BT_DFX: btAvrcpVolError: %s", reason);                              \
        BT_DFX_SEND_AVRCP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_AVRCP, BT_DFXC_AVRCP_VOL), \
            "%s:%s", "btAvrcpVolError", reason);                                     \
    } while (0)

// hfp
#if defined(CONFIG_BLUETOOTH_DFX) && (defined(CONFIG_BLUETOOTH_HFP_HF) || defined(CONFIG_BLUETOOTH_HFP_AG))
#define BT_DFX_SEND_HFP_EVENT(...) sendEventMisightF(__VA_ARGS__)
#else
#define BT_DFX_SEND_HFP_EVENT(...)
#endif

#define BT_DFX_HFP_CONN_ERROR(reason)                                           \
    do {                                                                        \
        BT_LOGE("BT_DFX: btHfpConnectError: %s", reason);                       \
        BT_DFX_SEND_HFP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_HFP, BT_DFXC_HFP_CONN), \
            "%s:%s", "btHfpConnectError", reason);                              \
    } while (0)

#define BT_DFX_HFP_SCO_CONN_ERROR(reason)                                           \
    do {                                                                            \
        BT_LOGE("BT_DFX: btHfpScoConnectError: %s", reason);                        \
        BT_DFX_SEND_HFP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_HFP, BT_DFXC_HFP_SCO_CONN), \
            "%s:%s", "btHfpScoConnectError", reason);                               \
    } while (0)

#define BT_DFX_HFP_VOL_ERROR(reason)                                           \
    do {                                                                       \
        BT_LOGE("BT_DFX: btHfpVolError: %s", reason);                          \
        BT_DFX_SEND_HFP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_HFP, BT_DFXC_HFP_VOL), \
            "%s:%s", "btHfpVolError", reason);                                 \
    } while (0)

#define BT_DFX_HFP_MEDIA_ERROR(reason)                                           \
    do {                                                                         \
        BT_LOGE("BT_DFX: btHfpMediaError: %s", reason);                          \
        BT_DFX_SEND_HFP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_HFP, BT_DFXC_HFP_MEDIA), \
            "%s:%s", "btHfpMediaError", reason);                                 \
    } while (0)

#define BT_DFX_HFP_OFFLOAD_ERROR(reason)                                           \
    do {                                                                           \
        BT_LOGE("BT_DFX: btHfpOffloadError: %s", reason);                          \
        BT_DFX_SEND_HFP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_HFP, BT_DFXC_HFP_OFFLOAD), \
            "%s:%s", "btHfpOffloadError", reason);                                 \
    } while (0)

// hid
#if defined(CONFIG_BLUETOOTH_DFX) && defined(CONFIG_BLUETOOTH_HID_DEVICE)
#define BT_DFX_SEND_HID_EVENT(...) sendEventMisightF(__VA_ARGS__)
#else
#define BT_DFX_SEND_HID_EVENT(...)
#endif

#define BT_DFX_HID_CONN_ERROR(reason)                                           \
    do {                                                                        \
        BT_LOGE("BT_DFX: btHidConnectError: %s", reason);                       \
        BT_DFX_SEND_HID_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_HID, BT_DFXC_HID_CONN), \
            "%s:%s", "btHidConnectError", reason);                              \
    } while (0)

// others
#if defined(CONFIG_BLUETOOTH_DFX)
#define BT_DFX_SEND_OTHERS_EVENT(...) sendEventMisightF(__VA_ARGS__)
#else
#define BT_DFX_SEND_OTHERS_EVENT(...)
#endif

#define BT_DFX_SOCKET_ERROR(reason, port)                                           \
    do {                                                                            \
        BT_LOGE("BT_DFX: btSocketError: %s, port: %d", reason, port);               \
        BT_DFX_SEND_OTHERS_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_OTHERS, BT_DFXC_SOCKET), \
            "%s:%s,%s:%d", "btSocketError", reason, "port", port);                  \
    } while (0)

#define BT_DFX_IPC_CONN_ERROR(type, reason)                                           \
    do {                                                                              \
        BT_LOGE("BT_DFX: btIpcConnectError: %s, reason: %s", type, reason);           \
        BT_DFX_SEND_OTHERS_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_OTHERS, BT_DFXC_IPC_CONN), \
            "%s:%s,%s:%s", "btIpcConnectError", type, "reason", reason);              \
    } while (0)

#define BT_DFX_IPC_ALLOC_ERROR(reason, packet_code)                                        \
    do {                                                                                   \
        BT_LOGE("BT_DFX: btIpcAllocError: %s, packetCode: %" PRIu32, reason, packet_code); \
        BT_DFX_SEND_OTHERS_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_OTHERS, BT_DFXC_IPC_ALLOC),     \
            "%s:%s,%s:%" PRIu32 "", "btIpcAllocError", reason, "packetCode", packet_code); \
    } while (0)

#define BT_DFX_DRIVER_ERROR(type, name, reason)                                          \
    do {                                                                                 \
        BT_LOGE("BT_DFX: btDriverError: %s, name: %s, reason: %s", type, name, reason);  \
        BT_DFX_SEND_OTHERS_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_OTHERS, BT_DFXC_DRIVER),      \
            "%s:%s,%s:%s,%s:%s", "btDriverError", type, "name", name, "reason", reason); \
    } while (0)

#define BT_DFX_OPEN_ERROR(reason)                                                 \
    do {                                                                          \
        BT_LOGE("BT_DFX: btOpenError: %s", reason);                               \
        BT_DFX_SEND_OTHERS_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_OTHERS, BT_DFXC_OPEN), \
            "%s:%s", "btOpenError", reason);                                      \
    } while (0)

#endif /* _BT_DFX_H_ */