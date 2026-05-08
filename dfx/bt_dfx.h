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

#ifdef CONFIG_DFX
#include <dfx.h>
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

// a2dp
#if defined(CONFIG_BLUETOOTH_DFX) && defined(CONFIG_BLUETOOTH_BREDR_SUPPORT)
#define BT_DFX_SEND_A2DP_EVENT(...) sendEventMisightF(__VA_ARGS__)
#else
#define BT_DFX_SEND_A2DP_EVENT(...)
#endif

#define BT_DFX_A2DP_CONN_ERROR(reason)                                               \
    do {                                                                             \
        BT_LOGE("BT_DFX: a2dpConnError: %s", reason);                                \
        BT_DFX_SEND_A2DP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_A2DP, BT_DFXC_A2DP_CONN),  \
            "%s:%s", "a2dpConnError", reason);                                       \
    } while (0)

#define BT_DFX_A2DP_OFFLOAD_ERROR(reason)                                                  \
    do {                                                                                   \
        BT_LOGE("BT_DFX: a2dpOffloadError: %s", reason);                                   \
        BT_DFX_SEND_A2DP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_A2DP, BT_DFXC_A2DP_OFFLOAD),     \
            "%s:%s", "a2dpOffloadError", reason);                                          \
    } while (0)

// hfp
#if defined(CONFIG_BLUETOOTH_DFX) && defined(CONFIG_BLUETOOTH_BREDR_SUPPORT)
#define BT_DFX_SEND_HFP_EVENT(...) sendEventMisightF(__VA_ARGS__)
#else
#define BT_DFX_SEND_HFP_EVENT(...)
#endif

#define BT_DFX_HFP_CONN_ERROR(reason)                                              \
    do {                                                                            \
        BT_LOGE("BT_DFX: hfpConnError: %s", reason);                               \
        BT_DFX_SEND_HFP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_HFP, BT_DFXC_HFP_CONN),  \
            "%s:%s", "hfpConnError", reason);                                      \
    } while (0)

#define BT_DFX_HFP_OFFLOAD_ERROR(reason)                                                 \
    do {                                                                                 \
        BT_LOGE("BT_DFX: hfpOffloadError: %s", reason);                                  \
        BT_DFX_SEND_HFP_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_HFP, BT_DFXC_HFP_OFFLOAD),     \
            "%s:%s", "hfpOffloadError", reason);                                         \
    } while (0)

// open (enable)
#if defined(CONFIG_BLUETOOTH_DFX)
#define BT_DFX_SEND_OTHERS_EVENT(...) sendEventMisightF(__VA_ARGS__)
#else
#define BT_DFX_SEND_OTHERS_EVENT(...)
#endif

#define BT_DFX_OPEN_ERROR(reason)                                                    \
    do {                                                                             \
        BT_LOGE("BT_DFX: openError: %s", reason);                                    \
        BT_DFX_SEND_OTHERS_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_OTHERS, BT_DFXC_OPEN),   \
            "%s:%s", "openError", reason);                                           \
    } while (0)

// spp / ipc
#define BT_DFX_SPP_CONN_ERROR(reason)                                                \
    do {                                                                             \
        BT_LOGE("BT_DFX: sppConnError: %s", reason);                                 \
        BT_DFX_SEND_BR_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_RFCOMM, 0),                  \
            "%s:%s", "sppConnError", reason);                                        \
    } while (0)

#define BT_DFX_IPC_CONN_ERROR(reason, extra)                                         \
    do {                                                                             \
        BT_LOGE("BT_DFX: ipcConnError: %s %s", reason, extra);                       \
        BT_DFX_SEND_BR_EVENT(BT_DFX_BUILD_CODE(BT_DFXG_RFCOMM, 0),                  \
            "%s:%s:%s", "ipcConnError", reason, extra);                              \
    } while (0)

#endif /* _BT_DFX_H_ */