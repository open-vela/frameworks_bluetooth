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

#ifdef __BT_MESSAGE_CODE__
BT_PAN_MESSAGE_START,
    BT_PAN_REGISTER_CALLBACKS,
    BT_PAN_UNREGISTER_CALLBACKS,
    BT_PAN_CONNECT,
    BT_PAN_DISCONNECT,
    BT_PAN_MESSAGE_END,
#endif

#ifdef __BT_CALLBACK_CODE__
    BT_PAN_CALLBACK_START,
    BT_PAN_NETIF_STATE_CB,
    BT_PAN_CONNECTION_STATE_CB,
    BT_PAN_CALLBACK_END,
#endif

#ifndef _BT_MESSAGE_PAN_H__
#define _BT_MESSAGE_PAN_H__

#ifdef __cplusplus
    extern "C"
{
#endif

#include "bluetooth.h"
#include "bt_pan.h"

    typedef union {
        uint8_t status; /* bt_status_t */
        uint8_t pad[3];
        uint32_t v32;
    } bt_pan_result_t;

    typedef union {
        struct {
            bt_address_t addr;
            uint8_t dst_role;
            uint8_t src_role;
        } _bt_pan_connect;

        struct {
            bt_address_t addr;
        } _bt_pan_disconnect;

    } bt_message_pan_t;

    typedef struct
    {
        struct {
            char ifname[64];
            uint8_t state; /* pan_netif_state_t */
            uint8_t local_role;
        } _netif_state_cb;

        struct {
            bt_address_t bd_addr;
            uint8_t state; /* profile_connection_state_t */
            uint8_t local_role;
            uint8_t remote_role;
        } _connection_state_cb;
    } bt_message_pan_callbacks_t;

#ifdef __cplusplus
}
#endif

#endif /* _BT_MESSAGE_PAN_H__ */
