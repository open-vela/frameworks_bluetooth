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
#ifdef __APP_BT_MESSAGE_CODE__
APP_BT_GAP_MESSAGE_START,
    APP_BT_GAP_GET_NAME,
    APP_BT_GAP_SET_SCANMODE,
    APP_BT_GAP_SET_IO_CAPABILITY,
    APP_BT_GAP_CONNECT_REQUEST_REPLY,
    APP_BT_GAP_CONNECT_DISCONNECT,
    APP_BT_GAP_ON_GAP_STATE_CHANGED,
    APP_BT_GAP_ON_CONNECTION_STATE_CHANGED,
    APP_BT_GAP_ON_BOND_STATE_CHANGED,
    APP_BT_GAP_MESSAGE_END,
#endif

#ifndef _BT_MESSAGE_ADAPTER_H__
#define _BT_MESSAGE_ADAPTER_H__

#define BT_NAME_LENGTH 64

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bt_adapter.h"

    typedef union {
        struct {
            char name[BT_NAME_LENGTH];
        } _bt_adapter_set_name,
            _bt_adapter_get_name;

        struct {
            uint8_t mode; /* bt_scan_mode_t */
            uint8_t bondable; /* boolean */
        } _bt_adapter_set_scan_mode;

        struct {
            uint8_t cap; /* bt_io_capability_t */
        } _bt_adapter_set_io_capability;

        struct {
            bt_address_t addr;
            uint8_t accept; /* boolean */
        } _bt_device_connect_request_reply;
    } app_bt_message_gap_t;

    typedef union {
        struct {
            uint8_t state; /* bt_adapter_state_t */
        } _on_adapter_state_changed;

        struct {
            bt_address_t addr;
            uint8_t transport; /* bt_transport_t */
            uint8_t state; /* connection_state_t */
        } _on_connection_state_changed;

        struct {
            bt_address_t addr;
            uint8_t transport; /* bt_transport_t */
            uint8_t state; /* bond_state_t */
            uint8_t is_ctkd; /* boolean */
        } _on_bond_state_changed;
    } app_bt_message_gap_callbacks_t;
#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_ADAPTER_H__ */