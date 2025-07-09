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

#endif /* _BT_DFX_H_ */